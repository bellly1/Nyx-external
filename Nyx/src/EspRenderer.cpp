#include "Overlay.hpp"
#include "Math.hpp"
#include "Bones.hpp"
#include "Config.hpp"
#include "imgui.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace
{
ImU32 ToU32(const Color4& c) { return ColorToU32(c); }

inline float Snap(float v) { return std::floor(v + 0.5f); }
inline ImVec2 Snap2(float x, float y) { return { Snap(x), Snap(y) }; }
inline ImVec2 Snap2(ImVec2 p) { return { Snap(p.x), Snap(p.y) }; }

void StrokeLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float thick = 1.f)
{
    a = Snap2(a); b = Snap2(b);
    if (thick < 1.f) thick = 1.f;
    dl->AddLine(a, b, IM_COL32(0, 0, 0, 220), thick + 1.4f);
    dl->AddLine(a, b, color, thick);
}

void GlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float thick)
{
    a = Snap2(a); b = Snap2(b);
    if (thick < 1.f) thick = 1.f;
    const int cr = (int)((color >> IM_COL32_R_SHIFT) & 0xFF);
    const int cg = (int)((color >> IM_COL32_G_SHIFT) & 0xFF);
    const int cb = (int)((color >> IM_COL32_B_SHIFT) & 0xFF);
    const int ca = (int)((color >> IM_COL32_A_SHIFT) & 0xFF);
    dl->AddLine(a, b, IM_COL32(cr, cg, cb, (ca * 28) / 255), thick * 5.5f);
    dl->AddLine(a, b, IM_COL32(cr, cg, cb, (ca * 55) / 255), thick * 3.4f);
    dl->AddLine(a, b, IM_COL32(cr, cg, cb, (ca * 110) / 255), thick * 1.8f);
    dl->AddLine(a, b, IM_COL32(cr, cg, cb, ca), thick);
    dl->AddLine(a, b, IM_COL32(255, 255, 255, (ca * 160) / 255), (std::max)(1.f, thick * 0.45f));
}

void StrokeRect(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 color, float /*thick*/ = 1.f)
{
    min = Snap2(min); max = Snap2(max);
    dl->AddRect(min, max, IM_COL32(0, 0, 0, 255), 0.f, 0, 1.f);
    dl->AddRect({ min.x + 1.f, min.y + 1.f }, { max.x - 1.f, max.y - 1.f }, color, 0.f, 0, 1.f);
}

void CornerBox(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 color, float /*thick*/ = 1.f)
{
    min = Snap2(min); max = Snap2(max);
    const float w = max.x - min.x, h = max.y - min.y;
    const float len = Snap((std::max)(5.f, (std::min)(w, h) * 0.22f));
    auto arm = [&](ImVec2 o, float dx, float dy) {
        StrokeLine(dl, o, { o.x + dx * len, o.y }, color, 1.f);
        StrokeLine(dl, o, { o.x, o.y + dy * len }, color, 1.f);
    };
    arm(min, 1, 1);
    arm({ max.x, min.y }, -1, 1);
    arm({ min.x, max.y }, 1, -1);
    arm(max, -1, -1);
}

void GlowCornerBox(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 color, float w)
{
    min = Snap2(min); max = Snap2(max);
    const float bw = max.x - min.x, bh = max.y - min.y;
    const float len = Snap((std::max)(5.f, (std::min)(bw, bh) * 0.22f));
    auto arm = [&](ImVec2 o, float dx, float dy) {
        dl->AddLine(o, { o.x + dx * len, o.y }, color, w);
        dl->AddLine(o, { o.x, o.y + dy * len }, color, w);
    };
    arm(min, 1, 1);
    arm({ max.x, min.y }, -1, 1);
    arm({ min.x, max.y }, 1, -1);
    arm(max, -1, -1);
}

void StrokeCircle(ImDrawList* dl, ImVec2 c, float r, ImU32 color)
{
    c = Snap2(c);
    r = Snap(r);
    dl->AddCircle(c, r, IM_COL32(0, 0, 0, 255), 48, 1.f);
    dl->AddCircle(c, r, color, 48, 1.f);
}

ImVec2 CursorOL(HWND hwnd)
{
    POINT p{}; GetCursorPos(&p);
    if (hwnd) ScreenToClient(hwnd, &p);
    return { (float)p.x, (float)p.y };
}

