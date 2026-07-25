#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "Config.hpp"
#include "Protect.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>

static void CameraAxesFlat(const Matrix3& r, Vector3& flatFwd, Vector3& flatRight)
{
    // Roblox camera CFrame basis (LookVector / RightVector)
    Vector3 look = Normalize({ -r.m[2], -r.m[5], -r.m[8] });
    // RightVector is +X column of the rotation (not negated)
    Vector3 right = Normalize({ r.m[0], r.m[3], r.m[6] });

    flatFwd = Normalize({ look.x, 0.f, look.z });
    if (flatFwd.LengthSquared() < 1e-6f)
        flatFwd = { 0.f, 0.f, -1.f };

    flatRight = Normalize({ right.x, 0.f, right.z });
    // re-orthogonalize on ground plane
    flatRight = flatRight - flatFwd * Dot(flatRight, flatFwd);
    flatRight = Normalize(flatRight);
    if (flatRight.LengthSquared() < 1e-6f)
        flatRight = Normalize(Cross({ 0.f, 1.f, 0.f }, flatFwd));
    if (flatRight.LengthSquared() < 1e-6f)
        flatRight = { 1.f, 0.f, 0.f };
}

void Engine::ApplyVelocityMovement(const Config& cfg, const CameraData& cam, uintptr_t rootPart) const
{
    if (!rootPart || !cam.valid)
        return;
    if (cfg.menuOpen)
        return;

    const uintptr_t prim = m_memory.Read<uintptr_t>(rootPart + Offsets::BasePart::Primitive);
    if (!prim)
        return;

    const bool flyHeld = cfg.flightEnabled && (
        cfg.flyKey == 0 || (GetAsyncKeyState(cfg.flyKey) & 0x8000) != 0);
    const bool speedHeld = cfg.speedEnabled && (
        cfg.speedKey == 0 || (GetAsyncKeyState(cfg.speedKey) & 0x8000) != 0);

    if (!flyHeld && !speedHeld)
        return;

    const bool w = (GetAsyncKeyState('W') & 0x8000) != 0 || (GetAsyncKeyState(0x57) & 0x8000) != 0;
    const bool a = (GetAsyncKeyState('A') & 0x8000) != 0 || (GetAsyncKeyState(0x41) & 0x8000) != 0;
    const bool s = (GetAsyncKeyState('S') & 0x8000) != 0 || (GetAsyncKeyState(0x53) & 0x8000) != 0;
    const bool d = (GetAsyncKeyState('D') & 0x8000) != 0 || (GetAsyncKeyState(0x44) & 0x8000) != 0;
    const bool upKey = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    const bool downKey = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
        || (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0
        || (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0
        || (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;

    Vector3 flatFwd{}, flatRight{};
    CameraAxesFlat(cam.rotation, flatFwd, flatRight);

    auto buildDir = [&](bool includeVert) -> Vector3 {
        Vector3 dir{};
        if (w) dir = dir + flatFwd;
        if (s) dir = dir - flatFwd;
        if (d) dir = dir + flatRight;
        if (a) dir = dir - flatRight;
        if (includeVert)
        {
            if (upKey) dir = dir + Vector3{ 0.f, 1.f, 0.f };
            if (downKey) dir = dir - Vector3{ 0.f, 1.f, 0.f };
        }
        if (dir.LengthSquared() > 1e-6f)
            dir = Normalize(dir);
        return dir;
    };

    if (flyHeld)
    {
        Vector3 dir = buildDir(true);
        const float speed = cfg.flyAmount < 1.f ? 1.f : cfg.flyAmount;

        if (dir.LengthSquared() < 1e-6f)
        {
            const Vector3 cur = m_memory.Read<Vector3>(prim + Offsets::Primitive::Position);
            m_memory.Write<Vector3>(prim + Offsets::Primitive::Position, cur);
            m_memory.Write<Vector3>(prim + Offsets::Primitive::Position, cur);
            const Vector3 zero{};
            m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, zero);
            m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, zero);
            m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
            m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
            return;
        }

        const Vector3 cur = m_memory.Read<Vector3>(prim + Offsets::Primitive::Position);
        const float dt = 0.001f;
        const Vector3 newPos = cur + dir * speed * dt;
        m_memory.Write<Vector3>(prim + Offsets::Primitive::Position, newPos);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::Position, newPos);
        const Vector3 vel = dir * speed;
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        const Vector3 zero{};
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
        return;
    }

    if (speedHeld && (w || a || s || d))
    {
        Vector3 dir = buildDir(false);
        if (dir.LengthSquared() < 1e-6f)
            return;

        const float speed = cfg.speedAmount < 1.f ? 1.f : cfg.speedAmount;
        Vector3 cur = m_memory.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
        Vector3 vel = dir * speed;
        vel.y = cur.y;
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        const Vector3 zero{};
        m_memory.Write<Vector3>(prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
    }
}

void Engine::MovementThread()
{
    constexpr double kTargetMs = 1.0; // ~1000Hz for smooth speed/flight

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        // Same sticky unlock as aim — seal soft-fails were blocking sky + movement
        if (!Protect::LicensedHot())
        {
            SleepBudget(kTargetMs, timer.End());
            continue;
        }

        const Config cfg = m_config.Get();
        if (cfg.speedEnabled || cfg.flightEnabled || cfg.jumpEnabled)
        {
            const auto players = m_players.Read();
            const auto camera = m_camera.Read();

            uintptr_t root = 0;
            uintptr_t humanoid = 0;
            if (players && players->valid)
            {
                for (const auto& e : players->entities)
                {
                    if (!e.isLocal) continue;
                    root = e.rootPart;
                    humanoid = e.humanoid;
                    break;
                }
            }

            if (cfg.jumpEnabled && humanoid)
            {
                m_memory.Write<float>(humanoid + Offsets::Humanoid::JumpPower, cfg.jumpPower);
                m_memory.Write<uint8_t>(humanoid + Offsets::Humanoid::UseJumpPower, 1);
            }

            if (root && camera && camera->valid)
                ApplyVelocityMovement(cfg, *camera, root);
        }

        // World lighting handled only by WorldThread (~60Hz)

        const double ms = timer.End();
        SleepBudget(kTargetMs, ms);
    }
}
