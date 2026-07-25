#pragma once

#include "Math.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "GameData.hpp"

#include <Windows.h>
#include <cstdint>
#include <cmath>

namespace SilentAim
{

// Matches working original: rbx::camera_t::CalculateViewport
#pragma pack(push, 1)
struct ViewportI16
{
    int16_t x = 0;
    int16_t y = 0;
};
#pragma pack(pop)

inline ViewportI16 CalculateViewport(
    const Vector2& TargetPosition,
    const Vector2& Size,
    const Vector2& MousePosition)
{
    ViewportI16 Result{};

    double TargetY = static_cast<double>(TargetPosition.y);
    if (TargetY < 1.0) TargetY = 1.0;
    if (TargetY > static_cast<double>(Size.y) - 1.0)
        TargetY = static_cast<double>(Size.y) - 1.0;

    double Ratio = static_cast<double>(MousePosition.y) / TargetY;
    double VY = static_cast<double>(Size.y) * Ratio;
    if (VY > 32767.0) VY = 32767.0;
    if (VY < 1.0) VY = 1.0;

    Ratio = VY / static_cast<double>(Size.y);
    double VX = 2.0 * static_cast<double>(MousePosition.x)
        - Ratio * (2.0 * static_cast<double>(TargetPosition.x) - static_cast<double>(Size.x));
    if (VX > 32767.0) VX = 32767.0;
    if (VX < 1.0) VX = 1.0;

    Result.x = static_cast<int16_t>(std::round(VX));
    Result.y = static_cast<int16_t>(std::round(VY));
    return Result;
}

inline bool GetViewData(Memory& mem, uintptr_t moduleBase, Matrix4& view, Vector2& viewport)
{
    if (!moduleBase)
        return false;
    if (!Offsets::VisualEngine::Pointer)
        return false;

    const uintptr_t ve = mem.Read<uintptr_t>(moduleBase + Offsets::VisualEngine::Pointer);
    if (!ve)
        return false;

    if (!mem.ReadRaw(ve + Offsets::VisualEngine::ViewMatrix, &view, sizeof(view)))
        return false;
    if (!mem.ReadRaw(ve + Offsets::VisualEngine::Dimensions, &viewport, sizeof(viewport)))
        return false;
    if (viewport.x <= 0.0f || viewport.y <= 0.0f)
        return false;

    return true;
}

// Matches working original AimWorldToScreen
inline bool WorldToScreen(
    const Vector3& world,
    Vector2& out,
    const Matrix4& view,
    const Vector2& viewport)
{
    const float* m = view.m;
    float w_x = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];
    if (w_x < 0.01f)
        return false;

    float screen_x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
    float screen_y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
    float inv_w = 1.0f / w_x;
    out.x = (viewport.x * 0.5f * screen_x * inv_w) + (viewport.x * 0.5f);
    out.y = -(viewport.y * 0.5f * screen_y * inv_w) + (viewport.y * 0.5f);
    if (out.x != out.x || out.y != out.y)
        return false;
    return true;
}

inline Vector2 ReadMouseDimensions(Memory& mem, const GlobalsData& g, const Vector2& dimensions, HWND robloxHwnd)
{
    // Prefer in-game mouse if available (matches what roblox uses for raycasts)
    if (g.mouseService && Offsets::MouseService::MousePosition)
    {
        Vector2 mp = mem.Read<Vector2>(g.mouseService + Offsets::MouseService::MousePosition);
        if (mp.x > 0.5f && mp.y > 0.5f && mp.x < dimensions.x + 50.f && mp.y < dimensions.y + 50.f)
            return mp;
    }

    if (g.overkillMode && dimensions.x > 1.f && dimensions.y > 1.f)
        return { dimensions.x * 0.5f, dimensions.y * 0.5f };

    POINT pt{};
    GetCursorPos(&pt);

    HWND hwnd = robloxHwnd;
    if (!hwnd && g.robloxHwnd)
        hwnd = reinterpret_cast<HWND>(g.robloxHwnd);

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
        if (cw > 1.f && ch > 1.f && dimensions.x > 0.f && dimensions.y > 0.f)
        {
            x *= dimensions.x / cw;
            y *= dimensions.y / ch;
        }
        if (x < 1.f) x = 1.f;
        if (y < 1.f) y = 1.f;
        if (x > dimensions.x - 1.f) x = dimensions.x - 1.f;
        if (y > dimensions.y - 1.f) y = dimensions.y - 1.f;
        return { x, y };
    }

    if (dimensions.x > 0.f && dimensions.y > 0.f)
        return { dimensions.x * 0.5f, dimensions.y * 0.5f };
    return { 1.f, 1.f };
}

// Pure original silent: W2S target -> CalculateViewport(mouse) -> write Camera.Viewport
inline bool Apply(Memory& mem, const GlobalsData& g, const Vector3& worldPos, HWND robloxHwnd = nullptr)
{
    if (!Offsets::VisualEngine::Pointer || !Offsets::Camera::Viewport)
        return false;

    uintptr_t base = g.moduleBase;
    if (!base)
        return false;

    Matrix4 view{};
    Vector2 size{};
    if (!GetViewData(mem, base, view, size))
        return false;

    Vector2 targetScreen{};
    if (!WorldToScreen(worldPos, targetScreen, view, size))
        return false;

    const Vector2 mouse = ReadMouseDimensions(mem, g, size, robloxHwnd);
    const ViewportI16 vp = CalculateViewport(targetScreen, size, mouse);

    uintptr_t camera = 0;
    if (g.workspace)
        camera = mem.Read<uintptr_t>(g.workspace + Offsets::Workspace::CurrentCamera);
    if (!camera)
        camera = g.currentCamera;
    if (!camera)
        return false;

    // Write int16 pair exactly as original
    return mem.Write<ViewportI16>(camera + Offsets::Camera::Viewport, vp);
}

// Call every tick while locked for max reliability
inline bool ApplyLoop(Memory& mem, const GlobalsData& g, const Vector3& worldPos, HWND robloxHwnd = nullptr)
{
    bool ok = Apply(mem, g, worldPos, robloxHwnd);
    // second write in same frame helps against engine overwrite
    if (ok)
        Apply(mem, g, worldPos, robloxHwnd);
    return ok;
}
}
