#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include "Config.hpp"
#include "Protect.hpp"
#include <algorithm>
#include <chrono>
#include <string>

uintptr_t Engine::ResolveLighting() const
{
    if (const auto g = m_globals.Read())
    {
        if (g->lighting) return g->lighting;
        if (g->dataModel)
        {
            for (const uintptr_t child : ReadChildren(g->dataModel))
            {
                if (ReadClassName(child) == "Lighting" || ReadName(child) == "Lighting")
                    return child;
            }
        }
    }
    return 0;
}

void Engine::ForceWorldLighting()
{
    const Config cfg = m_config.Get();
    if (!cfg.worldAmbienceEnabled && !cfg.worldFogEnabled && !cfg.worldBrightnessEnabled && !cfg.skyboxEnabled)
        return;
    uintptr_t lighting = ResolveLighting();
    if (!lighting)
    {
        if (const auto g = m_globals.Read())
            lighting = g->lighting;
    }
    if (lighting)
        ApplyWorldLighting(cfg, lighting);
}

void Engine::ApplyWorldLighting(const Config& cfg, uintptr_t lighting)
{
    if (!lighting) return;

    auto clamp01 = [](float v) -> float {
        if (v != v) return 0.f;
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    };

    // Hard defaults (imtheo) if FetchOffsets missed a field
    const uintptr_t offAmbient  = Offsets::Lighting::Ambient ? Offsets::Lighting::Ambient : 0xC8;
    const uintptr_t offOutdoor  = Offsets::Lighting::OutdoorAmbient ? Offsets::Lighting::OutdoorAmbient : 0xF8;
    const uintptr_t offBright   = Offsets::Lighting::Brightness ? Offsets::Lighting::Brightness : 0x110;
    const uintptr_t offFogCol   = Offsets::Lighting::FogColor ? Offsets::Lighting::FogColor : 0xEC;
    const uintptr_t offFogEnd   = Offsets::Lighting::FogEnd ? Offsets::Lighting::FogEnd : 0x124;
    const uintptr_t offFogStart = Offsets::Lighting::FogStart ? Offsets::Lighting::FogStart : 0x128;
    const uintptr_t offExposure = Offsets::Lighting::ExposureCompensation ? Offsets::Lighting::ExposureCompensation : 0x11C;
    const uintptr_t offEnvDiff  = Offsets::Lighting::EnvironmentDiffuseScale ? Offsets::Lighting::EnvironmentDiffuseScale : 0x114;
    const uintptr_t offEnvSpec  = Offsets::Lighting::EnvironmentSpecularScale ? Offsets::Lighting::EnvironmentSpecularScale : 0x118;

    auto writeCol = [&](uintptr_t base, uintptr_t off, float r, float g, float b) {
        if (!base || !off) return;
        const Vector3 c{ clamp01(r), clamp01(g), clamp01(b) };
        m_memory.Write<Vector3>(base + off, c);
    };
    auto writeF = [&](uintptr_t base, uintptr_t off, float v) {
        if (!base || !off) return;
        m_memory.Write<float>(base + off, v);
    };

    if (cfg.worldAmbienceEnabled)
    {
        writeCol(lighting, offAmbient, cfg.worldAmbient.r, cfg.worldAmbient.g, cfg.worldAmbient.b);
        writeCol(lighting, offOutdoor, cfg.worldAmbient.r, cfg.worldAmbient.g, cfg.worldAmbient.b);
    }

    if (cfg.worldFogEnabled)
    {
        writeCol(lighting, offFogCol, cfg.worldFogColor.r, cfg.worldFogColor.g, cfg.worldFogColor.b);
        float fs = cfg.worldFogStart;
        float fe = cfg.worldFogEnd;
        if (fs < 0.f) fs = 0.f;
        if (fe < fs + 10.f) fe = fs + 10.f;
        writeF(lighting, offFogStart, fs);
        writeF(lighting, offFogEnd, fe);
    }

    if (cfg.worldBrightnessEnabled)
    {
        float bright = cfg.worldBrightness;
        if (bright < 0.f) bright = 0.f;
        if (bright > 20.f) bright = 20.f;
        writeF(lighting, offBright, bright);
        writeF(lighting, offExposure, cfg.worldExposure);
        writeF(lighting, offEnvDiff, cfg.worldIntensity);
        writeF(lighting, offEnvSpec, cfg.worldIntensity * 0.85f);
    }

    // --- ClockTime (time changer) ---
    if (cfg.worldTimeEnabled)
    {
        const uintptr_t offClock = Offsets::Lighting::ClockTime ? Offsets::Lighting::ClockTime : 0x1a8;
        float t = cfg.worldTimeOfDay;
        if (t < 0.f) t = 0.f;
        if (t > 24.f) t = 24.f;
        writeF(lighting, offClock, t);
    }

    // --- Skybox face writes ---
    const bool skyOffsetsOk =
        Offsets::Sky::SkyboxBk && Offsets::Sky::SkyboxDn &&
        Offsets::Sky::SkyboxFt && Offsets::Sky::SkyboxLf &&
        Offsets::Sky::SkyboxRt && Offsets::Sky::SkyboxUp;

    if (cfg.skyboxEnabled && skyOffsetsOk)
    {
        static constexpr const char* kSkyboxPresets[5][6] = {
            { "", "", "", "", "", "" }, // 0: Default
            { "rbxassetid://15983968922", "rbxassetid://15983966825", "rbxassetid://15983965025",
              "rbxassetid://15983967420", "rbxassetid://15983966246", "rbxassetid://15983964246" }, // 1: Arctic
            { "rbxassetid://159454299", "rbxassetid://159454296", "rbxassetid://159454293",
              "rbxassetid://159454286", "rbxassetid://159454288", "rbxassetid://159454300" }, // 2: Nebula
            { "rbxassetid://144933338", "rbxassetid://144931530", "rbxassetid://144933262",
              "rbxassetid://144933244", "rbxassetid://144933299", "rbxassetid://144931564" }, // 3: Realistic
            { "rbxassetid://17359299523", "rbxassetid://17359302440", "rbxassetid://17359305344",
              "rbxassetid://17359309400", "rbxassetid://17359311050", "rbxassetid://17359315951" }, // 4: Sunset
        };

        // Find Sky instance under Lighting
        uintptr_t skyInstance = 0;
        const uintptr_t skyContainer = m_memory.Read<uintptr_t>(lighting + Offsets::Instance::ChildrenStart);
        if (skyContainer)
        {
            const uintptr_t sStart = m_memory.Read<uintptr_t>(skyContainer);
            const uintptr_t sEnd = m_memory.Read<uintptr_t>(skyContainer + Offsets::Instance::ChildrenEnd);
            if (sStart && sEnd && sEnd >= sStart)
            {
                for (uintptr_t n = sStart; n < sEnd; n += 0x10)
                {
                    const uintptr_t child = m_memory.Read<uintptr_t>(n);
                    if (!child) continue;
                    const uintptr_t desc = m_memory.Read<uintptr_t>(child + Offsets::Instance::ClassDescriptor);
                    if (!desc) continue;
                    const std::string cls = ReadClassName(child);
                    if (cls == "Sky")
                    {
                        skyInstance = child;
                        break;
                    }
                }
            }
        }

        if (skyInstance)
        {
            const int presetIdx = std::clamp(cfg.skyboxPreset, 0, 4);
            const auto& faces = kSkyboxPresets[presetIdx];
            const uintptr_t faceOffsets[] = {
                Offsets::Sky::SkyboxBk, Offsets::Sky::SkyboxDn, Offsets::Sky::SkyboxFt,
                Offsets::Sky::SkyboxLf, Offsets::Sky::SkyboxRt, Offsets::Sky::SkyboxUp
            };
            for (int i = 0; i < 6; ++i)
            {
                const uintptr_t addr = skyInstance + faceOffsets[i];
                if (faces[i][0] != '\0')
                    m_memory.WriteString(addr, faces[i]);
            }
            if (Offsets::Sky::StarCount)
            {
                const uintptr_t starAddr = skyInstance + Offsets::Sky::StarCount;
                m_memory.Write<int>(starAddr, cfg.skyStarCount);
            }
        }
    }
}

void Engine::WorldThread()
{
    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        double targetMs = 40.0;
        if (Protect::LicensedHot() || Protect::CombatUnlocked().load(std::memory_order_relaxed))
        {
            const Config cfg = m_config.Get();
            if (cfg.worldAmbienceEnabled || cfg.worldFogEnabled || cfg.worldBrightnessEnabled || cfg.skyboxEnabled || cfg.worldTimeEnabled)
            {
                targetMs = 16.0; // ~60Hz sticky so games that reset Lighting still stick
                if (const uintptr_t lighting = ResolveLighting())
                    ApplyWorldLighting(cfg, lighting);
            }
        }

        SleepBudget(targetMs, timer.End());
    }
}
