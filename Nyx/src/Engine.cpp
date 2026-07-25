#include "Engine.hpp"
#include "MapParser.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "SilentAim.hpp"
#include "Protect.hpp"
#include <thread>
#include <chrono>

Engine::Engine(Memory& memory)
    : m_memory(memory)
    , m_mapParser(memory)
{
}

Engine::~Engine()
{
    Stop();
}

void Engine::Start()
{
    running.store(true, std::memory_order_release);
    m_globalsThread = std::thread(&Engine::GlobalsThread, this);
    m_actorsThread = std::thread(&Engine::ActorsThread, this);
    m_playersThread = std::thread(&Engine::PlayersThread, this);
    m_cameraThread = std::thread(&Engine::CameraThread, this);
    m_aimThread = std::thread(&Engine::AimThread, this);
    m_movementThread = std::thread(&Engine::MovementThread, this);
    m_worldThread = std::thread(&Engine::WorldThread, this);
    m_occlusionThread = std::thread(&Engine::OcclusionThread, this);
    m_triggerbotThread = std::thread(&Engine::TriggerbotThread, this);
}

void Engine::Stop()
{
    running.store(false, std::memory_order_release);

    if (m_globalsThread.joinable()) m_globalsThread.join();
    if (m_actorsThread.joinable()) m_actorsThread.join();
    if (m_playersThread.joinable()) m_playersThread.join();
    if (m_cameraThread.joinable()) m_cameraThread.join();
    if (m_aimThread.joinable()) m_aimThread.join();
    if (m_movementThread.joinable()) m_movementThread.join();
    if (m_worldThread.joinable()) m_worldThread.join();
    if (m_occlusionThread.joinable()) m_occlusionThread.join();
    if (m_triggerbotThread.joinable()) m_triggerbotThread.join();
}

void Engine::SleepBudget(double targetMs, double elapsedMs) const
{
    const double remain = targetMs - elapsedMs;
    if (remain <= 0.05)
        return;
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(remain));
}

std::string Engine::ReadName(uintptr_t instance) const
{
    if (!instance) return {};
    return m_memory.ReadStringPointer(instance + Offsets::Instance::Name);
}

std::string Engine::ReadClassName(uintptr_t instance) const
{
    if (!instance) return {};
    const uintptr_t desc = m_memory.Read<uintptr_t>(instance + Offsets::Instance::ClassDescriptor);
    if (!desc) return {};
    return m_memory.ReadStringPointer(desc + Offsets::Instance::ClassName);
}

std::vector<uintptr_t> Engine::ReadChildren(uintptr_t instance) const
{
    std::vector<uintptr_t> out;
    if (!instance) return out;

    const uintptr_t container = m_memory.Read<uintptr_t>(instance + Offsets::Instance::ChildrenStart);
    if (!container) return out;

    const uintptr_t start = m_memory.Read<uintptr_t>(container);
    const uintptr_t end = m_memory.Read<uintptr_t>(container + Offsets::Instance::ChildrenEnd);
    if (!start || !end || end < start) return out;

    const size_t count = (end - start) / 0x10;
    if (!count || count > 2048) return out;

    out.reserve(count);
    for (uintptr_t n = start; n < end; n += 0x10)
    {
        const uintptr_t child = m_memory.Read<uintptr_t>(n);
        if (child) out.push_back(child);
    }
    return out;
}

Vector3 Engine::ReadPartPos(uintptr_t part) const
{
    if (!part) return {};
    const uintptr_t prim = m_memory.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
    if (!prim) return {};
    return m_memory.Read<Vector3>(prim + Offsets::Primitive::Position);
}

Vector3 Engine::ReadPartSize(uintptr_t part) const
{
    if (!part) return {};
    const uintptr_t prim = m_memory.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
    if (!prim) return {};
    return m_memory.Read<Vector3>(prim + Offsets::Primitive::Size);
}

BoneId Engine::BoneFromName(const std::string& name, bool& isR15) const
{
    if (name == "Head") return BoneId::Head;
    if (name == "UpperTorso") { isR15 = true; return BoneId::UpperTorso; }
    if (name == "LowerTorso") { isR15 = true; return BoneId::LowerTorso; }
    if (name == "Torso") { isR15 = false; return BoneId::UpperTorso; }
    if (name == "LeftUpperArm") { isR15 = true; return BoneId::LeftUpperArm; }
    if (name == "LeftLowerArm") { isR15 = true; return BoneId::LeftLowerArm; }
    if (name == "LeftHand") { isR15 = true; return BoneId::LeftHand; }
    if (name == "RightUpperArm") { isR15 = true; return BoneId::RightUpperArm; }
    if (name == "RightLowerArm") { isR15 = true; return BoneId::RightLowerArm; }
    if (name == "RightHand") { isR15 = true; return BoneId::RightHand; }
    if (name == "LeftUpperLeg") { isR15 = true; return BoneId::LeftUpperLeg; }
    if (name == "LeftLowerLeg") { isR15 = true; return BoneId::LeftLowerLeg; }
    if (name == "LeftFoot") { isR15 = true; return BoneId::LeftFoot; }
    if (name == "RightUpperLeg") { isR15 = true; return BoneId::RightUpperLeg; }
    if (name == "RightLowerLeg") { isR15 = true; return BoneId::RightLowerLeg; }
    if (name == "RightFoot") { isR15 = true; return BoneId::RightFoot; }
    if (name == "Left Arm") { isR15 = false; return BoneId::LeftUpperArm; }
    if (name == "Right Arm") { isR15 = false; return BoneId::RightUpperArm; }
    if (name == "Left Leg") { isR15 = false; return BoneId::LeftUpperLeg; }
    if (name == "Right Leg") { isR15 = false; return BoneId::RightUpperLeg; }
    return BoneId::Count;
}

