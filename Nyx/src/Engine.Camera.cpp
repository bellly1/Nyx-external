#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "Config.hpp"
#include "SilentAim.hpp"
#include <chrono>
#include <string>

bool Engine::GetViewData(Matrix4& view, Vector2& viewport) const
{
    uintptr_t base = 0;
    if (const auto g = m_globals.Read())
        base = g->moduleBase;
    if (!base)
        base = m_memory.GetModuleBase(L"RobloxPlayerBeta.exe");
    return SilentAim::GetViewData(m_memory, base, view, viewport);
}

void Engine::CameraThread()
{
    constexpr double kTargetMs = 1.0;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        CameraData cam{};
        const auto globals = m_globals.Read();
        const Config cfg = m_config.Get();
        if (globals && globals->valid && globals->moduleBase)
        {
            cam.visualEngine = m_memory.Read<uintptr_t>(globals->moduleBase + Offsets::VisualEngine::Pointer);
            cam.camera = globals->currentCamera;

            if (globals->workspace)
            {
                const uintptr_t liveCam = m_memory.Read<uintptr_t>(globals->workspace + Offsets::Workspace::CurrentCamera);
                if (liveCam) cam.camera = liveCam;
            }

            if (cam.visualEngine)
            {
                m_memory.ReadRaw(cam.visualEngine + Offsets::VisualEngine::ViewMatrix, cam.viewMatrix.m, sizeof(cam.viewMatrix.m));
                const Vector2 dims = m_memory.Read<Vector2>(cam.visualEngine + Offsets::VisualEngine::Dimensions);
                cam.dimensionsX = dims.x;
                cam.dimensionsY = dims.y;
            }

            if (cam.camera)
            {
                cam.position = m_memory.Read<Vector3>(cam.camera + Offsets::Camera::Position);
                m_memory.ReadRaw(cam.camera + Offsets::Camera::Rotation, cam.rotation.m, sizeof(cam.rotation.m));
                if (cfg.camFovEnabled)
                    m_memory.Write<float>(cam.camera + Offsets::Camera::FieldOfView, cfg.camFovAmount);
                cam.fov = m_memory.Read<float>(cam.camera + Offsets::Camera::FieldOfView);
            }

            cam.valid = cam.visualEngine != 0 && cam.dimensionsX > 1.f && cam.dimensionsY > 1.f;
        }

        if (cam.valid)
        {
            cam.sequence = m_seqCamera.fetch_add(1, std::memory_order_relaxed) + 1;
            m_camera.Publish(std::move(cam));
        }

        const double ms = timer.End();
        m_timings.camera.Record(ms);
        SleepBudget(kTargetMs, ms);
    }
}
