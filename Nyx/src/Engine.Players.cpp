#include "Engine.hpp"
#include "MapParser.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "SilentAim.hpp"
#include "Protect.hpp"
#include "Config.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <string>
#include <vector>
uintptr_t Engine::FindLocalModelFromCamera(uintptr_t workspace) const
{
    if (!workspace) return 0;
    const uintptr_t cam = m_memory.Read<uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
    if (!cam) return 0;
    const uintptr_t sub = m_memory.Read<uintptr_t>(cam + Offsets::Camera::CameraSubject);
    if (!sub) return 0;

    if (ReadClassName(sub) == "Humanoid")
        return FindModelFromHumanoid(sub);

    uintptr_t cur = sub;
    for (int i = 0; i < 12; ++i)
    {
        if (ReadClassName(cur) == "Model")
            return cur;
        cur = m_memory.Read<uintptr_t>(cur + Offsets::Instance::Parent);
        if (!cur) break;
    }
    return 0;
}

void Engine::DiscoverOnyxStandalone(uintptr_t workspace, std::vector<uintptr_t>& outModels,
    const std::unordered_set<uintptr_t>& already) const
{
    const uintptr_t folder = FindChildByName(workspace, "ONYX HITBOXES");
    if (!folder) return;

    std::vector<Vector3> knownPos;
    knownPos.reserve(already.size());
    for (uintptr_t m : already)
    {
        if (!m) continue;

        Vector3 p{};
        bool got = false;
        for (const uintptr_t c : ReadChildren(m))
        {
            const std::string n = ReadName(c);
            const std::string cls = ReadClassName(c);
            if (cls != "Part" && cls != "MeshPart") continue;
            if (n == "HumanoidRootPart" || n == "Root" || n == "Hitbox" || n == "Head")
            {
                p = ReadPartPos(c);
                got = true;
                break;
            }
        }
        if (!got)
        {
            const std::string cls = ReadClassName(m);
            if (cls == "Part" || cls == "MeshPart")
            {
                p = ReadPartPos(m);
                got = true;
            }
        }
        if (got) knownPos.push_back(p);
    }

    for (const uintptr_t c : ReadChildren(folder))
    {
        const std::string cls = ReadClassName(c);
        if (cls != "Part" && cls != "MeshPart") continue;
        if (already.count(c)) continue;

        const Vector3 pos = ReadPartPos(c);
        if (pos.LengthSquared() < 0.01f) continue;

        bool nearKnown = false;
        for (const Vector3& k : knownPos)
        {
            const float dx = pos.x - k.x, dy = pos.y - k.y, dz = pos.z - k.z;
            if (dx * dx + dy * dy + dz * dz < 100.f)
            {
                nearKnown = true;
                break;
            }
        }
        if (nearKnown) continue;

        outModels.push_back(c);
        knownPos.push_back(pos);
    }
}

void Engine::DiscoverOverkillPlayers(uintptr_t workspace, std::vector<uintptr_t>& outModels) const
{
    outModels.clear();
    if (!workspace) return;

    std::unordered_set<uintptr_t> seen;

    auto isSystem = [](const std::string& name, const std::string& cls) -> bool {

        return name == "World" || name == "Objects"
            || name == "Debris" || name == "Camera" || cls == "Terrain"
            || name == "Terrain" || name == "Baseplate" || name == "SpawnLocation"
            || name == "SoundService" || cls == "Camera";
    };

    struct Node { uintptr_t inst; int depth; };
    std::vector<Node> stack;
    for (const uintptr_t child : ReadChildren(workspace))
    {
        const std::string name = ReadName(child);
        const std::string cls = ReadClassName(child);
        if (isSystem(name, cls)) continue;

        if (name == "ONYX HITBOXES") continue;
        stack.push_back({ child, 0 });
    }

    while (!stack.empty())
    {
        const Node n = stack.back();
        stack.pop_back();
        if (!n.inst) continue;

        const std::string cls = ReadClassName(n.inst);
        if (cls == "Humanoid")
        {
            uintptr_t model = FindModelFromHumanoid(n.inst);

            if (!model)
            {
                uintptr_t cur = n.inst;
                for (int i = 0; i < 12; ++i)
                {
                    cur = m_memory.Read<uintptr_t>(cur + Offsets::Instance::Parent);
                    if (!cur || cur == workspace) break;
                    const std::string pc = ReadClassName(cur);
                    if (pc == "Model" || pc == "Folder" || pc == "Actor")
                    {
                        if (pc == "Model" || pc == "Actor")
                        {
                            model = cur;
                            break;
                        }
                        if (!model) model = cur;
                    }
                }
            }
            if (model && !seen.count(model))
            {
                seen.insert(model);
                outModels.push_back(model);
            }
        }

        if (n.depth < 26)
        {
            for (const uintptr_t c : ReadChildren(n.inst))
            {
                const std::string cn = ReadName(c);
                const std::string cc = ReadClassName(c);
                if (isSystem(cn, cc) && n.depth == 0) continue;
                stack.push_back({ c, n.depth + 1 });
            }
        }
    }

    DiscoverOnyxStandalone(workspace, outModels, seen);
}