void Engine::FillBones(EntityData& entity, const std::vector<uintptr_t>& children) const
{
    bool isR15 = false;
    uintptr_t parts[kBoneCount]{};
    uintptr_t headPart = 0;

    for (const uintptr_t child : children)
    {
        const std::string name = ReadName(child);
        if (name.empty()) continue;

        if (name == "HumanoidRootPart" || name == "Root" || name == "Hitbox")
        {
            if (!entity.rootPart || name == "HumanoidRootPart")
                entity.rootPart = child;
            continue;
        }
        if (name == "Humanoid") { entity.humanoid = child; continue; }

        const BoneId id = BoneFromName(name, isR15);
        if (id == BoneId::Count) continue;

        parts[static_cast<int>(id)] = child;
        if (id == BoneId::Head) headPart = child;
        if (id == BoneId::UpperTorso && name == "Torso")
            parts[static_cast<int>(BoneId::LowerTorso)] = child;
    }

    entity.isR15 = isR15;
    if (!entity.isR15 && entity.humanoid && Offsets::Humanoid::RigType)
        entity.isR15 = m_memory.Read<int>(entity.humanoid + Offsets::Humanoid::RigType) == 0;

    if (!entity.humanoid)
    {
        for (const uintptr_t child : children)
        {
            if (ReadClassName(child) == "Humanoid")
            {
                entity.humanoid = child;
                break;
            }
        }
    }

    if (!entity.rootPart && entity.humanoid)
        entity.rootPart = m_memory.Read<uintptr_t>(entity.humanoid + Offsets::Humanoid::HumanoidRootPart);

    entity.validBones = 0;
    for (int i = 0; i < kBoneCount; ++i)
    {
        if (!parts[i]) continue;
        BoneJoint& bone = entity.bones[i];
        bone.position = ReadPartPos(parts[i]);
        bone.boundsPosition = bone.position;
        bone.size = ReadPartSize(parts[i]);
        bone.valid = true;
        entity.validBones++;
    }

    if (entity.bones[static_cast<int>(BoneId::Head)].valid && headPart)
        entity.bones[static_cast<int>(BoneId::Head)].position.y += ReadPartSize(headPart).y * 0.35f;

    if (entity.bones[static_cast<int>(BoneId::LeftFoot)].valid && parts[static_cast<int>(BoneId::LeftFoot)])
        entity.bones[static_cast<int>(BoneId::LeftFoot)].position.y -= ReadPartSize(parts[static_cast<int>(BoneId::LeftFoot)]).y * 0.35f;

    if (entity.bones[static_cast<int>(BoneId::RightFoot)].valid && parts[static_cast<int>(BoneId::RightFoot)])
        entity.bones[static_cast<int>(BoneId::RightFoot)].position.y -= ReadPartSize(parts[static_cast<int>(BoneId::RightFoot)]).y * 0.35f;

    if (!isR15)
    {
        auto copyIf = [&](BoneId from, BoneId to) {
            if (entity.bones[static_cast<int>(from)].valid && !entity.bones[static_cast<int>(to)].valid)
                entity.bones[static_cast<int>(to)] = entity.bones[static_cast<int>(from)];
        };
        copyIf(BoneId::LeftUpperArm, BoneId::LeftLowerArm);
        copyIf(BoneId::LeftLowerArm, BoneId::LeftHand);
        copyIf(BoneId::RightUpperArm, BoneId::RightLowerArm);
        copyIf(BoneId::RightLowerArm, BoneId::RightHand);
        copyIf(BoneId::LeftUpperLeg, BoneId::LeftLowerLeg);
        copyIf(BoneId::LeftLowerLeg, BoneId::LeftFoot);
        copyIf(BoneId::RightUpperLeg, BoneId::RightLowerLeg);
        copyIf(BoneId::RightLowerLeg, BoneId::RightFoot);
    }
}

