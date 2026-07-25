#include "Engine.hpp"
#include "Offsets.hpp"
#include "Config.hpp"
#include "Protect.hpp"
#include "Notify.hpp"
#include <chrono>
#include <thread>
#include <cmath>

void Engine::TriggerbotThread()
{
    auto lastShotTime = std::chrono::steady_clock::now();

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        if (!Protect::LicensedHot())
        {
            SleepBudget(50.0, timer.End());
            continue;
        }

        const Config cfg = m_config.Get();
        if (!cfg.triggerEnabled || cfg.menuOpen)
        {
            SleepBudget(50.0, timer.End());
            continue;
        }

        const bool aimOrSilentActive =
            (cfg.aimEnabled && (cfg.aimAlwaysOn || cfg.aimKey == 0 || (GetAsyncKeyState(cfg.aimKey) & 0x8000)))
            || cfg.silentEnabled;
        if (aimOrSilentActive)
        {
            SleepBudget(5.0, timer.End());
            continue;
        }

        const bool keyHeld = cfg.triggerAlwaysOn ||
            (cfg.triggerKey != 0 && (GetAsyncKeyState(cfg.triggerKey) & 0x8000) != 0);

        if (!keyHeld)
        {
            SleepBudget(5.0, timer.End());
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShotTime).count();
        if (elapsed < (long long)cfg.triggerDelay)
        {
            SleepBudget(1.0, timer.End());
            continue;
        }

        auto g = m_globals.Read();
        auto cam = m_camera.Read();
        auto pl = m_players.Read();
        auto occ = m_occlusion.Read();
        if (!g || !g->valid || !cam || !cam->valid || !pl || !pl->valid)
        {
            SleepBudget(50.0, timer.End());
            continue;
        }

        POINT pt{};
        GetCursorPos(&pt);
        HWND robloxHwnd = g->robloxHwnd ? reinterpret_cast<HWND>(g->robloxHwnd) : nullptr;
        if (robloxHwnd)
            ScreenToClient(robloxHwnd, &pt);

        const float cx = (float)pt.x;
        const float cy = (float)pt.y;
        const float rangeSq = cfg.triggerRange * cfg.triggerRange;
        const float sx = (cam->dimensionsX > 1.f) ? cam->dimensionsX : 1.f;
        const float sy = (cam->dimensionsY > 1.f) ? cam->dimensionsY : 1.f;

        float bestDistSq = rangeSq;
        int bestIdx = -1;
        Vector3 bestWorld{};

        for (int i = 0; i < (int)pl->entities.size(); ++i)
        {
            const auto& e = pl->entities[i];
            if (!e.hasCharacter || e.validBones < 1) continue;
            if (e.isLocal) continue;
            if (e.maxHealth > 1.f && e.health <= 0.f) continue;

            for (int bi = 0; bi < kBoneCount; ++bi)
            {
                if (!e.bones[bi].valid) continue;
                Vector2 sp{};
                if (!WorldToScreen(e.bones[bi].position, cam->viewMatrix, sx, sy, sp)) continue;
                float dx = sp.x - cx;
                float dy = sp.y - cy;
                float dSq = dx * dx + dy * dy;
                if (dSq < bestDistSq)
                {
                    bestDistSq = dSq;
                    bestIdx = i;
                    bestWorld = e.bones[bi].position;
                }
                break;
            }
        }

        if (bestIdx >= 0)
        {
            const auto& best = pl->entities[bestIdx];

            if (cfg.triggerVisCheck || cfg.aimVisibleCheck || cfg.silentVisibleCheck)
            {
                bool vis = false;
                if (best.bones[static_cast<int>(BoneId::Head)].valid
                    && HasLineOfSight(*cam, best.bones[static_cast<int>(BoneId::Head)].position, occ.get(), best.address))
                    vis = true;
                else if (best.bones[static_cast<int>(BoneId::UpperTorso)].valid
                    && HasLineOfSight(*cam, best.bones[static_cast<int>(BoneId::UpperTorso)].position, occ.get(), best.address))
                    vis = true;
                else if (best.bones[static_cast<int>(BoneId::LowerTorso)].valid
                    && HasLineOfSight(*cam, best.bones[static_cast<int>(BoneId::LowerTorso)].position, occ.get(), best.address))
                    vis = true;
                else if (best.rootPos.LengthSquared() > 0.01f
                    && HasLineOfSight(*cam, best.rootPos, occ.get(), best.address))
                    vis = true;

                if (!vis)
                {
                    SleepBudget(1.0, timer.End());
                    continue;
                }
            }

            if (cfg.silentEnabled)
            {
                for (int f = 0; f < 6; ++f)
                {
                    ApplySilentAim(*g, bestWorld);
                    Sleep(3);
                }
            }

            ClickMouse(16.f);
            lastShotTime = std::chrono::steady_clock::now();
        }
        else
        {
            SleepBudget(2.0, timer.End());
        }
    }
}