void TextOut(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* t, bool bg, float fontSize)
{
    if (!t || !t[0]) return;
    ImFont* font = ImGui::GetFont();
    if (fontSize < 8.f) fontSize = 8.f;
    if (fontSize > 48.f) fontSize = 48.f;
    pos = Snap2(pos);
    const ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, t);
    if (bg)
        dl->AddRectFilled({ pos.x - 3.f, pos.y - 1.f }, { pos.x + ts.x + 3.f, pos.y + ts.y + 1.f }, IM_COL32(8, 8, 12, 200), 2.f);
    const ImU32 o = IM_COL32(0, 0, 0, 255);
    dl->AddText(font, fontSize, { pos.x - 1.f, pos.y }, o, t);
    dl->AddText(font, fontSize, { pos.x + 1.f, pos.y }, o, t);
    dl->AddText(font, fontSize, { pos.x, pos.y - 1.f }, o, t);
    dl->AddText(font, fontSize, { pos.x, pos.y + 1.f }, o, t);
    dl->AddText(font, fontSize, pos, col, t);
}

inline ImVec2 EspTextSize(const char* t, float fontSize)
{
    ImFont* font = ImGui::GetFont();
    if (fontSize < 8.f) fontSize = 8.f;
    return font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, t ? t : "");
}

float EspTag(ImDrawList* dl, float centerX, float topY, const char* text, ImU32 textCol, ImU32 accent, bool bg, float scale)
{
    if (!text || !text[0]) return 0.f;
    const bool tightPad = scale < 0.88f;

    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, text);

    const float padX = tightPad ? 3.f : 4.f;
    const float padY = tightPad ? 1.f : 2.f;
    const float barW = 2.f;
    const float gap = 2.f;
    const float w = ts.x + padX * 2.f + (bg ? barW + gap : 0.f);
    const float h = ts.y + padY * 2.f;
    const float x0 = Snap(centerX - w * 0.5f);
    const float y0 = Snap(topY);

    if (bg)
    {
        dl->AddRectFilled({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(8, 8, 12, 210), 2.f);
        dl->AddRectFilled({ x0 + 1.f, y0 + 2.f }, { x0 + 1.f + barW, y0 + h - 2.f }, accent, 1.f);
    }

    const float tx = Snap(x0 + padX + (bg ? barW + gap : 0.f));
    const float ty = Snap(y0 + padY);
    const ImU32 outline = IM_COL32(0, 0, 0, 255);
    if ((textCol >> 24) > 0)
    {
        dl->AddText(font, fs, { tx - 1.f, ty }, outline, text);
        dl->AddText(font, fs, { tx + 1.f, ty }, outline, text);
        dl->AddText(font, fs, { tx, ty - 1.f }, outline, text);
        dl->AddText(font, fs, { tx, ty + 1.f }, outline, text);
    }
    dl->AddText(font, fs, { tx, ty }, textCol, text);
    return h + 2.f;
}

Color4 LerpCol(const Color4& a, const Color4& b, float t)
{
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return Color4::From(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t);
}

Color4 HsvRgb(float h, float s, float v, float a = 1.f)
{
    h = h - std::floor(h);
    const float i = std::floor(h * 6.f);
    const float f = h * 6.f - i;
    const float p = v * (1.f - s);
    const float q = v * (1.f - f * s);
    const float t = v * (1.f - (1.f - f) * s);
    const int n = (int)i % 6;
    float r = v, g = t, b = p;
    if (n == 0) { r = v; g = t; b = p; }
    else if (n == 1) { r = q; g = v; b = p; }
    else if (n == 2) { r = p; g = v; b = t; }
    else if (n == 3) { r = p; g = q; b = v; }
    else if (n == 4) { r = t; g = p; b = v; }
    else { r = v; g = p; b = q; }
    return Color4::From(r, g, b, a);
}

