#pragma once

#include "imgui.h"
#include "Config.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>

namespace ImAdd
{
inline int& MenuTheme()
{
    static int t = 1;
    return t;
}
inline void SetMenuTheme(int theme)
{
    if (theme < 0) theme = 0;
    if (theme > 3) theme = 3;
    MenuTheme() = theme;
}

struct ThemePalette
{
    ImVec4 shell;
    ImVec4 content;
    ImVec4 card;
    ImVec4 frame;
    ImVec4 frameH;
    ImVec4 frameA;
    ImVec4 popup;
    ImVec4 button;
    ImVec4 buttonH;
};

inline ThemePalette ThemeOf(int theme)
{
    // One shared presentation surface: #0A0B12.
    const ImVec4 base(0.039f, 0.043f, 0.071f, 1.f);
    ThemePalette p{};
    switch (theme)
    {
    case 0:
        p.shell   = ImVec4(0.086f, 0.090f, 0.118f, 0.95f);
        p.content = ImVec4(0.11f, 0.115f, 0.145f, 0.62f);
        p.card    = ImVec4(0.095f, 0.100f, 0.125f, 0.72f);
        p.frame   = ImVec4(0.12f, 0.125f, 0.155f, 1.f);
        p.frameH  = ImVec4(0.15f, 0.155f, 0.185f, 1.f);
        p.frameA  = ImVec4(0.17f, 0.175f, 0.210f, 1.f);
        p.popup   = ImVec4(0.09f, 0.095f, 0.12f, 0.98f);
        p.button  = ImVec4(0.13f, 0.135f, 0.165f, 1.f);
        p.buttonH = ImVec4(0.17f, 0.175f, 0.210f, 1.f);
        break;
    case 2:
        p.shell   = ImVec4(0.039f, 0.043f, 0.055f, 0.99f);
        p.content = ImVec4(0.050f, 0.053f, 0.068f, 0.70f);
        p.card    = ImVec4(0.042f, 0.045f, 0.058f, 0.78f);
        p.frame   = ImVec4(0.065f, 0.068f, 0.085f, 1.f);
        p.frameH  = ImVec4(0.085f, 0.090f, 0.110f, 1.f);
        p.frameA  = ImVec4(0.100f, 0.105f, 0.130f, 1.f);
        p.popup   = ImVec4(0.040f, 0.043f, 0.055f, 0.99f);
        p.button  = ImVec4(0.070f, 0.073f, 0.090f, 1.f);
        p.buttonH = ImVec4(0.095f, 0.100f, 0.125f, 1.f);
        break;
    case 3:
        p.shell   = ImVec4(0.024f, 0.027f, 0.039f, 1.f);
        p.content = ImVec4(0.032f, 0.035f, 0.050f, 0.75f);
        p.card    = ImVec4(0.028f, 0.030f, 0.045f, 0.82f);
        p.frame   = ImVec4(0.045f, 0.048f, 0.068f, 1.f);
        p.frameH  = ImVec4(0.060f, 0.065f, 0.090f, 1.f);
        p.frameA  = ImVec4(0.075f, 0.080f, 0.110f, 1.f);
        p.popup   = ImVec4(0.028f, 0.030f, 0.045f, 0.99f);
        p.button  = ImVec4(0.050f, 0.053f, 0.075f, 1.f);
        p.buttonH = ImVec4(0.070f, 0.075f, 0.100f, 1.f);
        break;
    default:
        p.shell   = ImVec4(0.055f, 0.059f, 0.078f, 0.97f);
        p.content = ImVec4(0.070f, 0.074f, 0.095f, 0.60f);
        p.card    = ImVec4(0.055f, 0.058f, 0.075f, 0.70f);
        p.frame   = ImVec4(0.085f, 0.088f, 0.110f, 1.f);
        p.frameH  = ImVec4(0.110f, 0.115f, 0.140f, 1.f);
        p.frameA  = ImVec4(0.130f, 0.135f, 0.165f, 1.f);
        p.popup   = ImVec4(0.055f, 0.058f, 0.075f, 0.98f);
        p.button  = ImVec4(0.095f, 0.100f, 0.125f, 1.f);
        p.buttonH = ImVec4(0.125f, 0.130f, 0.160f, 1.f);
        break;
    }
    // Keep every non-interactive surface consistent across theme selections.
    p.shell = p.content = p.card = p.frame = p.popup = p.button = base;
    p.frameH = p.buttonH = ImVec4(base.x + 0.018f, base.y + 0.018f, base.z + 0.024f, 1.f);
    p.frameA = ImVec4(base.x + 0.032f, base.y + 0.032f, base.z + 0.042f, 1.f);
    return p;
}

inline ImU32 Col(const Color4& a, int alpha = -1)
{
    auto ch = [](float v) -> int {
        if (v < 0.f) return 0;
        if (v > 1.f) return 255;
        return (int)(v * 255.f + 0.5f);
    };
    return IM_COL32(ch(a.r), ch(a.g), ch(a.b), alpha < 0 ? ch(a.a) : alpha);
}

inline void ApplyStyle(const Color4& accent, int theme = -1)
{
    if (theme < 0) theme = MenuTheme();
    else SetMenuTheme(theme);

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 18.f;
    s.ChildRounding = 14.f;
    s.FrameRounding = 10.f;
    s.PopupRounding = 13.f;
    s.GrabRounding = 10.f;
    s.ScrollbarRounding = 12.f;
    s.TabRounding = 10.f;
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize = 0.f;
    s.FrameBorderSize = 0.f;
    s.WindowPadding = ImVec2(0, 0);
    s.FramePadding = ImVec2(10, 5);
    s.ItemSpacing = ImVec2(8, 5);
    s.ItemInnerSpacing = ImVec2(5, 3);
    s.ScrollbarSize = 6.f;
    s.GrabMinSize = 9.f;

    const ThemePalette tp = ThemeOf(MenuTheme());
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]       = tp.shell;
    c[ImGuiCol_ChildBg]        = tp.card;
    c[ImGuiCol_PopupBg]        = tp.popup;
    c[ImGuiCol_Border]         = ImVec4(1.f, 1.f, 1.f, MenuTheme() >= 2 ? 0.05f : 0.08f);
    c[ImGuiCol_Text]           = ImVec4(0.93f, 0.94f, 0.97f, 1.f);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.46f, 0.47f, 0.52f, 1.f);
    c[ImGuiCol_FrameBg]        = tp.frame;
    c[ImGuiCol_FrameBgHovered] = ImVec4(
        accent.r * 0.12f + tp.frameH.x * 0.88f,
        accent.g * 0.12f + tp.frameH.y * 0.88f,
        accent.b * 0.12f + tp.frameH.z * 0.88f, 1.f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(
        accent.r * 0.20f + tp.frameA.x * 0.80f,
        accent.g * 0.20f + tp.frameA.y * 0.80f,
        accent.b * 0.20f + tp.frameA.z * 0.80f, 1.f);
    c[ImGuiCol_Button]         = tp.button;
    c[ImGuiCol_ButtonHovered]  = ImVec4(
        accent.r * 0.28f + tp.buttonH.x * 0.55f,
        accent.g * 0.28f + tp.buttonH.y * 0.55f,
        accent.b * 0.28f + tp.buttonH.z * 0.55f, 1.f);
    c[ImGuiCol_ButtonActive]   = ImVec4(accent.r * 0.50f, accent.g * 0.50f, accent.b * 0.50f, 1.f);
    c[ImGuiCol_Header]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_HeaderHovered]  = tp.frameH;
    c[ImGuiCol_HeaderActive]   = ImVec4(accent.r * 0.28f, accent.g * 0.28f, accent.b * 0.28f, 0.85f);
    c[ImGuiCol_CheckMark]      = ImVec4(0.04f, 0.04f, 0.05f, 1.f);
    c[ImGuiCol_SliderGrab]     = ImVec4(accent.r, accent.g, accent.b, 1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(
        (std::min)(1.f, accent.r * 1.08f),
        (std::min)(1.f, accent.g * 1.08f),
        (std::min)(1.f, accent.b * 1.08f), 1.f);
    c[ImGuiCol_Separator]      = ImVec4(0.16f, 0.17f, 0.20f, 0.45f);
    c[ImGuiCol_ScrollbarBg]    = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]  = ImVec4(accent.r * 0.42f, accent.g * 0.42f, accent.b * 0.42f, 0.90f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accent.r * 0.58f, accent.g * 0.58f, accent.b * 0.58f, 1.f);
    c[ImGuiCol_TableHeaderBg]  = tp.card;
    c[ImGuiCol_TableRowBg]     = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]  = ImVec4(1, 1, 1, 0.014f);
}

