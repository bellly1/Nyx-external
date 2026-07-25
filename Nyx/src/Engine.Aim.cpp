#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "SilentAim.hpp"
#include "Protect.hpp"
#include "Config.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <string>

Vector3 Engine::AimPoint(const EntityData& e, AimBone bone) const
{
    BoneId id = BoneId::Head;
    if (bone == AimBone::UpperTorso || bone == AimBone::Arms) id = BoneId::UpperTorso;
    if (bone == AimBone::LowerTorso || bone == AimBone::Legs) id = BoneId::LowerTorso;

    if (e.bones[static_cast<int>(id)].valid)
        return e.bones[static_cast<int>(id)].position;
    if (e.bones[static_cast<int>(BoneId::Head)].valid)
        return e.bones[static_cast<int>(BoneId::Head)].position;
    return e.rootPos;
}

Vector3 Engine::ClosestBoneToCursor(const EntityData& e, const Vector2& cursor, const Matrix4& view, float screenW, float screenH) const
{
    const BoneId candidates[] = {
        BoneId::Head, BoneId::UpperTorso, BoneId::LowerTorso,
        BoneId::LeftUpperArm, BoneId::LeftLowerArm,
        BoneId::RightUpperArm, BoneId::RightLowerArm,
        BoneId::LeftUpperLeg, BoneId::LeftLowerLeg, BoneId::LeftFoot,
        BoneId::RightUpperLeg, BoneId::RightLowerLeg, BoneId::RightFoot
    };

    float bestDist = 1e18f;
    Vector3 bestPos = e.rootPos;

    for (BoneId bid : candidates)
    {
        const auto& bone = e.bones[static_cast<int>(bid)];
        if (!bone.valid) continue;
        Vector2 screen{};
        if (!AimWorldToScreen(bone.position, screen, view, Vector2{ screenW, screenH }))
            continue;
        float dx = screen.x - cursor.x;
        float dy = screen.y - cursor.y;
        float d = dx * dx + dy * dy;
        if (d < bestDist)
        {
            bestDist = d;
            bestPos = bone.position;
        }
    }

    if (bestDist > 1e17f)
    {
        if (e.bones[static_cast<int>(BoneId::Head)].valid)
            return e.bones[static_cast<int>(BoneId::Head)].position;
        return e.rootPos;
    }
    return bestPos;
}

HWND Engine::FindRobloxWindow() const
{
    struct EnumData { DWORD pid; HWND result; } data{ m_memory.GetProcessId(), nullptr };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* d = reinterpret_cast<EnumData*>(lParam);
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != d->pid || !IsWindowVisible(hwnd)) return TRUE;

        wchar_t title[256]{};
        GetWindowTextW(hwnd, title, 256);
        if (!title[0]) return TRUE;

        RECT rc{};
        if (!GetClientRect(hwnd, &rc) || rc.right < 100 || rc.bottom < 100) return TRUE;

        d->result = hwnd;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&data));

    return data.result;
}

Vector2 Engine::CursorInGame(const GlobalsData& g, const CameraData& cam) const
{
    return CursorInViewport(g, Vector2{ cam.dimensionsX, cam.dimensionsY });
}

Vector2 Engine::CursorInViewport(const GlobalsData& g, const Vector2& viewport) const
{
    POINT pt{};
    GetCursorPos(&pt);

    HWND hwnd = g.robloxHwnd ? reinterpret_cast<HWND>(g.robloxHwnd) : FindRobloxWindow();
    if (hwnd)
    {
        POINT client = pt;
        ScreenToClient(hwnd, &client);

        float x = static_cast<float>(client.x);
        float y = static_cast<float>(client.y);

        RECT rc{};
        GetClientRect(hwnd, &rc);
        const float cw = static_cast<float>(rc.right - rc.left);
        const float ch = static_cast<float>(rc.bottom - rc.top);

        if (cw > 1.f && ch > 1.f && viewport.x > 0.f && viewport.y > 0.f)
        {
            x *= viewport.x / cw;
            y *= viewport.y / ch;
        }
        return { x, y };
    }

    if (viewport.x > 0.f && viewport.y > 0.f)
        return { viewport.x * 0.5f, viewport.y * 0.5f };
    return {};
}

