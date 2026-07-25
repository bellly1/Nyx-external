#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "Config.hpp"
#include <algorithm>
#include <chrono>
#include <string>
#include <cmath>

void Engine::OcclusionThread()
{
    constexpr double kTargetMs = 350.0;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        OcclusionFrame frame{};
        const auto globals = m_globals.Read();
        const Config cfg = m_config.Get();

        // Must scan for aim vis-check AND ESP Vis Check coloring
        const bool wantVis = cfg.aimVisibleCheck || cfg.silentVisibleCheck || cfg.visCheckEsp;

        if (wantVis && globals && globals->valid && globals->workspace)
        {
            m_mapParser.Scan(globals->workspace);
            frame.parts = m_mapParser.PartsCopy();
            frame.valid = !frame.parts.empty();
        }
        else if (globals && globals->valid && globals->workspace)
        {
            frame.valid = false;
        }

        frame.sequence = m_seqOcclusion.fetch_add(1, std::memory_order_relaxed) + 1;
        m_occlusion.Publish(std::move(frame));

        // ESP needs fresher maps; aim can share the same rate
        const double budget = wantVis ? (cfg.visCheckEsp ? 180.0 : 250.0) : kTargetMs;
        const double ms = timer.End();
        SleepBudget(budget, ms);
    }
}

static bool IsOnScreenFront(const CameraData& cam, const Vector3& world)
{
    if (!cam.valid) return false;
    Vector2 screen{};
    if (!WorldToScreen(world, cam.viewMatrix, cam.dimensionsX, cam.dimensionsY, screen))
        return false;
    if (screen.x < -20.f || screen.y < -20.f
        || screen.x > cam.dimensionsX + 20.f || screen.y > cam.dimensionsY + 20.f)
        return false;

    Vector3 toTarget = world - cam.position;
    const float len = toTarget.Length();
    if (len < 0.05f) return true;
    toTarget = toTarget * (1.f / len);

    Vector3 forward{ -cam.rotation.m[2], -cam.rotation.m[5], -cam.rotation.m[8] };
    forward = Normalize(forward);
    return Dot(forward, toTarget) > 0.0f;
}

bool Engine::HasLineOfSight(const CameraData& cam, const Vector3& target,
    const OcclusionFrame* occ, uintptr_t skipPlayer) const
{
    if (!cam.valid) return false;

    // Eye height slightly above camera
    Vector3 origin = cam.position;
    origin.y += 0.45f;

    Vector3 to = target - origin;
    const float len = to.Length();
    if (len < 0.6f) return true;

    Vector3 forward{ -cam.rotation.m[2], -cam.rotation.m[5], -cam.rotation.m[8] };
    forward = Normalize(forward);
    const Vector3 dir = to * (1.f / len);

    // No wall map yet → on-screen / in-front only (don't false-block)
    if (!occ || !occ->valid || occ->parts.empty())
        return IsOnScreenFront(cam, target);

    // Nudge start/end so we don't self-hit camera volume or target hull
    const float nudgeIn = (std::min)(0.35f, len * 0.08f);
    const float nudgeOut = (std::min)(0.55f, len * 0.12f);
    const Vector3 rayStart = origin + dir * nudgeIn;
    const Vector3 rayEnd = target - dir * nudgeOut;

    if (!m_mapParser.IsVisible(rayStart, rayEnd, skipPlayer, false, nullptr))
        return false;

    return IsOnScreenFront(cam, target);
}

// Multi-point check for ESP: head / chest / pelvis — visible if any ray is clear
bool Engine::IsEntityVisibleEsp(const CameraData& cam, const EntityData& e,
    const OcclusionFrame* occ) const
{
    if (!cam.valid || !e.hasCharacter) return false;

    Vector3 samples[4]{};
    int n = 0;

    auto addBone = [&](BoneId id, float yBoost) {
        const int i = (int)id;
        if (i >= 0 && i < kBoneCount && e.bones[i].valid)
            samples[n++] = e.bones[i].position;
        else
        {
            Vector3 p = e.rootPos;
            p.y += yBoost;
            samples[n++] = p;
        }
    };

    addBone(BoneId::Head, 1.55f);
    addBone(BoneId::UpperTorso, 1.15f);
    addBone(BoneId::LowerTorso, 0.75f);
    // Slight offset to the side helps when only a shoulder peeks a corner
    if (n > 0)
    {
        Vector3 side = samples[0];
        side.x += 0.35f;
        samples[n++] = side;
    }

    int clear = 0;
    for (int i = 0; i < n; ++i)
    {
        if (HasLineOfSight(cam, samples[i], occ, e.address))
            ++clear;
    }
    // Any clear sample = visible (peeking around walls still shows green)
    return clear > 0;
}