inline void ContentShell(const char* id, ImVec2 size)
{
    const ThemePalette tp = ThemeOf(MenuTheme());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, tp.content);
    ImGui::BeginChild(id, size, ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

inline void EndContentShell()
{
    ImGui::EndChild();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}

inline void Column(const char* id, ImVec2 size)
{
    const ThemePalette tp = ThemeOf(MenuTheme());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 13.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 3));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, tp.card);
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

inline void EndColumn()
{
    ImGui::EndChild();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}

inline bool ColorPickerButton(const char* id, Color4* c, ImVec2 size = ImVec2(20.f, 20.f), bool withAlpha = true, bool above = false)
{
    if (!c) return false;
    ImGui::PushID(id);
    ImGui::PushID(static_cast<const void*>(c));
    auto clamp01 = [](float& v) {
        if (v != v) v = 1.f;
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
    };
    clamp01(c->r); clamp01(c->g); clamp01(c->b); clamp01(c->a);
    const float side = (size.x > 2.f) ? size.x : 20.f;
    char popupId[48];
    std::snprintf(popupId, sizeof(popupId), "##cp%p", static_cast<const void*>(c));
    const ImVec4 col(c->r, c->g, c->b, withAlpha ? c->a : 1.f);
    ImGuiColorEditFlags btnFlags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreviewHalf;
    if (!withAlpha) btnFlags |= ImGuiColorEditFlags_NoAlpha;
    bool openPopup = false;
    if (ImGui::ColorButton("##sw", col, btnFlags, ImVec2(side, side)))
        openPopup = true;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        openPopup = true;
    ImVec2 btnMin = ImGui::GetItemRectMin();
    ImVec2 btnMax = ImGui::GetItemRectMax();
    if (openPopup)
        ImGui::OpenPopup(popupId);
    bool changed = false;
    ImGui::SetNextWindowSize(ImVec2(200.f, 0.f), ImGuiCond_Always);
    ImVec2 disp = ImGui::GetIO().DisplaySize;
    ImVec2 popupPos;
    if (above)
    {
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        popupPos = { winPos.x + winSize.x * 0.5f - 100.f, winPos.y + winSize.y * 0.5f - 150.f };
    }
    else
    {
        popupPos = { btnMin.x, btnMax.y + 2.f };
        if (popupPos.y + 300.f > disp.y)
            popupPos.y = btnMin.y - 300.f;
    }
    if (popupPos.x + 210.f > disp.x)
        popupPos.x = disp.x - 210.f;
    if (popupPos.x < 0.f) popupPos.x = 0.f;
    if (popupPos.y < 0.f) popupPos.y = 0.f;
    ImGui::SetNextWindowPos(popupPos, ImGuiCond_Always);
    if (ImGui::BeginPopup(popupId, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGuiColorEditFlags pf = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB |
            ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoLabel;
        if (withAlpha) pf |= ImGuiColorEditFlags_AlphaBar;
        else pf |= ImGuiColorEditFlags_NoAlpha;
        ImGui::PushItemWidth(170.f);
        changed = ImGui::ColorPicker4("##picker", c->Data(), pf);
        ImGui::PopItemWidth();
        if (!withAlpha) c->a = 1.f;
        clamp01(c->r); clamp01(c->g); clamp01(c->b); clamp01(c->a);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

inline void ColorRow(const char* id, Color4* c, const char* caption)
{
    ImGui::PushID(static_cast<const void*>(c));
    ColorPickerButton(id, c, ImVec2(20.f, 20.f), true);
    ImGui::SameLine(0, 8.f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(caption ? caption : "");
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0, 1));
}

inline const char* DisplayLabel(const char* label, char* buf, size_t bufSize)
{
    if (!label) { if (bufSize) buf[0] = 0; return buf; }
    const char* hash = std::strstr(label, "##");
    if (!hash) return label;
    size_t n = static_cast<size_t>(hash - label);
    if (n >= bufSize) n = bufSize - 1;
    std::memcpy(buf, label, n);
    buf[n] = 0;
    return buf;
}

inline bool Checkbox(const char* label, bool* v, const Color4& accent, int* key = nullptr, bool* waiting = nullptr)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 24.f;
    const float boxSize = 18.f;
    const float boxRounding = 4.5f;
    ImGui::Dummy(ImVec2(0, 1.f));
    const float fullW = ImGui::GetContentRegionAvail().x;
    ImVec2 start = ImGui::GetCursorScreenPos();
    char disp[128]{};
    const char* shown = DisplayLabel(label, disp, sizeof(disp));
    ImGui::PushID(static_cast<const void*>(v));
    if (key) ImGui::PushID(static_cast<const void*>(key));
    if (waiting) ImGui::PushID(static_cast<const void*>(waiting));
    ImGui::InvisibleButton("##cb_row", ImVec2(fullW, h));
    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
    const bool clicked = ImGui::IsItemClicked(0);
    float pillKbW = 0.f;
    char kb[24] = {};
    if (key)
    {
        if (waiting && *waiting) std::snprintf(kb, sizeof(kb), "...");
        else std::snprintf(kb, sizeof(kb), "%s", KeyName(*key));
        ImVec2 ksz = ImGui::CalcTextSize(kb);
        pillKbW = ksz.x + 16.f;
        if (pillKbW < 36.f) pillKbW = 36.f;
    }
    if (clicked)
    {
        float mx = ImGui::GetIO().MousePos.x;
        if (key && mx > start.x + fullW - pillKbW - 4.f)
        { if (waiting) *waiting = true; }
        else if (!(waiting && *waiting)) { *v = !*v; }
    }

    if (hovered)
    {
        dl->AddRectFilled(ImVec2(start.x - 4.f, start.y), ImVec2(start.x + fullW + 4.f, start.y + h), IM_COL32(255, 255, 255, 8), 6.f);
    }

    const float boxY = start.y + (h - boxSize) * 0.5f;
    const float boxX = start.x + 2.f;
    const ImVec2 boxMin = { boxX, boxY };
    const ImVec2 boxMax = { boxX + boxSize, boxY + boxSize };

    if (*v)
    {
        dl->AddRectFilled(ImVec2(boxMin.x - 1.5f, boxMin.y - 1.5f), ImVec2(boxMax.x + 1.5f, boxMax.y + 1.5f), Col(accent, hovered ? 75 : 45), boxRounding + 1.5f);

        ImU32 boxBg = Col(accent);
        if (hovered) {
            boxBg = IM_COL32(
                (int)((std::min)(1.f, accent.r * 1.15f) * 255.f),
                (int)((std::min)(1.f, accent.g * 1.15f) * 255.f),
                (int)((std::min)(1.f, accent.b * 1.15f) * 255.f), 255);
        }
        dl->AddRectFilled(boxMin, boxMax, boxBg, boxRounding);
    }
    else
    {
        ImU32 boxBg = hovered ? IM_COL32(40, 44, 56, 255) : IM_COL32(24, 26, 36, 255);
        ImU32 boxBorder = hovered ? IM_COL32(255, 255, 255, 45) : IM_COL32(255, 255, 255, 25);

        dl->AddRectFilled(boxMin, boxMax, boxBg, boxRounding);
        dl->AddRect(boxMin, boxMax, boxBorder, boxRounding, 0, 1.2f);
    }

    ImU32 textCol = *v ? IM_COL32(240, 242, 250, 255) : (hovered ? IM_COL32(205, 210, 222, 255) : IM_COL32(170, 172, 185, 255));
    dl->AddText({ boxX + boxSize + 10.f, start.y + (h - ImGui::GetTextLineHeight()) * 0.5f }, textCol, shown);

    if (key)
    {
        ImVec2 p0{ start.x + fullW - pillKbW, start.y + 1.f };
        ImVec2 p1{ start.x + fullW, start.y + h - 1.f };
        const bool isWaiting = waiting && *waiting;
        ImU32 kbBg = isWaiting ? Col(accent, 160) : (hovered ? IM_COL32(28, 32, 46, 255) : IM_COL32(20, 22, 32, 255));
        ImU32 kbBorder = isWaiting ? Col(accent) : IM_COL32(255, 255, 255, 22);

        dl->AddRectFilled(p0, p1, kbBg, 6.f);
        dl->AddRect(p0, p1, kbBorder, 6.f, 0, 1.f);

        ImVec2 ts = ImGui::CalcTextSize(kb);
        ImU32 kbTxtCol = isWaiting ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 184, 198, 255);
        dl->AddText({ p0.x + (pillKbW - ts.x) * 0.5f, p0.y + (p1.y - p0.y - ts.y) * 0.5f }, kbTxtCol, kb);
    }

    if (waiting) ImGui::PopID();
    if (key) ImGui::PopID();
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0, 1.f));
    return *v;
}