void Engine::MoveMouseRelative(int dx, int dy) const
{
    if (dx == 0 && dy == 0) return;

    if (dx > 300) dx = 300;
    if (dx < -300) dx = -300;
    if (dy > 300) dy = 300;
    if (dy < -300) dy = -300;

    // Chunk into ≤127 so raw-input games register every step
    while (dx != 0 || dy != 0)
    {
        int sx = dx;
        int sy = dy;
        if (sx > 127) sx = 127;
        if (sx < -127) sx = -127;
        if (sy > 127) sy = 127;
        if (sy < -127) sy = -127;

        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dx = sx;
        in.mi.dy = sy;
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        if (SendInput(1, &in, sizeof(INPUT)) == 0)
            mouse_event(MOUSEEVENTF_MOVE, (DWORD)(LONG)sx, (DWORD)(LONG)sy, 0, 0);

        dx -= sx;
        dy -= sy;
    }
}

void Engine::MoveMouseTowardScreen(HWND hwnd, float targetClientX, float targetClientY, float strength) const
{
    // Prefer relative delta from current cursor → target (SetCursorPos is ignored by Roblox)
    if (!hwnd || strength <= 0.f) return;
    if (strength > 1.f) strength = 1.f;

    POINT cur{};
    GetCursorPos(&cur);
    POINT target{ static_cast<LONG>(targetClientX + 0.5f), static_cast<LONG>(targetClientY + 0.5f) };
    ClientToScreen(hwnd, &target);

    const float dx = (float)(target.x - cur.x) * strength;
    const float dy = (float)(target.y - cur.y) * strength;
    const int mx = (int)(dx >= 0.f ? dx + 0.5f : dx - 0.5f);
    const int my = (int)(dy >= 0.f ? dy + 0.5f : dy - 0.5f);
    if (mx != 0 || my != 0)
        MoveMouseRelative(mx, my);
}