void Engine::FillBonesDeep(EntityData& entity, uintptr_t root, int maxDepth) const
{
    if (!root) return;

    bool isR15 = false;
    uintptr_t parts[kBoneCount]{};
    uintptr_t headPart = 0;

    struct Node { uintptr_t inst; int depth; };
    std::vector<Node> stack;
    stack.push_back({ root, 0 });

    while (!stack.empty())
    {
        const Node n = stack.back();
        stack.pop_back();
        for (const uintptr_t child : ReadChildren(n.inst))
        {
            const std::string name = ReadName(child);
            const std::string cls = ReadClassName(child);

            if (cls == "Humanoid")
            {
                entity.humanoid = child;
            }
            else if (cls == "Part" || cls == "MeshPart" || cls == "WedgePart"
                || cls == "UnionOperation" || cls == "CornerWedgePart")
            {
                if (name == "HumanoidRootPart" || name == "Root" || name == "Hitbox")
                {
                    if (!entity.rootPart || name == "HumanoidRootPart")
                        entity.rootPart = child;
                }
                else
                {
                    const BoneId id = BoneFromName(name, isR15);
                    if (id != BoneId::Count)
                    {
                        parts[static_cast<int>(id)] = child;
                        if (id == BoneId::Head) headPart = child;
                    }
                }
            }

            if (n.depth < maxDepth)
                stack.push_back({ child, n.depth + 1 });
        }
    }

    entity.isR15 = isR15;
    if (!entity.isR15 && entity.humanoid && Offsets::Humanoid::RigType)
        entity.isR15 = m_memory.Read<int>(entity.humanoid + Offsets::Humanoid::RigType) == 0;
    if (!entity.rootPart && entity.humanoid)
        entity.rootPart = m_memory.Read<uintptr_t>(entity.humanoid + Offsets::Humanoid::HumanoidRootPart);

    entity.validBones = 0;
    for (int i = 0; i < kBoneCount; ++i)
    {
        if (!parts[i]) continue;
        BoneJoint& bone = entity.bones[i];
        bone.position = ReadPartPos(parts[i]);
        bone.boundsPosition = bone.position;
        bone.size = ReadPartSize(parts[i]);
        bone.valid = true;
        entity.validBones++;
    }

    if (entity.bones[static_cast<int>(BoneId::Head)].valid && headPart)
        entity.bones[static_cast<int>(BoneId::Head)].position.y += ReadPartSize(headPart).y * 0.35f;

    if (!isR15)
    {
        auto copyIf = [&](BoneId from, BoneId to) {
            if (entity.bones[static_cast<int>(from)].valid && !entity.bones[static_cast<int>(to)].valid)
                entity.bones[static_cast<int>(to)] = entity.bones[static_cast<int>(from)];
        };
        copyIf(BoneId::LeftUpperArm, BoneId::LeftLowerArm);
        copyIf(BoneId::LeftLowerArm, BoneId::LeftHand);
        copyIf(BoneId::RightUpperArm, BoneId::RightLowerArm);
        copyIf(BoneId::RightLowerArm, BoneId::RightHand);
        copyIf(BoneId::LeftUpperLeg, BoneId::LeftLowerLeg);
        copyIf(BoneId::LeftLowerLeg, BoneId::LeftFoot);
        copyIf(BoneId::RightUpperLeg, BoneId::RightLowerLeg);
        copyIf(BoneId::RightLowerLeg, BoneId::RightFoot);
    }

    if (entity.rootPart)
        entity.rootPos = ReadPartPos(entity.rootPart);
    else if (entity.bones[static_cast<int>(BoneId::Head)].valid)
        entity.rootPos = entity.bones[static_cast<int>(BoneId::Head)].position;
    else if (entity.bones[static_cast<int>(BoneId::UpperTorso)].valid)
        entity.rootPos = entity.bones[static_cast<int>(BoneId::UpperTorso)].position;
}

bool Engine::IsOverkillPlace(int64_t placeId, int64_t gameId) const
{
    if (placeId == 124842176624983LL
        || placeId == 74996816424339LL
        || placeId == 7872343282LL)
        return true;

    if (gameId == 7872343282LL
        || gameId == 74996816424339LL
        || gameId == 124842176624983LL)
        return true;
    return false;
}

bool Engine::DetectOverkillWorkspace(uintptr_t workspace) const
{
    if (!workspace) return false;

    return FindChildByName(workspace, "ONYX HITBOXES") != 0;
}

uintptr_t Engine::FindChildByName(uintptr_t parent, const char* name) const
{
    if (!parent || !name) return 0;
    for (const uintptr_t c : ReadChildren(parent))
    {
        if (ReadName(c) == name)
            return c;
    }
    return 0;
}

uintptr_t Engine::FindModelFromHumanoid(uintptr_t humanoid) const
{
    uintptr_t cur = humanoid;
    for (int i = 0; i < 16; ++i)
    {
        const uintptr_t parent = m_memory.Read<uintptr_t>(cur + Offsets::Instance::Parent);
        if (!parent) break;
        if (ReadClassName(parent) == "Model")
            return parent;
        cur = parent;
    }
    return 0;
}