inline bool CheckColorRow(const char* label, bool* v, const Color4& accent, Color4* col, bool above = false)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 24.f;
    const float boxSize = 18.f;
    const float boxRounding = 4.5f;
    const float sw = 20.f;
    const float swGap = 8.f;
    ImGui::Dummy(ImVec2(0, 1.f));
    const float fullW = ImGui::GetContentRegionAvail().x;
    char disp[128]{};
    const char* shown = DisplayLabel(label, disp, sizeof(disp));
    ImGui::PushID(static_cast<const void*>(v));
    if (col) ImGui::PushID(static_cast<const void*>(col));
    ImGui::BeginGroup();
    const float checkW = col ? (fullW - sw - swGap) : fullW;
    ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##cc_row", ImVec2(checkW, h));
    const bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked(0)) *v = !*v;

    if (hovered)
    {
        dl->AddRectFilled(ImVec2(start.x - 4.f, start.y), ImVec2(start.x + checkW + 4.f, start.y + h), IM_COL32(255, 255, 255, 8), 6.f);
    }

    const float boxY = start.y + (h - boxSize) * 0.5f;
    const float boxX = start.x + 2.f;
    const ImVec2 boxMin = { boxX, boxY };
    const ImVec2 boxMax = { boxX + boxSize, boxY + boxSize };

    if (*v)
    {
        dl->AddRectFilled(ImVec2(boxMin.x - 1.5f, boxMin.y - 1.5f), ImVec2(boxMax.x + 1.5f, boxMax.y + 1.5f), Col(accent, hovered ? 75 : 45), boxRounding + 1.5f);

        ImU32 boxBg = Col(accent);
        if (hovered) {
            boxBg = IM_COL32(
                (int)((std::min)(1.f, accent.r * 1.15f) * 255.f),
                (int)((std::min)(1.f, accent.g * 1.15f) * 255.f),
                (int)((std::min)(1.f, accent.b * 1.15f) * 255.f), 255);
        }
        dl->AddRectFilled(boxMin, boxMax, boxBg, boxRounding);
    }
    else
    {
        ImU32 boxBg = hovered ? IM_COL32(40, 44, 56, 255) : IM_COL32(24, 26, 36, 255);
        ImU32 boxBorder = hovered ? IM_COL32(255, 255, 255, 45) : IM_COL32(255, 255, 255, 25);

        dl->AddRectFilled(boxMin, boxMax, boxBg, boxRounding);
        dl->AddRect(boxMin, boxMax, boxBorder, boxRounding, 0, 1.2f);
    }

    ImU32 textCol = *v ? IM_COL32(240, 242, 250, 255) : (hovered ? IM_COL32(205, 210, 222, 255) : IM_COL32(170, 172, 185, 255));
    dl->AddText({ boxX + boxSize + 10.f, start.y + (h - ImGui::GetTextLineHeight()) * 0.5f }, textCol, shown);

    if (col)
    {
        ImGui::SameLine(0, swGap);
        const float yPad = (h - sw) * 0.5f;
        if (yPad > 0.f) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yPad);
        ColorPickerButton("##rowsw", col, ImVec2(sw, sw), true, above);
    }
    ImGui::EndGroup();
    if (col) ImGui::PopID();
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0, 1.f));
    return *v;
}