void Engine::CollectChamParts(EntityData& entity, uintptr_t , int ) const
{

    entity.chamCount = 0;
}

bool Engine::FillEntityFromModel(EntityData& e, uintptr_t model) const
{
    if (!model) return false;
    e.address = model;
    e.character = model;
    e.hasCharacter = true;
    e.isNpc = true;
    e.isLocal = false;
    e.name = ReadName(model);
    e.displayName = e.name.empty() ? "NPC" : e.name;
    e.team = 0;
    e.chamCount = 0;

    auto children = ReadChildren(model);
    FillBones(e, children);
    if (e.validBones == 0)
    {
        for (const uintptr_t child : children)
        {
            const std::string cls = ReadClassName(child);
            if (cls == "Model" || cls == "Folder" || cls == "Actor")
            {
                const auto nested = ReadChildren(child);
                FillBones(e, nested);
                if (e.validBones > 0)
                {
                    children = nested;
                    break;
                }
            }
        }
    }
    if (e.validBones == 0)
        FillBonesDeep(e, model, 14);

    if (!e.humanoid)
    {
        // no humanoid → not a living NPC
        return false;
    }

    e.health = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::Health);
    e.maxHealth = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::MaxHealth);

    if (e.rootPart)
    {
        e.rootPos = ReadPartPos(e.rootPart);
        const uintptr_t prim = m_memory.Read<uintptr_t>(e.rootPart + Offsets::BasePart::Primitive);
        if (prim)
            e.velocity = m_memory.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
    }
    else if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid)
        e.rootPos = e.bones[static_cast<int>(BoneId::UpperTorso)].position;
    else if (e.bones[static_cast<int>(BoneId::Head)].valid)
        e.rootPos = e.bones[static_cast<int>(BoneId::Head)].position;

    return e.validBones > 0 || e.rootPos.LengthSquared() > 0.01f;
}

void Engine::AppendWorkspaceNpcs(uintptr_t workspace, PlayerFrame& frame) const
{
    if (!workspace) return;

    std::unordered_set<uintptr_t> known;
    known.reserve(frame.entities.size() * 2 + 16);
    for (const auto& e : frame.entities)
    {
        if (e.character) known.insert(e.character);
        if (e.address) known.insert(e.address);
    }

    constexpr size_t kMaxNpcs = 48;
    size_t added = 0;

    auto tryModel = [&](uintptr_t model) {
        if (!model || known.count(model) || added >= kMaxNpcs) return;
        const std::string cls = ReadClassName(model);
        if (cls != "Model" && cls != "Actor") return;

        // Must have a Humanoid child (or nested) to count as NPC
        bool hasHum = false;
        for (const uintptr_t c : ReadChildren(model))
        {
            if (ReadClassName(c) == "Humanoid") { hasHum = true; break; }
            if (ReadClassName(c) == "Model" || ReadClassName(c) == "Folder")
            {
                for (const uintptr_t c2 : ReadChildren(c))
                {
                    if (ReadClassName(c2) == "Humanoid") { hasHum = true; break; }
                }
            }
            if (hasHum) break;
        }
        if (!hasHum) return;

        EntityData e{};
        if (!FillEntityFromModel(e, model)) return;
        if (e.health <= 0.f && e.maxHealth > 1.f) return;

        known.insert(model);
        frame.entities.push_back(std::move(e));
        ++added;
    };

    // Workspace direct children + one folder level (NPCs / Mobs / Characters folders)
    for (const uintptr_t child : ReadChildren(workspace))
    {
        const std::string cls = ReadClassName(child);
        if (cls == "Model" || cls == "Actor")
        {
            tryModel(child);
        }
        else if (cls == "Folder" || cls == "Configuration" || cls == "WorldModel")
        {
            const std::string nm = ReadName(child);
            // skip heavy services / map folders when possible
            if (nm == "Terrain" || nm == "Camera" || nm == "Baseplate") continue;
            for (const uintptr_t nested : ReadChildren(child))
            {
                const std::string ncls = ReadClassName(nested);
                if (ncls == "Model" || ncls == "Actor")
                    tryModel(nested);
                else if (ncls == "Folder")
                {
                    for (const uintptr_t deep : ReadChildren(nested))
                    {
                        if (ReadClassName(deep) == "Model" || ReadClassName(deep) == "Actor")
                            tryModel(deep);
                    }
                }
            }
        }
        if (added >= kMaxNpcs) break;
    }
}