Color4 ResolveEspColor(const Config& cfg, const Color4& base, float healthFrac, float dist, float timeSec)
{
    switch (cfg.colorMode)
    {
    case EspColorMode::Rainbow:
    {
        const float hue = timeSec * cfg.rainbowSpeed + dist * 0.002f;
        return HsvRgb(hue, 0.85f, 1.f, base.a);
    }
    case EspColorMode::Health:
        return LerpCol(cfg.colHealthLow, cfg.colHealthHigh, healthFrac);
    case EspColorMode::Distance:
    {
        const float maxD = cfg.renderDistance > 1.f ? cfg.renderDistance : 500.f;
        float t = dist / maxD;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        return LerpCol(cfg.colNear, cfg.colFar, t);
    }
    case EspColorMode::Pulse:
    {
        const float wave = 0.5f + 0.5f * std::sin(timeSec * cfg.pulseSpeed);
        return LerpCol(base, cfg.colAmbient, wave * (std::min)(1.f, cfg.ambientStrength * 1.25f));
    }
    case EspColorMode::Ambient:
        return LerpCol(base, cfg.colAmbient, (std::min)(1.f, 0.55f + cfg.ambientStrength * 0.55f));
    case EspColorMode::Static:
    default:
        return base;
    }
}
}

void Overlay::RenderEsp(const PlayerFrame& players, const CameraData& camera, const AimLockData& aim, const Config& cfg)
{
    float sw = (float)m_overlayW, sh = (float)m_overlayH;
    if (sw < 2 || sh < 2) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->Flags &= ~(ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLinesUseTex | ImDrawListFlags_AntiAliasedFill);

    float w2sW = (camera.valid && camera.dimensionsX > 1) ? camera.dimensionsX : sw;
    float w2sH = (camera.valid && camera.dimensionsY > 1) ? camera.dimensionsY : sh;
    float sx = sw / w2sW, sy = sh / w2sH;

    auto toOL = [&](float gx, float gy) -> ImVec2 { return { gx * sx, gy * sy }; };
    ImVec2 screenCenterOL{ sw * 0.5f, sh * 0.5f };
    ImVec2 mouseOL = CursorOL(m_hwnd);

    ImVec2 fovCenterOL = screenCenterOL;
    if (aim.active && !aim.useCenter && aim.cursorScreen.x > 1.f && aim.cursorScreen.y > 1.f)
        fovCenterOL = toOL(aim.cursorScreen.x, aim.cursorScreen.y);
    else if (!aim.useCenter)
        fovCenterOL = mouseOL;

    auto fovCol = [&]() -> ImU32 { return ToU32(cfg.colFov); };

    if (cfg.drawSilentFov && cfg.silentEnabled)
        StrokeCircle(dl, fovCenterOL, cfg.silentFov * sx, ToU32(cfg.colSilentFov));
    if (cfg.drawFov && cfg.aimEnabled)
        StrokeCircle(dl, fovCenterOL, cfg.aimFov * sx, fovCol());

    if (cfg.drawAimLine && aim.locked)
    {
        ImVec2 from = aim.useCenter ? screenCenterOL : fovCenterOL;
        ImVec2 to = toOL(aim.targetScreen.x, aim.targetScreen.y);
        Color4 lc = cfg.colAimLine;
        lc.a = 1.f;
        StrokeLine(dl, from, to, ToU32(lc), 1.0f);
        dl->AddCircleFilled(to, 2.2f, ToU32(lc), 10);
        dl->AddCircle(to, 2.2f, IM_COL32(0, 0, 0, 200), 10, 1.0f);
    }

    if (!cfg.espEnabled || !camera.valid || !players.valid) return;

    auto occ = cfg.visCheckEsp ? m_engine.Occlusion() : nullptr;

    const float pad = (std::max)(0.f, cfg.boxPadding);
    const float timeSec = (float)ImGui::GetTime();

    uintptr_t localTeam = 0;
    for (const auto& p : players.entities)
        if (p.isLocal) { localTeam = p.team; break; }

    for (const auto& p : players.entities)
    {
        if (!p.hasCharacter || p.validBones < 1) continue;
        if (p.isLocal && !cfg.showLocal) continue;
        if (p.isNpc && !cfg.npcEsp) continue;
        if (p.maxHealth > 1.f && p.health <= 0.f) continue;
        if (!p.isNpc && cfg.teamCheckEsp && localTeam && p.team && p.team == localTeam && !p.isLocal) continue;

        const float wd = (p.rootPos - camera.position).Length();
        if (cfg.renderDistance > 0.f && wd > cfg.renderDistance) continue;

        Vector2 sb[kBoneCount]{};
        bool on[kBoneCount]{};
        int proj = 0;
        for (int i = 0; i < kBoneCount; ++i)
        {
            if (!p.bones[i].valid) continue;
            Vector2 sp{};
            if (!WorldToScreen(p.bones[i].position, camera.viewMatrix, w2sW, w2sH, sp)) continue;
            sb[i] = { sp.x * sx, sp.y * sy };
            on[i] = true;
            ++proj;
        }
        if (proj < 1) continue;

        const int iHead = (int)BoneId::Head;
        const int iUT = (int)BoneId::UpperTorso;
        const int iLT = (int)BoneId::LowerTorso;
        const int iLF = (int)BoneId::LeftFoot;
        const int iRF = (int)BoneId::RightFoot;

        const BoneId boundsBones[] = {
            BoneId::Head, BoneId::UpperTorso, BoneId::LowerTorso,
            BoneId::LeftHand, BoneId::RightHand, BoneId::LeftFoot, BoneId::RightFoot
        };
        float minX = FLT_MAX, minY = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX;
        int boundCorners = 0;
        for (BoneId id : boundsBones)
        {
            const BoneJoint& bone = p.bones[static_cast<int>(id)];
            if (!bone.valid || bone.size.x <= 0.f || bone.size.y <= 0.f || bone.size.z <= 0.f)
                continue;

            const Vector3 half = bone.size * 0.5f;
            const Vector3& center = bone.boundsPosition;
            for (int x : { -1, 1 })
            for (int y : { -1, 1 })
            for (int z : { -1, 1 })
            {
                const Vector3 corner{
                    center.x + half.x * (float)x,
                    center.y + half.y * (float)y,
                    center.z + half.z * (float)z
                };
                Vector2 projected{};
                if (!WorldToScreen(corner, camera.viewMatrix, w2sW, w2sH, projected))
                    continue;
                const float px = projected.x * sx;
                const float py = projected.y * sy;
                minX = (std::min)(minX, px); minY = (std::min)(minY, py);
                maxX = (std::max)(maxX, px); maxY = (std::max)(maxY, py);
                ++boundCorners;
            }
        }
        if (boundCorners < 4 || maxX - minX < 3.f || maxY - minY < 3.f)
            continue;

        ImVec2 bmin{ Snap(minX - pad), Snap(minY - pad) };
        ImVec2 bmax{ Snap(maxX + pad), Snap(maxY + pad) };

        if (bmax.x < -20.f || bmax.y < -20.f || bmin.x > sw + 20.f || bmin.y > sh + 20.f)
            continue;

        const float cx = Snap((bmin.x + bmax.x) * 0.5f);

        float hpFrac = 1.f;
        if (p.maxHealth > 0.f)
            hpFrac = (std::max)(0.f, (std::min)(1.f, p.health / p.maxHealth));

        Color4 baseCol = cfg.colBox;
        if (p.isNpc)
            baseCol = cfg.colNpc;
        else if (cfg.teamBasedColor && !p.isLocal)
        {
            const bool ally = localTeam && p.team && p.team == localTeam;
            baseCol = ally ? cfg.colTeamAlly : cfg.colTeamEnemy;
        }

        if (cfg.visCheckEsp && !p.isLocal)
        {
            const bool visible = m_engine.IsEntityVisibleEsp(camera, p, occ.get());
            baseCol = visible ? cfg.colVisible : cfg.colHidden;
        }

        Color4 theme = ResolveEspColor(cfg, baseCol, hpFrac, wd, timeSec);
        Color4 skelCol = ResolveEspColor(cfg, cfg.colSkeleton, hpFrac, wd, timeSec);
        Color4 nameCol = ResolveEspColor(cfg, cfg.colName, hpFrac, wd, timeSec);
        Color4 headCol = ResolveEspColor(cfg, cfg.colHeadDot, hpFrac, wd, timeSec);
        Color4 tracerCol = ResolveEspColor(cfg, cfg.colTracer, hpFrac, wd, timeSec);
        if (cfg.visCheckEsp && !p.isLocal)
        {
            skelCol = theme;
            nameCol = theme;
            headCol = theme;
            tracerCol = theme;
        }
        else if (cfg.teamBasedColor && !p.isLocal)
        {
            skelCol = theme;
        }

        auto glowRgb = [](const Color4& c, int& cr, int& cg, int& cb) {
            cr = (int)(c.r * 255.f); cg = (int)(c.g * 255.f); cb = (int)(c.b * 255.f);
        };

        if (cfg.skeleton)
        {
            for (size_t e = 0; e < kSkeletonEdgeCount; ++e)
            {
                const int a = (int)kSkeletonEdges[e].from;
                const int b = (int)kSkeletonEdges[e].to;
                if (!on[a] || !on[b]) continue;
                ImVec2 p0 = Snap2(sb[a].x, sb[a].y);
                ImVec2 p1 = Snap2(sb[b].x, sb[b].y);
                if (cfg.glowEsp)
                {
                    int cr, cg, cb; glowRgb(skelCol, cr, cg, cb);
                    dl->AddLine(p0, p1, IM_COL32(cr, cg, cb, 40), 8.0f);
                    dl->AddLine(p0, p1, IM_COL32(cr, cg, cb, 80), 4.5f);
                    dl->AddLine(p0, p1, IM_COL32(cr, cg, cb, 140), 2.5f);
                }
                StrokeLine(dl, p0, p1, ToU32(skelCol), 1.5f);
            }
        }

        if (cfg.boxes)
        {
            if (cfg.boxFill)
            {
                Color4 fill = theme;
                fill.a = cfg.colBoxFill.a;
                dl->AddRectFilled(bmin, bmax, ToU32(fill));
            }
            if (cfg.boxType == BoxType::Corner)
            {
                if (cfg.glowEsp)
                {
                    int cr, cg, cb; glowRgb(theme, cr, cg, cb);
                    GlowCornerBox(dl, { bmin.x - 2.5f, bmin.y - 2.5f }, { bmax.x + 2.5f, bmax.y + 2.5f }, IM_COL32(cr, cg, cb, 25), 6.f);
                    GlowCornerBox(dl, { bmin.x - 1.f, bmin.y - 1.f }, { bmax.x + 1.f, bmax.y + 1.f }, IM_COL32(cr, cg, cb, 55), 3.5f);
                    GlowCornerBox(dl, { bmin.x - 0.f, bmin.y - 0.f }, { bmax.x + 0.f, bmax.y + 0.f }, IM_COL32(cr, cg, cb, 100), 2.f);
                }
                CornerBox(dl, bmin, bmax, ToU32(theme), 1.f);
            }
            else
            {
                if (cfg.glowEsp)
                {
                    int cr, cg, cb; glowRgb(theme, cr, cg, cb);
                    dl->AddRect({ bmin.x - 3.f, bmin.y - 3.f }, { bmax.x + 3.f, bmax.y + 3.f },
                        IM_COL32(cr, cg, cb, 35), 0.f, 0, 4.5f);
                    dl->AddRect({ bmin.x - 1.5f, bmin.y - 1.5f }, { bmax.x + 1.5f, bmax.y + 1.5f },
                        IM_COL32(cr, cg, cb, 75), 0.f, 0, 2.5f);
                }
                StrokeRect(dl, bmin, bmax, ToU32(theme), 1.f);
            }
        }

        if (cfg.headDot && on[iHead])
        {
            ImVec2 hp = Snap2(sb[iHead].x, sb[iHead].y);
            if (cfg.glowEsp)
            {
                int cr, cg, cb; glowRgb(headCol, cr, cg, cb);
                dl->AddCircleFilled(hp, 7.f, IM_COL32(cr, cg, cb, 25), 16);
                dl->AddCircleFilled(hp, 5.f, IM_COL32(cr, cg, cb, 55), 16);
                dl->AddCircleFilled(hp, 3.5f, IM_COL32(cr, cg, cb, 100), 12);
            }
            dl->AddCircleFilled(hp, 2.f, ToU32(headCol), 8);
            dl->AddCircle(hp, 2.f, IM_COL32(0, 0, 0, 220), 8, 1.f);
        }

        if (cfg.healthBar && p.maxHealth > 0.f)
        {
            const float t = hpFrac;
            const float gap = 3.f;
            const float barW = 2.f;
            ImVec2 h0{ bmin.x - gap - barW, bmin.y };
            ImVec2 h1{ bmin.x - gap, bmax.y };
            h0 = Snap2(h0); h1 = Snap2(h1);
            dl->AddRectFilled({ h0.x - 1.f, h0.y - 1.f }, { h1.x + 1.f, h1.y + 1.f }, IM_COL32(0, 0, 0, 200));
            dl->AddRectFilled(h0, h1, IM_COL32(18, 18, 22, 220));
            Color4 hc = cfg.colHealthHigh;
            hc.a = 1.f;
            const float filled = (h1.y - h0.y) * t;
            if (filled > 0.5f)
                dl->AddRectFilled({ h0.x, h1.y - filled }, h1, ToU32(hc));
            {
                const int hpInt = (int)(p.health + 0.5f);
                char hpBuf[16];
                std::snprintf(hpBuf, sizeof(hpBuf), "%d", hpInt);
                ImFont* font = ImGui::GetFont();
                const float hpFs = 9.f * cfg.textScale;
                if (hpFs >= 6.f) {
                    const ImVec2 ts = font->CalcTextSizeA(hpFs, FLT_MAX, 0.f, hpBuf);
                    const float ty = (h1.y - filled) - ts.y * 0.5f;
                    const float tx = (h0.x + h1.x) * 0.5f - ts.x * 0.5f;
                    const ImU32 hc32 = IM_COL32(255, 255, 255, 240);
                    const ImU32 oc = IM_COL32(0, 0, 0, 200);
                    dl->AddText(font, hpFs, { tx - 1.f, ty }, oc, hpBuf);
                    dl->AddText(font, hpFs, { tx + 1.f, ty }, oc, hpBuf);
                    dl->AddText(font, hpFs, { tx, ty - 1.f }, oc, hpBuf);
                    dl->AddText(font, hpFs, { tx, ty + 1.f }, oc, hpBuf);
                    dl->AddText(font, hpFs, { tx, ty }, hc32, hpBuf);
                }
            }
        }

        char nameBuf[80]{};
        if (cfg.names || (p.isNpc && cfg.npcEsp))
        {
            if (p.isNpc)
            {
                if (!p.displayName.empty())
                    std::snprintf(nameBuf, sizeof(nameBuf), "NPC | %s", p.displayName.c_str());
                else if (!p.name.empty())
                    std::snprintf(nameBuf, sizeof(nameBuf), "NPC | %s", p.name.c_str());
                else
                    std::snprintf(nameBuf, sizeof(nameBuf), "NPC");
            }
            else if (cfg.names)
            {
                if (cfg.nameMode == NameMode::Both && !p.displayName.empty() && p.displayName != p.name)
                    std::snprintf(nameBuf, sizeof(nameBuf), "%s", p.displayName.c_str());
                else if (cfg.nameMode == NameMode::Username || p.displayName.empty())
                    std::snprintf(nameBuf, sizeof(nameBuf), "%s", p.name.c_str());
                else
                    std::snprintf(nameBuf, sizeof(nameBuf), "%s", p.displayName.c_str());
            }
        }

        float scale = cfg.textScale;
        if (scale < 0.40f) scale = 0.40f;
        if (scale > 2.50f) scale = 2.50f;
        const float baseFs = ImGui::GetFontSize();
        const float fsName = baseFs * scale;
        const float fsTag  = baseFs * scale * 0.88f;

        auto drawTag = [&](float centerX, float& y, const char* text, ImU32 col, bool above) -> void {
            if (!text || !text[0]) return;
            ImFont* font = ImGui::GetFont();
            const float fs = fsTag;
            const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, text);
            const float padX = 5.f, padY = 2.f;
            const float w = ts.x + padX * 2.f;
            const float h = ts.y + padY * 2.f;
            if (above) y -= (h + 2.f);
            const float x0 = Snap(centerX - w * 0.5f);
            const float y0 = Snap(y);
            if (cfg.textBackground)
            {
                dl->AddRectFilled({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(6, 7, 10, 210), 5.f);
                dl->AddRect({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(255, 255, 255, 18), 5.f, 0, 1.f);
            }
            const float tx = Snap(x0 + padX);
            const float ty = Snap(y0 + padY);
            if (cfg.textOutline)
            {
                const ImU32 o = IM_COL32(0, 0, 0, 230);
                dl->AddText(font, fs, { tx - 1.f, ty }, o, text);
                dl->AddText(font, fs, { tx + 1.f, ty }, o, text);
                dl->AddText(font, fs, { tx, ty - 1.f }, o, text);
                dl->AddText(font, fs, { tx, ty + 1.f }, o, text);
            }
            dl->AddText(font, fs, { tx, ty }, col, text);
            if (!above) y += (h + 2.f);
        };

        float textY = bmin.y - 3.f;
        if (nameBuf[0])
        {
            Color4 nc = nameCol;
            if (cfg.textGradient)
                nc = ResolveEspColor(cfg, nameCol, hpFrac, wd, timeSec);
            ImFont* font = ImGui::GetFont();
            const ImVec2 ts = font->CalcTextSizeA(fsName, FLT_MAX, 0.f, nameBuf);
            const float padX = 6.f, padY = 2.5f;
            const float w = ts.x + padX * 2.f;
            const float h = ts.y + padY * 2.f;
            textY -= (h + 3.f);
            const float x0 = Snap(cx - w * 0.5f);
            const float y0 = Snap(textY);
            if (cfg.textBackground)
            {
                dl->AddRectFilled({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(8, 9, 12, 225), 6.f);
                dl->AddRectFilled({ x0, y0 }, { x0 + 2.5f, y0 + h }, ToU32(theme), 6.f, ImDrawFlags_RoundCornersLeft);
            }
            const float tx = Snap(x0 + padX);
            const float ty = Snap(y0 + padY);
            if (cfg.textOutline)
            {
                const ImU32 o = IM_COL32(0, 0, 0, 240);
                dl->AddText(font, fsName, { tx - 1.f, ty }, o, nameBuf);
                dl->AddText(font, fsName, { tx + 1.f, ty }, o, nameBuf);
                dl->AddText(font, fsName, { tx, ty - 1.f }, o, nameBuf);
                dl->AddText(font, fsName, { tx, ty + 1.f }, o, nameBuf);
            }
            dl->AddText(font, fsName, { tx, ty }, ToU32(nc), nameBuf);
        }

        if (cfg.distance)
        {
            char distBuf[24];
            std::snprintf(distBuf, sizeof(distBuf), "%.0f m", wd);
            Color4 dc = cfg.colDistance; dc.a = 1.f;
            drawTag(cx, textY, distBuf, ToU32(dc), true);
        }
        if (cfg.equippedItem && !p.equipped.empty())
        {
            char wepBuf[48];
            std::snprintf(wepBuf, sizeof(wepBuf), "%s", p.equipped.c_str());
            Color4 wc = cfg.colWeapon; wc.a = 1.f;
            drawTag(cx, textY, wepBuf, ToU32(wc), true);
        }

        if (cfg.healthText && p.maxHealth > 0.f)
        {
            char ht[32];
            std::snprintf(ht, sizeof(ht), "%.0f HP", p.health);
            Color4 hc = cfg.colHealthText;
            hc.a = 1.f;
            float below = bmax.y + 3.f;
            drawTag(cx, below, ht, ToU32(hc), false);
        }

        if (cfg.tracers)
        {
            ImVec2 o{ sw * 0.5f, sh };
            if (cfg.tracerOrigin == TracerOrigin::Top) o = { sw * 0.5f, 0.f };
            else if (cfg.tracerOrigin == TracerOrigin::Crosshair) o = fovCenterOL;
            else if (cfg.tracerOrigin == TracerOrigin::Center) o = { sw * 0.5f, sh * 0.5f };
            ImVec2 a = Snap2(o);
            ImVec2 b = Snap2(cx, bmax.y);
            if (cfg.glowEsp)
            {
                int cr, cg, cb; glowRgb(tracerCol, cr, cg, cb);
                dl->AddLine(a, b, IM_COL32(cr, cg, cb, 25), 5.0f);
                dl->AddLine(a, b, IM_COL32(cr, cg, cb, 60), 3.0f);
                dl->AddLine(a, b, IM_COL32(cr, cg, cb, 110), 1.5f);
            }
            StrokeLine(dl, a, b, ToU32(tracerCol), 1.f);
        }
    }
}