inline void Divider(const Color4& accent)
{
    (void)accent;
    ImGui::Dummy(ImVec2(0, 8.f));
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddRectFilled(
        { p.x + 2.f, p.y }, { p.x + w - 2.f, p.y + 1.f }, IM_COL32(255, 255, 255, 10), 0.f);
    ImGui::Dummy(ImVec2(0, 8.f));
}

inline bool Combo(const char* label, int* cur, const char* const* items, int count)
{
    char disp[128]{};
    const char* shown = DisplayLabel(label, disp, sizeof(disp));
    if (shown && shown[0] && shown[0] != '#')
    {
        ImGui::TextDisabled("%s", shown);
        ImGui::Dummy(ImVec2(0, 2.f));
    }
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 24.f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushID(static_cast<const void*>(cur));
    ImGui::PushID(label ? label : "combo");
    const char* id = "##combo_btn";
    ImGui::InvisibleButton(id, ImVec2(w, h));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();
    const bool isOpen = ImGui::IsPopupOpen(id);
    if (clicked) ImGui::OpenPopup(id);

    // Blends naturally with menu card background (#0A0B12)
    ImU32 bgCol = isOpen ? IM_COL32(20, 24, 38, 255) : (hovered ? IM_COL32(18, 22, 35, 255) : IM_COL32(12, 15, 24, 255));
    ImU32 borderCol = isOpen ? IM_COL32(50, 114, 216, 180) : (hovered ? IM_COL32(50, 114, 216, 100) : IM_COL32(30, 45, 72, 60));

    dl->AddRectFilled(pos, { pos.x + w, pos.y + h }, bgCol, 5.f);
    dl->AddRect(pos, { pos.x + w, pos.y + h }, borderCol, 5.f, 0, 1.f);

    const char* text = (*cur >= 0 && count > 0 && *cur < count) ? items[*cur] : "Select...";
    const bool noneSel = !(*cur >= 0 && count > 0 && *cur < count);
    dl->AddText({ pos.x + 8.f, pos.y + (h - ImGui::GetTextLineHeight()) * 0.5f },
        noneSel ? IM_COL32(130, 132, 145, 255) : IM_COL32(240, 242, 250, 255), text);

    {
        float bx = pos.x + w - 16.f;
        float by = pos.y + h * 0.5f;
        ImU32 arrowCol = isOpen ? IM_COL32(255, 255, 255, 255) : (hovered ? IM_COL32(220, 225, 240, 255) : IM_COL32(150, 155, 175, 255));
        if (isOpen)
        {
            dl->AddLine({ bx - 4.f, by + 2.f }, { bx, by - 2.f }, arrowCol, 2.0f);
            dl->AddLine({ bx, by - 2.f }, { bx + 4.f, by + 2.f }, arrowCol, 2.0f);
        }
        else
        {
            dl->AddLine({ bx - 4.f, by - 2.f }, { bx, by + 2.f }, arrowCol, 2.0f);
            dl->AddLine({ bx, by + 2.f }, { bx + 4.f, by - 2.f }, arrowCol, 2.0f);
        }
    }

    bool changed = false;
    ImGui::SetNextWindowPos({ pos.x, pos.y + h + 4.f });
    ImGui::SetNextWindowSize({ w, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 6.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 9.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f, 3.f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.07f, 0.12f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.12f, 0.18f, 0.32f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.22f, 0.40f, 1.0f));

    if (ImGui::BeginPopup(id, ImGuiWindowFlags_NoMove))
    {
        for (int i = 0; i < count; ++i)
        {
            ImGui::PushID(i);
            const bool isSelected = (*cur == i);
            if (ImGui::Selectable(items[i], isSelected, 0, ImVec2(0, 20.f)))
            {
                *cur = i;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
    ImGui::PopID();
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0, 4.f));
    return changed;
}

inline bool Slider(const char* label, float* v, float mn, float mx, const Color4& accent, const char* fmt = "%.0f")
{
    char disp[128]{};
    const char* shown = DisplayLabel(label, disp, sizeof(disp));
    ImGui::PushID(static_cast<const void*>(v));
    char buf[32];
    std::snprintf(buf, sizeof(buf), fmt, *v);
    if (shown && shown[0] && shown[0] != '#')
    {
        ImGui::TextUnformatted(shown);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buf).x);
        ImGui::TextDisabled("%s", buf);
    }
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 18.f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton("##slider", ImVec2(w, h));
    if (ImGui::IsItemActive())
    {
        float t = (ImGui::GetIO().MousePos.x - pos.x) / ((w > 1.f) ? w : 1.f);
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        *v = mn + t * (mx - mn);
    }
    float t = (*v - mn) / ((mx - mn) <= 0.f ? 1.f : (mx - mn));
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    const float cy = pos.y + h * 0.5f;
    dl->AddRectFilled({ pos.x, cy - 1.5f }, { pos.x + w, cy + 1.5f }, IM_COL32(22, 24, 35, 255), 2.f);
    if (t > 0.001f)
        dl->AddRectFilled({ pos.x, cy - 1.5f }, { pos.x + w * t, cy + 1.5f }, Col(accent), 2.f);
    dl->AddCircleFilled({ pos.x + w * t, cy }, 5.5f, IM_COL32(248, 248, 255, 255), 20);
    ImGui::PopID();
    return ImGui::IsItemActive();
}