void Engine::RefreshOverkillEntity(EntityData& e, uintptr_t model)
{
    e.address = model;
    e.character = model;
    e.hasCharacter = true;
    e.chamCount = 0;
    e.name = ReadName(model);
    e.displayName = e.name;

    const auto now = std::chrono::steady_clock::now();
    OverkillRigCache& cache = m_okCache[model];

    const std::string modelCls = ReadClassName(model);
    const bool isBarePart = (modelCls == "Part" || modelCls == "MeshPart"
        || modelCls == "UnionOperation");

    const bool needStructure = !cache.ready
        || std::chrono::duration_cast<std::chrono::milliseconds>(now - cache.lastStructure).count() > 450;

    if (needStructure)
    {
        cache = {};
        cache.barePart = isBarePart;
        cache.lastStructure = now;

        if (isBarePart)
        {
            cache.rootPart = model;
            cache.ready = true;
        }
        else
        {

            bool isR15 = false;
            struct Node { uintptr_t inst; int depth; };
            std::vector<Node> stack;
            stack.push_back({ model, 0 });
            while (!stack.empty())
            {
                const Node n = stack.back();
                stack.pop_back();
                for (const uintptr_t child : ReadChildren(n.inst))
                {
                    const std::string name = ReadName(child);
                    const std::string cls = ReadClassName(child);
                    if (cls == "Humanoid")
                        cache.humanoid = child;
                    else if (cls == "Part" || cls == "MeshPart" || cls == "WedgePart"
                        || cls == "UnionOperation" || cls == "CornerWedgePart")
                    {
                        if (name == "HumanoidRootPart" || name == "Root" || name == "Hitbox")
                        {
                            if (!cache.rootPart || name == "HumanoidRootPart")
                                cache.rootPart = child;
                        }
                        else
                        {
                            const BoneId id = BoneFromName(name, isR15);
                            if (id != BoneId::Count)
                                cache.boneParts[static_cast<int>(id)] = child;
                        }
                    }
                    if (n.depth < 12)
                        stack.push_back({ child, n.depth + 1 });
                }
            }
            if (!cache.rootPart && cache.humanoid)
                cache.rootPart = m_memory.Read<uintptr_t>(cache.humanoid + Offsets::Humanoid::HumanoidRootPart);
            cache.ready = true;
        }
    }

    e.humanoid = cache.humanoid;
    e.rootPart = cache.rootPart;
    e.isR15 = false;
    if (e.humanoid && Offsets::Humanoid::RigType)
        e.isR15 = m_memory.Read<int>(e.humanoid + Offsets::Humanoid::RigType) == 0;
    e.validBones = 0;

    if (cache.barePart)
    {
        e.rootPos = ReadPartPos(model);
        e.health = 100.f;
        e.maxHealth = 100.f;
        e.bones[static_cast<int>(BoneId::Head)].position = e.rootPos;
        e.bones[static_cast<int>(BoneId::Head)].position.y += 1.2f;
        e.bones[static_cast<int>(BoneId::Head)].valid = true;
        e.bones[static_cast<int>(BoneId::UpperTorso)].position = e.rootPos;
        e.bones[static_cast<int>(BoneId::UpperTorso)].valid = true;
        e.validBones = 2;
        const uintptr_t prim = m_memory.Read<uintptr_t>(model + Offsets::BasePart::Primitive);
        if (prim)
            e.velocity = m_memory.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
        return;
    }

    for (int i = 0; i < kBoneCount; ++i)
    {
        if (!cache.boneParts[i]) continue;
        BoneJoint& bone = e.bones[i];
        bone.position = ReadPartPos(cache.boneParts[i]);
        bone.boundsPosition = bone.position;
        bone.size = ReadPartSize(cache.boneParts[i]);
        bone.valid = true;
        e.validBones++;
    }
    if (e.bones[static_cast<int>(BoneId::Head)].valid && cache.boneParts[static_cast<int>(BoneId::Head)])
        e.bones[static_cast<int>(BoneId::Head)].position.y +=
            ReadPartSize(cache.boneParts[static_cast<int>(BoneId::Head)]).y * 0.35f;

    if (!e.isR15)
    {
        auto copyBoneIf = [&](BoneId from, BoneId to) {
            if (e.bones[static_cast<int>(from)].valid && !e.bones[static_cast<int>(to)].valid)
                e.bones[static_cast<int>(to)] = e.bones[static_cast<int>(from)];
        };
        copyBoneIf(BoneId::LeftUpperArm, BoneId::LeftLowerArm);
        copyBoneIf(BoneId::LeftLowerArm, BoneId::LeftHand);
        copyBoneIf(BoneId::RightUpperArm, BoneId::RightLowerArm);
        copyBoneIf(BoneId::RightLowerArm, BoneId::RightHand);
        copyBoneIf(BoneId::LeftUpperLeg, BoneId::LeftLowerLeg);
        copyBoneIf(BoneId::LeftLowerLeg, BoneId::LeftFoot);
        copyBoneIf(BoneId::RightUpperLeg, BoneId::RightLowerLeg);
        copyBoneIf(BoneId::RightLowerLeg, BoneId::RightFoot);
    }

    if (e.humanoid)
    {
        e.health = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::Health);
        e.maxHealth = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::MaxHealth);
        if (e.maxHealth <= 0.f && e.health <= 0.f)
        {
            e.health = 100.f;
            e.maxHealth = 100.f;
        }
        else if (e.health <= 0.f && e.maxHealth > 0.f && e.validBones > 0)
            e.health = (std::max)(1.f, e.maxHealth * 0.5f);

        const uintptr_t hrp = m_memory.Read<uintptr_t>(e.humanoid + Offsets::Humanoid::HumanoidRootPart);
        if (hrp)
        {
            e.rootPart = hrp;
            cache.rootPart = hrp;
        }
    }
    else
    {
        e.health = 100.f;
        e.maxHealth = 100.f;
    }

    if (e.rootPart)
    {
        e.rootPos = ReadPartPos(e.rootPart);
        const uintptr_t prim = m_memory.Read<uintptr_t>(e.rootPart + Offsets::BasePart::Primitive);
        if (prim)
            e.velocity = m_memory.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
    }
    else if (e.bones[static_cast<int>(BoneId::Head)].valid)
        e.rootPos = e.bones[static_cast<int>(BoneId::Head)].position;
    else if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid)
        e.rootPos = e.bones[static_cast<int>(BoneId::UpperTorso)].position;
}

