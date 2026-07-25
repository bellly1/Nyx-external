#pragma once

#include "Config.hpp"
#include "imgui.h"

#include <Windows.h>
#include <mutex>
#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Notify
{
struct Toast
{
    char text[128]{};
    float life = 0.f;
    float maxLife = 3.f;
    Color4 color = Color4::From(0.2f, 0.85f, 0.95f, 1.f);
    int kind = 0; // 0 info, 1 success, 2 accent
};

inline std::mutex& Mutex()
{
    static std::mutex m;
    return m;
}

inline std::vector<Toast>& Queue()
{
    static std::vector<Toast> q;
    return q;
}

inline void Push(const char* text, const Color4& col = Color4::From(0.25f, 0.85f, 1.f, 1.f),
    float duration = 3.f, int kind = 0)
{
    if (!text || !text[0]) return;
    std::lock_guard<std::mutex> lock(Mutex());
    Toast t{};
    std::snprintf(t.text, sizeof(t.text), "%s", text);
    t.life = duration;
    t.maxLife = duration;
    t.color = col;
    t.kind = kind;
    Queue().push_back(t);
    if (Queue().size() > 6)
        Queue().erase(Queue().begin());
}

inline void Info(const Config& cfg, const char* text)
{
    if (!cfg.notificationsEnabled) return;
    Push(text, Color4::From(0.72f, 0.78f, 0.92f, 1.f), cfg.notifDuration, 0);
}

inline void Success(const Config& cfg, const char* text)
{
    if (!cfg.notificationsEnabled) return;
    Push(text, Color4::From(0.45f, 0.92f, 0.62f, 1.f), cfg.notifDuration, 1);
}

inline void Combat(const Config& cfg, const char* text)
{
    if (!cfg.notificationsEnabled) return;
    Push(text, Color4::From(1.f, 0.42f, 0.40f, 1.f), cfg.notifDuration * 0.85f, 2);
}

inline void Hit(const Config& cfg, const char* label = "Hit!")
{
    Combat(cfg, label);
}

inline void Render(const Config& cfg, float dt, float screenW)
{
    (void)cfg;
    std::lock_guard<std::mutex> lock(Mutex());
    auto& q = Queue();
    if (q.empty()) return;

    float y = 28.f;
    const float xRight = screenW - 24.f;
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    for (size_t i = 0; i < q.size();)
    {
        Toast& t = q[i];
        t.life -= dt;
        if (t.life <= 0.f)
        {
            q.erase(q.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }

        float fade = 1.f;
        if (t.life < 0.32f) fade = t.life / 0.32f;
        else if (t.maxLife - t.life < 0.20f) fade = (t.maxLife - t.life) / 0.20f;
        if (fade < 0.f) fade = 0.f;
        if (fade > 1.f) fade = 1.f;

        const float slide = (1.f - fade) * (1.f - fade) * 22.f;

        ImVec2 ts = ImGui::CalcTextSize(t.text);
        const float padX = 16.f, padY = 11.f;
        const float barW = 3.5f;
        const float w = ts.x + padX * 2.f + barW + 4.f;
        const float h = ts.y + padY * 2.f;
        const float x0 = xRight - w + slide;
        const float y0 = y;

        const int aBg = static_cast<int>(230 * fade);
        const int aBar = static_cast<int>(255 * fade);
        const int aTx = static_cast<int>(250 * fade);
        const int aBorder = static_cast<int>(28 * fade);
        const int cr = (int)(t.color.r * 255), cg = (int)(t.color.g * 255), cb = (int)(t.color.b * 255);

        dl->AddRectFilled({ x0 + 2.f, y0 + 3.f }, { x0 + w + 2.f, y0 + h + 3.f },
            IM_COL32(0, 0, 0, static_cast<int>(70 * fade)), 10.f);

        dl->AddRectFilled({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(12, 13, 18, aBg), 10.f);
        dl->AddRect({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(255, 255, 255, aBorder), 10.f, 0, 1.f);

        dl->AddRectFilled({ x0, y0 }, { x0 + barW, y0 + h }, IM_COL32(cr, cg, cb, aBar), 10.f,
            ImDrawFlags_RoundCornersLeft);

        const float lifeFrac = (t.maxLife > 0.01f) ? (t.life / t.maxLife) : 0.f;
        dl->AddRectFilled({ x0 + barW, y0 + h - 2.f },
            { x0 + barW + (w - barW) * lifeFrac, y0 + h },
            IM_COL32(cr, cg, cb, static_cast<int>(140 * fade)), 0.f);

        dl->AddText({ x0 + padX + barW, y0 + padY }, IM_COL32(236, 238, 246, aTx), t.text);

        y += h + 9.f;
        ++i;
    }
}
}