inline bool MainTab(const char* label, bool selected, const Color4&)
{
    ImGui::PushStyleColor(ImGuiCol_Text, selected
        ? ImVec4(0.92f, 0.93f, 0.96f, 1.f)
        : ImVec4(0.45f, 0.46f, 0.52f, 1.f));
    bool c = ImGui::Selectable(label, false, 0, ImGui::CalcTextSize(label));
    ImGui::PopStyleColor();
    return c;
}

inline bool SubTab(const char* label, bool selected, const Color4& accent)
{
    ImVec2 ts = ImGui::CalcTextSize(label);
    ImVec2 pad(16.f, 7.f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, { ts.x + pad.x * 2.f, ts.y + pad.y * 2.f });
    bool hovered = ImGui::IsItemHovered();
    bool pressed = ImGui::IsItemClicked();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 acc = IM_COL32((int)(accent.r*255), (int)(accent.g*255), (int)(accent.b*255), 255);
    const ImVec2 end{ p.x + ts.x + pad.x * 2.f, p.y + ts.y + pad.y * 2.f };
    if (hovered && !selected)
        dl->AddRectFilled(p, end, IM_COL32(40, 42, 50, 50), 6.f);
    dl->AddText({ p.x + pad.x, p.y + pad.y }, selected
        ? IM_COL32(235, 236, 242, 255)
        : IM_COL32(120, 122, 135, 255), label);
    if (selected)
        dl->AddRectFilled({ p.x + pad.x, end.y - 2.f }, { end.x - pad.x, end.y }, acc, 2.f);
    return pressed;
}