void Engine::ApplySilentAim(const GlobalsData& g, const Vector3& world) const
{
    // Offsets must be live (FetchOffsets). Without Viewport, silent cannot write.
    if (!Offsets::Camera::Viewport || !Offsets::VisualEngine::Pointer)
        return;
    HWND hwnd = g.robloxHwnd ? reinterpret_cast<HWND>(g.robloxHwnd) : FindRobloxWindow();
    SilentAim::ApplyLoop(m_memory, g, world, hwnd);
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

static bool IsTargetVisible(const CameraData& cam, const Vector3& world)
{
    return IsOnScreenFront(cam, world);
}

static bool BestTriggerScreen(const EntityData& e, const CameraData& cam,
    float screenW, float screenH, const Vector2& cursor, float pred,
    Vector2& outScreen, Vector3& outWorld, float& outDist)
{
    const int bones[] = {
        (int)BoneId::Head,
        (int)BoneId::UpperTorso,
        (int)BoneId::LowerTorso,
    };

    float best = 1e9f;
    bool any = false;
    for (int bi : bones)
    {
        Vector3 world = e.rootPos;
        if (e.bones[bi].valid)
            world = e.bones[bi].position;
        else if (bi != (int)BoneId::Head)
            continue;
        if (pred > 0.001f)
            world = world + e.velocity * pred;

        Vector2 screen{};
        if (!WorldToScreen(world, cam.viewMatrix, screenW, screenH, screen))
            continue;

        const float d = Distance2D(screen, cursor);
        if (d < best)
        {
            best = d;
            outScreen = screen;
            outWorld = world;
            outDist = d;
            any = true;
        }
    }
    return any;
}

void Engine::ClickMouse(float releaseMs) const
{

    INPUT down{};
    down.type = INPUT_MOUSE;
    down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    if (SendInput(1, &down, sizeof(INPUT)) == 0)
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);

    const int wait = static_cast<int>((std::max)(8.f, (std::min)(120.f, releaseMs)));
    std::this_thread::sleep_for(std::chrono::milliseconds(wait));

    INPUT up{};
    up.type = INPUT_MOUSE;
    up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    if (SendInput(1, &up, sizeof(INPUT)) == 0)
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void Engine::AimThread()
{
    constexpr double kTargetMs = 2.0; // ~500Hz — less thrash than 1ms spam
    Vector2 smoothCarry{};
    uintptr_t stickyTarget = 0;
    uint64_t lockSeq = 0;

    auto lastTriggerClick = std::chrono::steady_clock::now();
    uintptr_t hitTrackAddr = 0;
    float hitTrackHp = -1.f;
    auto lastTracerTick = std::chrono::steady_clock::now();
    auto lastShotTracer = std::chrono::steady_clock::now();
    bool lmbWasDown = false;

    // Lock-on smoothing: tracks when we first locked a target
    uintptr_t lockOnTarget = 0;
    auto lockOnTime = std::chrono::steady_clock::now();

    // Vis-check grace: stop aim/silent thrashing when LoS flickers
    uintptr_t visGraceTarget = 0;
    auto visGraceUntil = std::chrono::steady_clock::now();
    constexpr int kVisGraceMs = 350;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        // Sticky combat unlock after key auth (not killed by integrity noise)
        if (!Protect::LicensedHot())
        {
            AimLockData empty{};
            m_aimLock.Publish(std::move(empty));
            smoothCarry = {};
            stickyTarget = 0;
            SleepBudget(kTargetMs, timer.End());
            continue;
        }

        AimLockData lock{};
        const Config cfg = m_config.Get();
        const auto globals = m_globals.Read();
        const auto players = m_players.Read();
        const auto camera = m_camera.Read();

        (void)lastTracerTick;
        (void)lastShotTracer;

        auto keyHeld = [](int vk) -> bool {
            if (vk == 0) return true;
            return (GetAsyncKeyState(vk) & 0x8000) != 0;
        };

        const bool aimActive = cfg.aimEnabled
            && (cfg.aimAlwaysOn || cfg.aimKey == 0 || keyHeld(cfg.aimKey));
        const bool silentActive = cfg.silentEnabled
            && (cfg.silentAlwaysOn || cfg.silentKey == 0 || keyHeld(cfg.silentKey));
        const bool triggerSilent = cfg.triggerEnabled && cfg.silentEnabled && !cfg.menuOpen && (
            cfg.triggerAlwaysOn || cfg.triggerKey == 0 || keyHeld(cfg.triggerKey));

        const bool triggerHeld = cfg.triggerEnabled && !cfg.menuOpen && (
            cfg.triggerAlwaysOn || cfg.triggerKey == 0 || keyHeld(cfg.triggerKey));
        const bool needScan = aimActive || silentActive || triggerHeld;

        const bool useSilent = silentActive;
        // FOV circle follows free mouse (user can move it). Only overkill forces center.
        // Mouse *movement* still aims toward screen center so it stays smooth (see below).
        const bool useCenter = false;

        if (!aimActive && !silentActive)
        {
            if (!triggerHeld)
                stickyTarget = 0;
            smoothCarry = {};
        }

        Matrix4 silentView{};
        Vector2 silentViewport{};
        bool silentViewOk = false;
        if (silentActive || aimActive || cfg.silentEnabled || cfg.aimEnabled)
            silentViewOk = GetViewData(silentView, silentViewport);

        if ((camera && camera->valid) || silentViewOk)
        {
            const float screenW = silentViewOk ? silentViewport.x : camera->dimensionsX;
            const float screenH = silentViewOk ? silentViewport.y : camera->dimensionsY;
            if (cfg.silentEnabled && globals)
            {
                lock.cursorScreen = CursorInViewport(*globals, Vector2{ screenW, screenH });
                lock.useCenter = false;
                lock.silent = true;
                lock.fovRadius = cfg.silentFov;
            }
            else if (cfg.aimEnabled && camera && camera->valid && globals)
            {
                if (globals->overkillMode)
                {
                    lock.cursorScreen = { camera->dimensionsX * 0.5f, camera->dimensionsY * 0.5f };
                    lock.useCenter = true;
                }
                else
                {
                    // FOV circle tracks free mouse so user can move it
                    lock.cursorScreen = CursorInGame(*globals, *camera);
                    lock.useCenter = false;
                }
                lock.silent = false;
                lock.fovRadius = cfg.aimFov;
            }
        }

        int diagTotal = 0, diagLocal = 0, diagDead = 0, diagTeam = 0,
            diagWl = 0, diagDist = 0, diagPos = 0, diagProj = 0,
            diagVisBlock = 0, diagFov = 0, diagBest = 0;
        bool frameWantVis = false;
        int occValid = -1, occParts = 0;

        // Silent can run off view-matrix alone if camera frame is briefly invalid
        if (needScan && globals && players && players->valid
            && ((camera && camera->valid) || silentViewOk || GetViewData(silentView, silentViewport)))
        {
            if (!silentViewOk)
                silentViewOk = GetViewData(silentView, silentViewport);

            const auto occlusion = m_occlusion.Read();
            (void)occValid;
            (void)occParts;
            occValid = occlusion ? (int)occlusion->valid : -1;
            occParts = occlusion ? (int)occlusion->parts.size() : 0;

            float screenW = (camera && camera->valid) ? camera->dimensionsX : silentViewport.x;
            float screenH = (camera && camera->valid) ? camera->dimensionsY : silentViewport.y;
            Matrix4 viewForSelect = (camera && camera->valid) ? camera->viewMatrix : silentView;
            Vector2 cursor{};

            const bool overkillAim = globals->overkillMode;

            // Always prefer live VisualEngine view for selection
            if (silentViewOk)
            {
                screenW = silentViewport.x;
                screenH = silentViewport.y;
                viewForSelect = silentView;
            }

            auto refreshCursor = [&]() {
                // FOV / target pick follows free mouse (or center only in overkill)
                if (overkillAim)
                {
                    cursor = { screenW * 0.5f, screenH * 0.5f };
                    return;
                }
                HWND hwnd = globals->robloxHwnd
                    ? reinterpret_cast<HWND>(globals->robloxHwnd) : FindRobloxWindow();
                // Prefer OS cursor in VE space so FOV circle moves with the mouse
                cursor = CursorInViewport(*globals, Vector2{ screenW, screenH });
                if (cursor.x < 0.5f || cursor.y < 0.5f
                    || cursor.x > screenW + 80.f || cursor.y > screenH + 80.f)
                {
                    cursor = SilentAim::ReadMouseDimensions(
                        m_memory, *globals, Vector2{ screenW, screenH }, hwnd);
                }
            };

            if (silentActive)
            {
                if (!silentViewOk)
                {
                    lock.sequence = ++lockSeq;
                    m_aimLock.Publish(std::move(lock));
                    const double ms = timer.End();
                    m_timings.aim.Record(ms);
                    SleepBudget(kTargetMs, ms);
                    continue;
                }
                refreshCursor();
            }
            else
            {
                refreshCursor();
            }

            float fovLimit = (silentActive || triggerSilent) ? cfg.silentFov : cfg.aimFov;
            if (fovLimit < 8.f) fovLimit = 8.f;

            lock.active = true;
            lock.silent = silentActive;
            lock.useCenter = useCenter;
            lock.fovRadius = fovLimit;
            lock.cursorScreen = cursor;

            // Vis check only when the active aim mode wants it (not trigger-only forcing aim)
            const bool wantVis =
                (silentActive && cfg.silentVisibleCheck)
                || (aimActive && cfg.aimVisibleCheck);
            // Triggerbot has its own fire-time vis gate below
            const bool triggerWantsVis =
                triggerHeld && (
                    (silentActive && cfg.silentVisibleCheck)
                    || (aimActive && cfg.aimVisibleCheck)
                    || (!aimActive && !silentActive && (cfg.aimVisibleCheck || cfg.silentVisibleCheck)));
            frameWantVis = wantVis || triggerWantsVis;

            const float maxDist = silentActive ? cfg.silentDistance : cfg.aimDistance;
            const bool sticky = (aimActive || silentActive) && (
                silentActive ? cfg.silentSticky : cfg.aimSticky);
            const AimBone bone = silentActive ? cfg.silentBone : cfg.aimBone;
            float bestDist = fovLimit + 0.01f;

            const EntityData* best = nullptr;
            Vector2 bestScreen{};
            Vector3 bestWorld{};
            const auto nowVis = std::chrono::steady_clock::now();

            // Always sync live view + cursor to the SAME space before FOV scan
            Matrix4 liveView{};
            Vector2 liveVp{};
            if ((aimActive || silentActive) && GetViewData(liveView, liveVp)
                && liveVp.x > 1.f && liveVp.y > 1.f)
            {
                viewForSelect = liveView;
                screenW = liveVp.x;
                screenH = liveVp.y;
                refreshCursor();
            }

            auto predict = [&](const EntityData& e) -> Vector3 {
                Vector3 world;
                if (bone == AimBone::ClosestToCursor)
                    world = ClosestBoneToCursor(e, cursor, viewForSelect, screenW, screenH);
                else
                    world = AimPoint(e, bone);
                if (world.LengthSquared() < 0.01f && e.bones[static_cast<int>(BoneId::Head)].valid)
                    world = e.bones[static_cast<int>(BoneId::Head)].position;
                if (world.LengthSquared() < 0.01f)
                    world = e.rootPos;
                const bool predOn = silentActive ? cfg.silentPredictionEnabled : cfg.aimPredictionEnabled;
                const float pred = silentActive ? cfg.silentPrediction : cfg.aimPrediction;
                if (predOn && pred > 0.001f)
                    world = world + e.velocity * pred;
                return world;
            };

            auto closestInFov = [&](const EntityData& e, Vector2& outScreen, Vector3& outWorld, float& outDist) -> bool {
                const bool predOn = silentActive ? cfg.silentPredictionEnabled : cfg.aimPredictionEnabled;
                const float pred = (predOn ? (silentActive ? cfg.silentPrediction : cfg.aimPrediction) : 0.f);
                auto tryW = [&](Vector3 w) -> bool {
                    if (pred > 0.001f) w = w + e.velocity * pred;
                    if (w.LengthSquared() < 0.01f) return false;
                    Vector2 s{};
                    if (!AimWorldToScreen(w, s, viewForSelect, Vector2{ screenW, screenH }))
                        return false;
                    outScreen = s;
                    outWorld = w;
                    outDist = Distance2D(s, cursor);
                    return true;
                };

                if (bone == AimBone::ClosestToCursor)
                {
                    const BoneId candidates[] = {
                        BoneId::Head, BoneId::UpperTorso, BoneId::LowerTorso,
                        BoneId::LeftUpperArm, BoneId::LeftLowerArm,
                        BoneId::RightUpperArm, BoneId::RightLowerArm,
                        BoneId::LeftUpperLeg, BoneId::LeftLowerLeg, BoneId::LeftFoot,
                        BoneId::RightUpperLeg, BoneId::RightLowerLeg, BoneId::RightFoot
                    };
                    float bestD = 1e18f;
                    bool found = false;
                    for (BoneId bid : candidates)
                    {
                        const auto& bk = e.bones[static_cast<int>(bid)];
                        if (!bk.valid) continue;
                        Vector3 w = bk.position;
                        if (pred > 0.001f) w = w + e.velocity * pred;
                        if (w.LengthSquared() < 0.01f) continue;
                        Vector2 s{};
                        if (!AimWorldToScreen(w, s, viewForSelect, Vector2{ screenW, screenH }))
                            continue;
                        float d = Distance2D(s, cursor);
                        if (d < bestD) { bestD = d; outScreen = s; outWorld = w; outDist = d; found = true; }
                    }
                    if (found) return true;
                    if (tryW(AimPoint(e, AimBone::Head))) return true;
                    if (e.rootPos.LengthSquared() > 0.01f && tryW(e.rootPos)) return true;
                    return false;
                }

                if (tryW(AimPoint(e, bone))) return true;
                if (e.bones[static_cast<int>(BoneId::Head)].valid
                    && tryW(e.bones[static_cast<int>(BoneId::Head)].position))
                    return true;
                if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid
                    && tryW(e.bones[static_cast<int>(BoneId::UpperTorso)].position))
                    return true;
                if (e.rootPos.LengthSquared() > 0.01f && tryW(e.rootPos))
                    return true;
                return false;
            };

            // Any common hit bone visible (less flicker than head-only)
            auto entityVisibleAny = [&](const EntityData& e) -> bool {
                if (!camera || !camera->valid) return true;
                auto test = [&](const Vector3& p) -> bool {
                    if (p.LengthSquared() < 0.01f) return false;
                    return HasLineOfSight(*camera, p, occlusion.get(), e.address);
                };
                if (e.bones[static_cast<int>(BoneId::Head)].valid
                    && test(e.bones[static_cast<int>(BoneId::Head)].position))
                    return true;
                if (e.bones[static_cast<int>(BoneId::UpperTorso)].valid
                    && test(e.bones[static_cast<int>(BoneId::UpperTorso)].position))
                    return true;
                if (e.bones[static_cast<int>(BoneId::LowerTorso)].valid
                    && test(e.bones[static_cast<int>(BoneId::LowerTorso)].position))
                    return true;
                if (e.rootPos.LengthSquared() > 0.01f && test(e.rootPos))
                    return true;
                return false;
            };

            uintptr_t localTeam = 0;
            for (const auto& e : players->entities)
                if (e.isLocal) { localTeam = e.team; break; }

            const bool teamCheck = silentActive ? cfg.silentTeamCheck : cfg.aimTeamCheck;
            const bool healthCheck = silentActive ? cfg.silentHealthCheck : cfg.aimHealthCheck;
            const float trigPred = 0.08f;

            const EntityData* stickyEnt = nullptr;
            Vector2 stickyScreen{};
            Vector3 stickyWorld{};
            float stickyDist = 1e9f;

            for (const auto& e : players->entities)
            {
                diagTotal++;
                if (e.isLocal) { diagLocal++; continue; }
                if (e.isNpc) continue; // ESP only — don't aim at NPCs
                if (!e.hasCharacter && e.rootPos.LengthSquared() < 0.01f && e.validBones <= 0) { diagPos++; continue; }
                if (e.validBones <= 0 && e.rootPos.LengthSquared() < 0.01f) { diagPos++; continue; }

                if (!overkillAim)
                {
                    if (e.health <= 0.f && e.maxHealth > 0.f) { diagDead++; continue; }
                }

                if (healthCheck && !overkillAim && e.health <= 0.f && e.maxHealth > 1.f) { diagDead++; continue; }
                if (teamCheck && localTeam && e.team && e.team == localTeam) { diagTeam++; continue; }

                if (ConfigIsWhitelisted(cfg, e.name) || ConfigIsWhitelisted(cfg, e.displayName)) { diagWl++; continue; }

                Vector3 camPos = (camera && camera->valid) ? camera->position : Vector3{};
                // estimate cam pos from view if needed later; root-distance still fine
                const float worldDist = camPos.LengthSquared() > 0.01f
                    ? (e.rootPos - camPos).Length()
                    : 50.f;

                if (camPos.LengthSquared() > 0.01f && worldDist < 1.5f) { diagDist++; continue; }
                if (maxDist > 0.f && worldDist > maxDist) { diagDist++; continue; }
                if (e.rootPos.y < -500.f || e.rootPos.y > 5000.f) { diagDist++; continue; }

                Vector2 screen{};
                Vector3 world{};
                float dist = 1e9f;

                if (useSilent || aimActive || triggerHeld)
                {
                    if (!closestInFov(e, screen, world, dist))
                        { diagProj++; continue; }
                }
                else
                {
                    world = predict(e);
                    if (!camera || !WorldToScreen(world, camera->viewMatrix, screenW, screenH, screen))
                        { diagProj++; continue; }
                    dist = Distance2D(screen, cursor);
                }

                // Visible check with multi-bone + sticky grace (stops aim glitching)
                if (wantVis && camera && camera->valid)
                {
                    bool vis = entityVisibleAny(e);
                    if (vis)
                    {
                        if (e.address == stickyTarget || e.address == visGraceTarget)
                        {
                            visGraceTarget = e.address;
                            visGraceUntil = nowVis + std::chrono::milliseconds(kVisGraceMs);
                        }
                    }
                    else
                    {
                        // Keep sticky / recent target through short LoS flicker
                        const bool inGrace =
                            (e.address == stickyTarget || e.address == visGraceTarget)
                            && nowVis < visGraceUntil;
                        if (!inGrace)
                        {
                            diagVisBlock++;
                            continue;
                        }
                    }
                }

                if (dist <= fovLimit && dist < bestDist)
                {
                    bestDist = dist;
                    best = &e;
                    bestScreen = screen;
                    bestWorld = world;
                    diagBest++;
                }
                else
                {
                    diagFov++;
                }

                if (sticky && stickyTarget && e.address == stickyTarget && dist <= fovLimit * 1.85f)
                {
                    stickyEnt = &e;
                    stickyScreen = screen;
                    stickyWorld = world;
                    stickyDist = dist;
                }
            }

            // Sticky: hard lock current target until they leave expanded FOV
            // Only switch if new target is MUCH closer (half the distance) or current is dead
            if (sticky && stickyEnt)
            {
                const bool keepSticky = !best
                    || stickyEnt->address == best->address
                    || bestDist > stickyDist * 0.55f
                    || stickyDist <= fovLimit * 1.6f;
                if (keepSticky)
                {
                    best = stickyEnt;
                    bestScreen = stickyScreen;
                    bestWorld = stickyWorld;
                    bestDist = stickyDist;
                }
            }
            else if (!best)
            {
                if (nowVis >= visGraceUntil)
                {
                    stickyTarget = 0;
                    visGraceTarget = 0;
                    hitTrackAddr = 0;
                    hitTrackHp = -1.f;
                }
            }

            if (best)
            {
                stickyTarget = best->address;
                lock.locked = true;
                lock.targetScreen = bestScreen;
                lock.targetWorld = bestWorld;
                lock.targetPlayer = best->address;
                lock.useCenter = overkillAim; // FOV stays on free mouse unless overkill

                if (cfg.notificationsEnabled)
                {
                    if (best->address == hitTrackAddr && hitTrackHp > 0.f
                        && best->health >= 0.f && best->health < hitTrackHp - 0.4f)
                    {
                        SignalHit();
                    }
                    hitTrackAddr = best->address;
                    hitTrackHp = best->health;
                }

                if ((silentActive || (triggerHeld && cfg.silentEnabled)) && globals)
                {
                    ApplySilentAim(*globals, bestWorld);
                    ApplySilentAim(*globals, bestWorld);
                    static uintptr_t lastSilentTarget = 0;
                    if (best->address != lastSilentTarget)
                    {
                        lastSilentTarget = best->address;
                        SignalLock();
                    }
                }

                if (triggerHeld && (aimActive || silentActive || cfg.silentEnabled) && bestDist <= fovLimit)
                {
                    bool canShoot = true;
                    if (camera && camera->valid)
                        canShoot = entityVisibleAny(*best);

                    const int clickGapMs = (aimActive && cfg.aimMethod == AimMethod::Mouse) ? 90 : 55;
                    const auto now = std::chrono::steady_clock::now();
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - lastTriggerClick).count();
                    if (canShoot && elapsed >= clickGapMs)
                    {
                        ClickMouse(16.f);
                        lastTriggerClick = now;
                        visGraceTarget = best->address;
                        visGraceUntil = now + std::chrono::milliseconds(kVisGraceMs);
                    }
                }

                // Aimbot
                if (aimActive && !silentActive)
                {
                    float smooth = cfg.aimSmooth;
                    if (smooth < 0.15f) smooth = 0.15f;
                    if (smooth > 40.f) smooth = 40.f;
                    float sens = cfg.aimSensitivity;
                    if (sens < 0.15f) sens = 0.15f;
                    if (sens > 3.f) sens = 3.f;

                    if (cfg.aimMethod == AimMethod::Camera)
                    {
                        uintptr_t camInst = camera ? camera->camera : 0;
                        if (globals->workspace && Offsets::Workspace::CurrentCamera)
                        {
                            const uintptr_t live = m_memory.Read<uintptr_t>(
                                globals->workspace + Offsets::Workspace::CurrentCamera);
                            if (live) camInst = live;
                        }

                        if (camInst && Offsets::Camera::Rotation && Offsets::Camera::Position)
                        {
                            // Gentle blend — hard snaps glitch against Roblox camera scripts
                            float blend = 0.45f * sens;
                            if (cfg.smoothMethod != SmoothMethod::None)
                            {
                                blend = (sens / smooth) * 0.65f;
                                if (blend > 0.55f) blend = 0.55f;
                                if (blend < 0.10f) blend = 0.10f;
                            }
                            else
                            {
                                if (blend > 0.70f) blend = 0.70f;
                            }

                            Vector3 from = m_memory.Read<Vector3>(camInst + Offsets::Camera::Position);
                            if (from.LengthSquared() < 0.01f && camera && camera->valid)
                                from = camera->position;

                            Matrix3 curRot{};
                            if (!m_memory.ReadRaw(camInst + Offsets::Camera::Rotation, curRot.m, sizeof(curRot.m)))
                            {
                                if (camera) curRot = camera->rotation;
                            }

                            Vector3 toDir = Normalize(bestWorld - from);
                            if (toDir.LengthSquared() < 1e-6f)
                                toDir = { 0.f, 0.f, -1.f };

                            Vector3 curForward{ -curRot.m[2], -curRot.m[5], -curRot.m[8] };
                            curForward = Normalize(curForward);
                            if (curForward.LengthSquared() < 1e-6f)
                                curForward = toDir;

                            // Don't re-aim if almost looking at target (stops camera shake)
                            const float align = Dot(curForward, toDir);
                            if (align < 0.9995f)
                            {
                                Vector3 blended = Normalize(curForward * (1.f - blend) + toDir * blend);
                                const Matrix3 rot = LookRotation(from, from + blended * 100.f);
                                m_memory.WriteRaw(camInst + Offsets::Camera::Rotation, rot.m, sizeof(rot.m));
                            }
                        }
                    }
                    else
                    {
                        // Mouse aim move: pull target toward SCREEN CENTER (smooth, no glitch).
                        // FOV circle still follows free mouse for picking — separate from this.
                        Matrix4 viewM{};
                        Vector2 veSize{};
                        Vector2 targetVe = bestScreen;
                        if (GetViewData(viewM, veSize) && veSize.x > 1.f && veSize.y > 1.f)
                        {
                            Vector2 reproj{};
                            if (AimWorldToScreen(bestWorld, reproj, viewM, veSize))
                                targetVe = reproj;
                        }
                        else
                        {
                            veSize = { screenW, screenH };
                        }
                        if (veSize.x < 1.f) veSize.x = screenW > 1.f ? screenW : 1920.f;
                        if (veSize.y < 1.f) veSize.y = screenH > 1.f ? screenH : 1080.f;

                        const float cx = veSize.x * 0.5f;
                        const float cy = veSize.y * 0.5f;
                        float dx = targetVe.x - cx;
                        float dy = targetVe.y - cy;
                        float dist = std::sqrt(dx * dx + dy * dy);

                        constexpr float kDead = 4.5f;
                        if (dist <= kDead)
                        {
                            smoothCarry = {};
                        }
                        else
                        {
                            float stepScale = 1.f;
                            if (cfg.smoothMethod == SmoothMethod::None)
                                stepScale = (std::min)(1.f, 0.32f * sens);
                            else if (cfg.smoothMethod == SmoothMethod::Linear)
                                stepScale = sens / (smooth + 0.55f);
                            else if (cfg.smoothMethod == SmoothMethod::Exponential)
                            {
                                float expo = cfg.aimExpo;
                                if (expo < 0.5f) expo = 0.5f;
                                stepScale = 1.f - std::exp(-(expo * 0.035f * sens));
                            }
                            else if (cfg.smoothMethod == SmoothMethod::Constant)
                            {
                                float spd = cfg.aimConstantSpeed * sens;
                                if (spd < 1.f) spd = 1.f;
                                stepScale = (std::min)(1.f, spd / dist);
                            }
                            else
                            {
                                float t = sens / (smooth + 0.55f);
                                if (t > 1.f) t = 1.f;
                                if (t < 0.08f) t = 0.08f;
                                stepScale = t * t * (3.f - 2.f * t);
                            }
                            if (stepScale < 0.06f) stepScale = 0.06f;
                            if (stepScale > 0.85f) stepScale = 0.85f;

                            if (cfg.smoothMethod == SmoothMethod::LockOn && lock.locked)
                            {
                                if (lock.targetPlayer != lockOnTarget)
                                {
                                    lockOnTarget = lock.targetPlayer;
                                    lockOnTime = std::chrono::steady_clock::now();
                                }
                                const float delayMs = cfg.lockOnDelay;
                                if (delayMs > 10.f)
                                {
                                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - lockOnTime).count();
                                    if (elapsed < delayMs)
                                    {
                                        const float ramp = 1.f - (float)elapsed / delayMs;
                                        const float factor = cfg.lockOnStrength * ramp;
                                        stepScale *= 1.f / (1.f + factor);
                                        if (stepScale < 0.02f) stepScale = 0.02f;
                                    }
                                }
                            }

                            dx *= stepScale;
                            dy *= stepScale;

                            const float maxStep = 18.f + 25.f * sens;
                            const float stepLen = std::sqrt(dx * dx + dy * dy);
                            if (stepLen > maxStep && stepLen > 0.01f)
                            {
                                const float s = maxStep / stepLen;
                                dx *= s;
                                dy *= s;
                            }

                            smoothCarry.x += dx;
                            smoothCarry.y += dy;
                            int mx = (int)std::lround(smoothCarry.x);
                            int my = (int)std::lround(smoothCarry.y);
                            smoothCarry.x -= (float)mx;
                            smoothCarry.y -= (float)my;

                            if (mx == 0 && my == 0 && dist > 10.f)
                            {
                                if (std::fabs(dx) >= std::fabs(dy))
                                    mx = (dx >= 0.f) ? 1 : -1;
                                else
                                    my = (dy >= 0.f) ? 1 : -1;
                            }

                            if (mx != 0 || my != 0)
                                MoveMouseRelative(mx, my);
                        }
                    }
                }
            }
            else
            {
                smoothCarry = {};
                lockOnTarget = 0;
            }

        }
        else
        {
            smoothCarry = {};
            lockOnTarget = 0;
        }

        lock.sequence = ++lockSeq;
        m_aimLock.Publish(std::move(lock));

        (void)lmbWasDown;

        const double ms = timer.End();
        m_timings.aim.Record(ms);
        SleepBudget(kTargetMs, ms);
    }
}