void Engine::MatchOnyxHitboxes(uintptr_t workspace, std::vector<EntityData>& entities) const
{
    if (!workspace || entities.empty()) return;
    const uintptr_t folder = FindChildByName(workspace, "ONYX HITBOXES");
    if (!folder) return;

    struct HB { uintptr_t part; Vector3 pos; bool used; };
    std::vector<HB> hitboxes;
    for (const uintptr_t c : ReadChildren(folder))
    {
        const std::string cls = ReadClassName(c);
        if (cls != "Part" && cls != "MeshPart") continue;
        HB h{};
        h.part = c;
        h.pos = ReadPartPos(c);
        if (h.pos.LengthSquared() < 0.01f) continue;
        h.used = false;
        hitboxes.push_back(h);
    }
    if (hitboxes.empty()) return;

    for (EntityData& e : entities)
    {

        Vector3 ref = e.rootPos;
        if (e.bones[static_cast<int>(BoneId::Head)].valid)
            ref = e.bones[static_cast<int>(BoneId::Head)].position;
        else if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid)
            ref = e.bones[static_cast<int>(BoneId::UpperTorso)].position;

        if (e.address)
        {
            for (HB& h : hitboxes)
            {
                if (h.part == e.address || h.part == e.rootPart)
                {
                    h.used = true;
                    e.rootPart = h.part;
                    e.rootPos = h.pos;
                    break;
                }
            }
        }

        float best = 22500.f;
        HB* pick = nullptr;
        for (HB& h : hitboxes)
        {
            if (h.used) continue;
            const float dx = h.pos.x - ref.x;
            const float dy = h.pos.y - ref.y;
            const float dz = h.pos.z - ref.z;
            const float d = dx * dx + dy * dy + dz * dz;
            if (d < best) { best = d; pick = &h; }
        }
        if (pick)
        {
            pick->used = true;
            e.rootPart = pick->part;
            e.rootPos = pick->pos;

            if (e.validBones <= 0)
            {
                e.bones[static_cast<int>(BoneId::Head)].position = pick->pos;
                e.bones[static_cast<int>(BoneId::Head)].position.y += 1.2f;
                e.bones[static_cast<int>(BoneId::Head)].valid = true;
                e.bones[static_cast<int>(BoneId::UpperTorso)].position = pick->pos;
                e.bones[static_cast<int>(BoneId::UpperTorso)].valid = true;
                e.validBones = 2;
            }
        }
    }
}