inline void Label(const char* t, const Color4& accent)
{
    ImGui::Dummy(ImVec2(0, 2.f));
    ImGui::TextColored(ImVec4(
        accent.r * 0.85f + 0.15f,
        accent.g * 0.85f + 0.15f,
        accent.b * 0.85f + 0.15f, 1.f), "%s", t);
    ImGui::Dummy({ 0, 5.f });
}

inline void ColorSq(const char* id, Color4* c)
{
    ColorPickerButton(id, c, ImVec2(20.f, 20.f), true);
}

inline bool ActionButton(const char* label, const Color4& accent, ImVec2 size, int style = 1)
{
    if (size.x <= 0) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0) size.y = 26.f;
    ImVec4 bg, bgH, bgA, tx;
    const ThemePalette tp = ThemeOf(MenuTheme());
    if (style == 2)
    {
        bg  = ImVec4(0.18f, 0.12f, 0.14f, 1.f);
        bgH = ImVec4(0.26f, 0.14f, 0.16f, 1.f);
        bgA = ImVec4(0.34f, 0.16f, 0.18f, 1.f);
        tx  = ImVec4(0.94f, 0.88f, 0.90f, 1.f);
    }
    else
    {
        (void)accent;
        bg  = tp.button;
        bgH = tp.buttonH;
        bgA = ImVec4((std::min)(1.f, tp.buttonH.x + 0.05f), (std::min)(1.f, tp.buttonH.y + 0.05f), (std::min)(1.f, tp.buttonH.z + 0.06f), 1.f);
        tx  = ImVec4(0.90f, 0.91f, 0.95f, 1.f);
    }
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgA);
    ImGui::PushStyleColor(ImGuiCol_Text, tx);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.f, 5.f));
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    return pressed;
}

inline bool AnyBindableKeyDown(int* outVk = nullptr)
{
    for (int vk : { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 })
    {
        if (GetAsyncKeyState(vk) & 0x8000) { if (outVk) *outVk = vk; return true; }
    }
    for (int vk = 8; vk < 256; ++vk)
    {
        if (vk >= VK_LBUTTON && vk <= VK_XBUTTON2) continue;
        if (GetAsyncKeyState(vk) & 0x8000) { if (outVk) *outVk = vk; return true; }
    }
    return false;
}

inline void PollKey(int* key, bool* waiting, int& phase)
{
    if (!waiting || !*waiting) { phase = 0; return; }
    if (phase <= 0) phase = 1;
    if (phase == 1) { if (!AnyBindableKeyDown()) phase = 2; return; }
    int vk = 0;
    if (!AnyBindableKeyDown(&vk)) return;
    if (vk == VK_ESCAPE) { *waiting = false; phase = 0; return; }
    *key = vk;
    *waiting = false;
    phase = 0;
}
}