uintptr_t Engine::ResolveCharacter(uintptr_t player, const std::string& playerName, uintptr_t workspace) const
{
    uintptr_t character = m_memory.Read<uintptr_t>(player + Offsets::Player::ModelInstance);
    if (character) return character;

    for (const uintptr_t child : ReadChildren(player))
    {
        const std::string cls = ReadClassName(child);
        if (cls == "Model" || cls == "Actor")
        {
            for (const uintptr_t c : ReadChildren(child))
            {
                if (ReadClassName(c) == "Humanoid" || ReadName(c) == "Humanoid")
                    return child;
            }
        }
    }

    if (workspace && !playerName.empty())
    {
        for (const uintptr_t child : ReadChildren(workspace))
        {
            if (ReadName(child) == playerName)
            {
                const std::string cls = ReadClassName(child);
                if (cls == "Model" || cls == "Actor" || cls.empty())
                    return child;
            }
        }
    }
    return 0;
}

void Engine::PlayersThread()
{
    constexpr double kTargetMs = 1.0;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        PlayerFrame frame{};
        const auto actors = m_actors.Read();
        if (actors && actors->valid)
        {
            frame.entities.reserve(actors->actors.size());

            uintptr_t workspace = 0;
            bool overkill = false;
            if (const auto globals = m_globals.Read())
            {
                workspace = globals->workspace;
                overkill = globals->overkillMode;
            }

            if (overkill)
            {
                Vector3 camPos{};
                if (const auto cam = m_camera.Read(); cam && cam->valid)
                    camPos = cam->position;

                {
                    std::unordered_set<uintptr_t> live(actors->actors.begin(), actors->actors.end());
                    for (auto it = m_okCache.begin(); it != m_okCache.end(); )
                    {
                        if (!live.count(it->first)) it = m_okCache.erase(it);
                        else ++it;
                    }
                }

                for (const uintptr_t model : actors->actors)
                {
                    EntityData e{};
                    RefreshOverkillEntity(e, model);
                    e.isLocal = (model == actors->localPlayer);

                    if (e.rootPos.LengthSquared() < 0.01f && !e.rootPart && e.validBones <= 0)
                        continue;

                    if (!e.isLocal && camPos.LengthSquared() > 0.01f)
                    {
                        if ((e.rootPos - camPos).Length() < 2.5f)
                            e.isLocal = true;
                    }

                    frame.entities.push_back(std::move(e));
                }

                const auto now = std::chrono::steady_clock::now();
                if (workspace && (m_lastOnyxMatch.time_since_epoch().count() == 0
                    || std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastOnyxMatch).count() > 200))
                {
                    MatchOnyxHitboxes(workspace, frame.entities);
                    m_lastOnyxMatch = now;

                    for (auto& e : frame.entities)
                    {
                        if (!e.rootPart) continue;
                        auto it = m_okCache.find(e.address);
                        if (it != m_okCache.end())
                            it->second.rootPart = e.rootPart;
                    }
                }
                else
                {

                    for (auto& e : frame.entities)
                    {
                        if (e.rootPart)
                            e.rootPos = ReadPartPos(e.rootPart);
                    }
                }

                const uintptr_t localModel = actors->localPlayer;
                float nearestLocal = 1e9f;
                size_t nearestLocalIdx = static_cast<size_t>(-1);
                for (size_t i = 0; i < frame.entities.size(); ++i)
                {
                    auto& e = frame.entities[i];
                    e.isLocal = (localModel && (e.address == localModel || e.character == localModel));
                    if (!e.isLocal && camPos.LengthSquared() > 0.01f)
                    {
                        const float d = (e.rootPos - camPos).Length();
                        if (d < 2.5f && d < nearestLocal)
                        {
                            nearestLocal = d;
                            nearestLocalIdx = i;
                        }
                    }
                }
                if (nearestLocalIdx != static_cast<size_t>(-1))
                    frame.entities[nearestLocalIdx].isLocal = true;

                frame.valid = true;
                frame.sequence = m_seqPlayers.fetch_add(1, std::memory_order_relaxed) + 1;
                m_players.Publish(std::move(frame));

                const double ms = timer.End();
                m_timings.players.Record(ms);
                SleepBudget(kTargetMs, ms);
                continue;
            }

            for (const uintptr_t player : actors->actors)
            {
                EntityData e{};
                e.address = player;
                e.isLocal = player == actors->localPlayer;
                e.name = ReadName(player);
                e.displayName = m_memory.ReadStringPointer(player + Offsets::Player::DisplayName);

                e.character = ResolveCharacter(player, e.name, workspace);
                if (!e.character)
                {

                    frame.entities.push_back(std::move(e));
                    continue;
                }

                e.hasCharacter = true;
                auto children = ReadChildren(e.character);
                FillBones(e, children);

                if (e.validBones == 0)
                {
                    for (const uintptr_t child : children)
                    {
                        const std::string cls = ReadClassName(child);
                        if (cls == "Model" || cls == "Folder" || cls == "Actor")
                        {
                            const auto nested = ReadChildren(child);
                            FillBones(e, nested);
                            if (e.validBones > 0)
                            {
                                children = nested;
                                break;
                            }
                        }
                    }
                }

                if (e.validBones == 0)
                    FillBonesDeep(e, e.character, 12);

                if (e.humanoid)
                {
                    e.health = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::Health);
                    e.maxHealth = m_memory.Read<float>(e.humanoid + Offsets::Humanoid::MaxHealth);
                }

                e.team = m_memory.Read<uintptr_t>(player + Offsets::Player::Team);

                if (e.rootPart)
                {
                    e.rootPos = ReadPartPos(e.rootPart);
                    const uintptr_t prim = m_memory.Read<uintptr_t>(e.rootPart + Offsets::BasePart::Primitive);
                    if (prim)
                        e.velocity = m_memory.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
                }
                else if (e.bones[static_cast<int>(BoneId::LowerTorso)].valid)
                    e.rootPos = e.bones[static_cast<int>(BoneId::LowerTorso)].position;
                else if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid)
                    e.rootPos = e.bones[static_cast<int>(BoneId::UpperTorso)].position;
                else if (e.bones[static_cast<int>(BoneId::Head)].valid)
                    e.rootPos = e.bones[static_cast<int>(BoneId::Head)].position;

                for (const uintptr_t child : children)
                {
                    const std::string cls = ReadClassName(child);
                    if (cls == "Tool" || cls == "HopperBin")
                    {
                        e.equipped = ReadName(child);
                        break;
                    }
                }

                if (e.isLocal && e.humanoid)
                {
                    const Config cfg = m_config.Get();
                    if (cfg.jumpEnabled)
                    {
                        m_memory.Write<float>(e.humanoid + Offsets::Humanoid::JumpPower, cfg.jumpPower);
                        m_memory.Write<uint8_t>(e.humanoid + Offsets::Humanoid::UseJumpPower, 1);
                    }
                }

                e.chamCount = 0;
                frame.entities.push_back(std::move(e));
            }

            // NPC models in workspace (Humanoid, not a Player character)
            {
                const Config cfgN = m_config.Get();
                if (cfgN.npcEsp && workspace)
                    AppendWorkspaceNpcs(workspace, frame);
            }

            frame.valid = true;
            frame.sequence = m_seqPlayers.fetch_add(1, std::memory_order_relaxed) + 1;
            m_players.Publish(std::move(frame));
        }

        const double ms = timer.End();
        m_timings.players.Record(ms);
        SleepBudget(kTargetMs, ms);
    }
}
