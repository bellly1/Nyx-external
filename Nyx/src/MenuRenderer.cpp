#include "Overlay.hpp"
#include "Math.hpp"
#include "ImAddWidgets.hpp"
#include "ConfigIO.hpp"
#include "Notify.hpp"
#include "AutoUpdate.hpp"
#include "Protect.hpp"
#include "Config.hpp"
#include "imgui.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

void Overlay::DrawMenu(Config& cfg, const GlobalsData& globals, const PlayerFrame& players, const CameraData& camera)
{
    (void)globals; (void)players; (void)camera;
    if (!cfg.menuOpen) { m_menuRectValid = false; return; }

    static int s_lastTheme = -1;
    if (memcmp(&cfg.accent, &m_lastAccent, sizeof(Color4)) != 0) {
        StyleImGui(cfg.accent);
        m_lastAccent = cfg.accent;
        s_lastTheme = cfg.menuBgTheme;
    } else if (cfg.menuBgTheme != s_lastTheme) {
        ImAdd::SetMenuTheme(cfg.menuBgTheme);
        s_lastTheme = cfg.menuBgTheme;
    }

    if (m_waitAimKey) ImAdd::PollKey(&cfg.aimKey, &m_waitAimKey, m_bindPhase);
    else if (m_waitSilentKey) ImAdd::PollKey(&cfg.silentKey, &m_waitSilentKey, m_bindPhase);
    else if (m_waitTriggerKey) ImAdd::PollKey(&cfg.triggerKey, &m_waitTriggerKey, m_bindPhase);
    else if (m_waitMenuKey) ImAdd::PollKey(&cfg.menuKey, &m_waitMenuKey, m_bindPhase);
    else if (m_waitSpeedKey) ImAdd::PollKey(&cfg.speedKey, &m_waitSpeedKey, m_bindPhase);
    else if (m_waitFlyKey) ImAdd::PollKey(&cfg.flyKey, &m_waitFlyKey, m_bindPhase);
    else m_bindPhase = 0;

    const float mw = 720.f, mh = 595.f;
    ImVec2 disp = ImGui::GetIO().DisplaySize;

    static float s_winX = -1.f, s_winY = -1.f;
    if (s_winX < 0.f) {
        s_winX = (disp.x - mw) * 0.5f;
        s_winY = (disp.y - mh) * 0.5f;
    }

    ImGui::SetNextWindowSize({ mw, mh }, ImGuiCond_Always);
    ImGui::SetNextWindowPos({ s_winX, s_winY }, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));

    bool open = cfg.menuOpen;
    if (!ImGui::Begin("##menu", &open,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBackground))
    {
        ImGui::End(); ImGui::PopStyleColor(2); ImGui::PopStyleVar(2);
        cfg.menuOpen = open;
        if (!open) { m_menuRectValid = false; SetClickThrough(true); }
        return;
    }
    cfg.menuOpen = open;

    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();
    m_menuX = wp.x; m_menuY = wp.y; m_menuW = ws.x; m_menuH = ws.y;
    m_menuRectValid = true;

    {
        ImDrawList* bg = ImGui::GetWindowDrawList();
        const ImVec2 a = wp;
        const ImVec2 b = { wp.x + ws.x, wp.y + ws.y };
        const ImU32 base = IM_COL32(10, 11, 18, 255); // #0A0B12
        bg->AddRectFilled(a, b, base, 18.f);
        bg->AddRect(a, b, IM_COL32(255, 255, 255, 13), 18.f, 0, 1.f);
    }

    float topH = 58.f;
    {
        ImDrawList* bg = ImGui::GetWindowDrawList();
        bg->AddRectFilled(wp, { wp.x + ws.x, wp.y + topH }, IM_COL32(10, 11, 18, 255), 18.f,
            ImDrawFlags_RoundCornersTop);
    }

    ImGui::SetCursorPos({ 14, 9 });
    ImGui::TextColored(ImVec4(cfg.accent.r, cfg.accent.g, cfg.accent.b, 1.f), "%s", kAppName);
    ImGui::SameLine(0, 7);
    ImGui::TextDisabled("%s", kAppTagline);

    ImGuiIO& io = ImGui::GetIO();
    const float tabY = wp.y + 30.f;
    const float tabH = 26.f;
    const char* tabNames[] = { "Combat", "ESP", "Exploits", "Players", "Config", "Settings" };
    const int tabCount = 6;
    const float wPerTab = ws.x / tabCount;

    static float animX = 0.f;
    float targetX = cfg.mainTab * wPerTab;
    float lerpF = io.DeltaTime * 10.f;
    if (lerpF > 1.f) lerpF = 1.f;
    animX += (targetX - animX) * lerpF;

    {
        ImDrawList* bg = ImGui::GetWindowDrawList();
        const float ulX = wp.x + animX + wPerTab * 0.12f;
        const float ulW = wPerTab * 0.76f;
        ImU32 acc = IM_COL32(
            (int)(cfg.accent.r * 255.f), (int)(cfg.accent.g * 255.f),
            (int)(cfg.accent.b * 255.f), 255);
        bg->AddRectFilled({ ulX, tabY + tabH - 2.f }, { ulX + ulW, tabY + tabH + 1.f }, acc, 2.f);
    }

    for (int i = 0; i < tabCount; ++i) {
        float tx = wp.x + i * wPerTab;
        ImVec2 ts = ImGui::CalcTextSize(tabNames[i]);
        float txtX = tx + (wPerTab - ts.x) * 0.5f;
        float txtY = tabY + (tabH - ts.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText({ txtX, txtY },
            i == cfg.mainTab ? IM_COL32(238, 241, 250, 255) : IM_COL32(130, 136, 156, 255),
            tabNames[i]);
        if (io.MousePos.x >= tx && io.MousePos.x < tx + wPerTab &&
            io.MousePos.y >= tabY && io.MousePos.y < tabY + tabH &&
            ImGui::IsMouseClicked(0))
            cfg.mainTab = i;
    }

    {
        bool mouseDown = io.MouseDown[0];
        static float dragOffX = 0.f, dragOffY = 0.f;
        bool overTop = io.MousePos.x >= wp.x && io.MousePos.x <= wp.x + ws.x &&
                       io.MousePos.y >= wp.y && io.MousePos.y <= wp.y + topH;
        if (overTop && mouseDown && !m_dragging) {
            m_dragging = true;
            dragOffX = io.MousePos.x - s_winX;
            dragOffY = io.MousePos.y - s_winY;
        }
        if (m_dragging && mouseDown) {
            s_winX = io.MousePos.x - dragOffX;
            s_winY = io.MousePos.y - dragOffY;
            ImGui::SetWindowPos({ s_winX, s_winY });
        }
        if (!mouseDown) m_dragging = false;
    }

    ImGui::GetWindowDrawList()->AddLine(
        { wp.x, wp.y + topH }, { wp.x + ws.x, wp.y + topH },
        IM_COL32(255, 255, 255, 10));

    ImGui::SetCursorPos({ 0, topH + 1 });
    float shellW = ws.x;
    float shellH = (std::max)(120.f, ws.y - topH - 1);
    ImAdd::ContentShell("##page", { shellW, shellH });

    switch (cfg.mainTab)
    {
    case 0: PageCombat(cfg); break;
    case 1: PageVisuals(cfg); break;
    case 2: PageCharacter(cfg); break;
    case 3: PagePlayers(cfg, players, camera); break;
    case 4: PageConfig(cfg); break;
    case 5: PageSettings(cfg, globals); break;
    }

    ImAdd::EndContentShell();

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    if (!cfg.menuOpen) { m_menuRectValid = false; SetClickThrough(true); }
    m_engine.Settings().Set(cfg);
}

void Overlay::PageCombat(Config& cfg)
{
    if (cfg.combatSub > 1) cfg.combatSub = 0;
    ImGui::Dummy({ 0, 10 });
    if (ImAdd::SubTab("Aimbot", cfg.combatSub == 0, cfg.accent)) cfg.combatSub = 0;
    ImGui::SameLine(0, 4);
    if (ImAdd::SubTab("Silent", cfg.combatSub == 1, cfg.accent)) cfg.combatSub = 1;
    ImGui::Dummy({ 0, 2 });

    float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 120.f) colH = 120.f;

    if (cfg.combatSub == 0)
    {
        ImAdd::Column("##cL", ImVec2(colW, colH));
        ImGui::Dummy(ImVec2(0, 0));
        ImAdd::Checkbox("Enabled##aim", &cfg.aimEnabled, cfg.accent, &cfg.aimKey, &m_waitAimKey);
        ImAdd::Checkbox("Team Check", &cfg.aimTeamCheck, cfg.accent);
        ImAdd::Checkbox("Health Check", &cfg.aimHealthCheck, cfg.accent);
        ImAdd::Checkbox("Visible Check", &cfg.aimVisibleCheck, cfg.accent);
        ImAdd::Checkbox("Sticky", &cfg.aimSticky, cfg.accent);
        ImAdd::CheckColorRow("Draw FOV##aim", &cfg.drawFov, cfg.accent, &cfg.colFov);
        const char* bones[] = { "Head", "Upper Torso", "Lower Torso", "Arms", "Legs", "Closest to Cursor" };
        int b = (int)cfg.aimBone;
        if (b < 0 || b > 5) b = 0;
        if (ImAdd::Combo("Hit Part", &b, bones, 6)) cfg.aimBone = (AimBone)b;
        ImAdd::Slider("Distance", &cfg.aimDistance, 50.f, 5000.f, cfg.accent, "%.0f");
        ImAdd::Slider("FOV", &cfg.aimFov, 10.f, 600.f, cfg.accent, "%.0f");
        const char* methods[] = { "Camera", "Mouse" };
        int m = (int)cfg.aimMethod;
        if (m < 0 || m > 1) m = 0;
        if (ImAdd::Combo("Method", &m, methods, 2)) cfg.aimMethod = (AimMethod)m;
        ImAdd::EndColumn();
        ImGui::SameLine(0, gap);
        ImAdd::Column("##cR", ImVec2(0, colH));
        ImAdd::Label("Smoothing", cfg.accent);
        const char* sm[] = { "None", "Linear", "Exponential", "Constant", "Smoothstep", "Lock On" };
        int s = (int)cfg.smoothMethod;
        if (s < 0 || s > 5) s = 0;
        if (ImAdd::Combo("Type", &s, sm, 6)) cfg.smoothMethod = (SmoothMethod)s;
        ImAdd::Slider("Smoothness", &cfg.aimSmooth, 0.1f, 30.f, cfg.accent, "%.1f");
        ImAdd::Slider("Sensitivity", &cfg.aimSensitivity, 0.f, 2.f, cfg.accent, "%.2f");
        ImAdd::Slider("Expo", &cfg.aimExpo, 0.5f, 20.f, cfg.accent, "%.1f");
        ImAdd::Slider("Constant Speed", &cfg.aimConstantSpeed, 0.5f, 30.f, cfg.accent, "%.1f");
        if (cfg.smoothMethod == SmoothMethod::LockOn)
        {
            ImAdd::Slider("Strength##lockOn", &cfg.lockOnStrength, 0.1f, 5.f, cfg.accent, "%.1f");
            ImAdd::Slider("Delay##lockOn", &cfg.lockOnDelay, 50.f, 800.f, cfg.accent, "%.0f");
        }
        ImGui::Dummy(ImVec2(0, 6.f));
        ImAdd::Checkbox("Prediction##aimPred", &cfg.aimPredictionEnabled, cfg.accent);
        if (cfg.aimPredictionEnabled)
            ImAdd::Slider("Pred Amount##aim", &cfg.aimPrediction, 0.01f, 0.5f, cfg.accent, "%.2f");
        ImGui::Dummy(ImVec2(0, 8.f));
        ImAdd::Label("Triggerbot", cfg.accent);
        ImAdd::Checkbox("Enabled##trigAim", &cfg.triggerEnabled, cfg.accent);
        ImAdd::Checkbox("Always On##trigAlwaysAim", &cfg.triggerAlwaysOn, cfg.accent);
        if (cfg.triggerEnabled && !cfg.triggerAlwaysOn)
        {
            static bool trigKeyRowAim = true;
            ImAdd::Checkbox("Hold Key##trigKeyAim", &trigKeyRowAim, cfg.accent, &cfg.triggerKey, &m_waitTriggerKey);
            trigKeyRowAim = true;
        }
        if (cfg.triggerEnabled)
        {
            ImAdd::Slider("Delay##trig", &cfg.triggerDelay, 0.f, 300.f, cfg.accent, "%.0f ms");
            ImAdd::Slider("Range##trig", &cfg.triggerRange, 20.f, 800.f, cfg.accent, "%.0f");
        }
        ImAdd::EndColumn();
    }
    else
    {
        ImAdd::Column("##silL", ImVec2(colW, colH));
        ImGui::Dummy(ImVec2(0, 0));
        ImAdd::Checkbox("Enabled##silent", &cfg.silentEnabled, cfg.accent, &cfg.silentKey, &m_waitSilentKey);
        ImAdd::Checkbox("Always On##silent", &cfg.silentAlwaysOn, cfg.accent);
        ImAdd::Checkbox("Team Check", &cfg.silentTeamCheck, cfg.accent);
        ImAdd::Checkbox("Health Check", &cfg.silentHealthCheck, cfg.accent);
        ImAdd::Checkbox("Visible Check", &cfg.silentVisibleCheck, cfg.accent);
        ImAdd::Checkbox("Sticky", &cfg.silentSticky, cfg.accent);
        const char* bones[] = { "Head", "Upper Torso", "Lower Torso", "Arms", "Legs", "Closest to Cursor" };
        int b = (int)cfg.silentBone;
        if (b < 0 || b > 5) b = 0;
        if (ImAdd::Combo("Hit Part", &b, bones, 6)) cfg.silentBone = (AimBone)b;
        ImAdd::Slider("Distance", &cfg.silentDistance, 50.f, 5000.f, cfg.accent, "%.0f");
        ImAdd::Slider("FOV", &cfg.silentFov, 10.f, 600.f, cfg.accent, "%.0f");
        ImAdd::EndColumn();
        ImGui::SameLine(0, gap);
        ImAdd::Column("##silR", ImVec2(0, colH));
        ImAdd::Label("Visuals", cfg.accent);
        ImAdd::CheckColorRow("Draw FOV##sil", &cfg.drawSilentFov, cfg.accent, &cfg.colSilentFov, true);
        ImAdd::CheckColorRow("Aim Line##sil", &cfg.drawAimLine, cfg.accent, &cfg.colAimLine, true);
        ImAdd::Checkbox("Prediction##silPred", &cfg.silentPredictionEnabled, cfg.accent);
        if (cfg.silentPredictionEnabled)
            ImAdd::Slider("Pred Amount##sil", &cfg.silentPrediction, 0.01f, 0.5f, cfg.accent, "%.2f");
        ImGui::Dummy(ImVec2(0, 8.f));
        ImAdd::Label("Triggerbot", cfg.accent);
        ImAdd::Checkbox("Enabled##trigSil", &cfg.triggerEnabled, cfg.accent);
        ImAdd::Checkbox("Always On##trigAlwaysSil", &cfg.triggerAlwaysOn, cfg.accent);
        if (cfg.triggerEnabled && !cfg.triggerAlwaysOn)
        {
            static bool trigKeyRowSil = true;
            ImAdd::Checkbox("Hold Key##trigKeySil", &trigKeyRowSil, cfg.accent, &cfg.triggerKey, &m_waitTriggerKey);
            trigKeyRowSil = true;
        }
        if (cfg.triggerEnabled)
        {
            ImAdd::Slider("Delay##trigSil", &cfg.triggerDelay, 0.f, 300.f, cfg.accent, "%.0f ms");
            ImAdd::Slider("Range##trigSil", &cfg.triggerRange, 20.f, 800.f, cfg.accent, "%.0f");
        }
        ImAdd::EndColumn();
    }
}

void Overlay::PageVisuals(Config& cfg)
{
    if (cfg.visualsSub > 2) cfg.visualsSub = 0;
    if (ImAdd::SubTab("ESP", cfg.visualsSub == 0, cfg.accent)) cfg.visualsSub = 0;
    ImGui::SameLine(0, 4);
    if (ImAdd::SubTab("NPC", cfg.visualsSub == 1, cfg.accent)) cfg.visualsSub = 1;
    ImGui::SameLine(0, 4);
    if (ImAdd::SubTab("World", cfg.visualsSub == 2, cfg.accent)) cfg.visualsSub = 2;
    ImGui::Dummy(ImVec2(0, 2));

    const float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 160.f) colH = 160.f;

    if (cfg.visualsSub == 0)
    {
        ImAdd::Column("##espL", ImVec2(colW, colH));
        ImGui::Dummy(ImVec2(0, 0));
        ImAdd::Checkbox("Enabled##esp", &cfg.espEnabled, cfg.accent);
        ImAdd::CheckColorRow("Box##esp", &cfg.boxes, cfg.accent, &cfg.colBox);
        ImAdd::CheckColorRow("Skeleton##esp", &cfg.skeleton, cfg.accent, &cfg.colSkeleton);
        ImAdd::CheckColorRow("Head Dot##esp", &cfg.headDot, cfg.accent, &cfg.colHeadDot);
        ImAdd::CheckColorRow("Health Bar##esp", &cfg.healthBar, cfg.accent, &cfg.colHealthHigh);
        ImAdd::CheckColorRow("Health Text##esp", &cfg.healthText, cfg.accent, &cfg.colHealthText);
        ImAdd::CheckColorRow("Name##esp", &cfg.names, cfg.accent, &cfg.colName);
        ImAdd::CheckColorRow("Distance##esp", &cfg.distance, cfg.accent, &cfg.colDistance);
        ImAdd::CheckColorRow("Equipped Item##esp", &cfg.equippedItem, cfg.accent, &cfg.colWeapon);
        ImAdd::CheckColorRow("Tracers##esp", &cfg.tracers, cfg.accent, &cfg.colTracer);
        ImAdd::CheckColorRow("Visible Check##esp", &cfg.visCheckEsp, cfg.accent, &cfg.colVisible);
        if (cfg.visCheckEsp)
            ImAdd::ColorRow("##hidCol", &cfg.colHidden, "Not Visible");
        ImAdd::Checkbox("Team Check##esp", &cfg.teamCheckEsp, cfg.accent);
        ImAdd::CheckColorRow("Team Based##esp", &cfg.teamBasedColor, cfg.accent, &cfg.colTeamEnemy);
        if (cfg.teamBasedColor)
            ImAdd::ColorRow("##allyCol", &cfg.colTeamAlly, "Ally");
        ImAdd::EndColumn();
        ImGui::SameLine(0, gap);
        ImAdd::Column("##espR", ImVec2(0, colH));
        ImAdd::Label("Options", cfg.accent);
        ImAdd::Slider("Render Distance", &cfg.renderDistance, 50.f, 5000.f, cfg.accent, "%.0f");
        ImAdd::Slider("Text Scale", &cfg.textScale, 0.50f, 2.00f, cfg.accent, "%.2f");
        {
            const char* bt[] = { "2D", "Corner" };
            int bx = (int)cfg.boxType;
            if (bx < 0 || bx > 1) bx = 0;
            if (ImAdd::Combo("Box Type", &bx, bt, 2)) cfg.boxType = (BoxType)bx;
        }
        ImAdd::CheckColorRow("Box Fill##esp", &cfg.boxFill, cfg.accent, &cfg.colBoxFill, true);
        if (cfg.boxFill)
            ImAdd::Slider("Fill Max Dist", &cfg.boxFillMaxDist, 20.f, 300.f, cfg.accent, "%.0f");
        ImAdd::Slider("Box Padding", &cfg.boxPadding, 0.f, 28.f, cfg.accent, "%.0f");
        {
            const char* nm[] = { "Username", "Display Name", "Both" };
            int n = (int)cfg.nameMode;
            if (n < 0 || n > 2) n = 1;
            if (ImAdd::Combo("Name Type", &n, nm, 3)) cfg.nameMode = (NameMode)n;
        }
        {
            const char* tr[] = { "Bottom", "Top", "Crosshair", "Center" };
            int t = (int)cfg.tracerOrigin;
            if (t < 0 || t > 3) t = 0;
            if (ImAdd::Combo("Tracer Origin", &t, tr, 4)) cfg.tracerOrigin = (TracerOrigin)t;
        }
        ImAdd::EndColumn();
    }
    else if (cfg.visualsSub == 1)
    {
        ImAdd::Column("##npcL", ImVec2(colW, colH));
        ImGui::Dummy(ImVec2(0, 0));
        ImAdd::CheckColorRow("NPC ESP##npc", &cfg.npcEsp, cfg.accent, &cfg.colNpc);
        ImAdd::Checkbox("Glow##npc", &cfg.glowEsp, cfg.accent);
        ImAdd::Checkbox("Self ESP##npc", &cfg.showLocal, cfg.accent);
        ImAdd::EndColumn();
        ImGui::SameLine(0, gap);
        ImAdd::Column("##npcR", ImVec2(0, colH));
        ImAdd::Label("Options", cfg.accent);
        ImAdd::Checkbox("Text Background##npc", &cfg.textBackground, cfg.accent);
        ImAdd::Checkbox("Text Gradient##npc", &cfg.textGradient, cfg.accent);
        ImAdd::Checkbox("Outline##npc", &cfg.textOutline, cfg.accent);
        ImAdd::Checkbox("Box Outline##npc", &cfg.boxOutline, cfg.accent);
        ImAdd::EndColumn();
    }
    else
    {
        const float gapW = 12.f;
        float wL = (ImGui::GetContentRegionAvail().x - gapW) * 0.5f;
        ImAdd::Column("##worldL", ImVec2(wL, colH));
        {
            ImAdd::Label("Lighting", cfg.accent);
            ImAdd::Checkbox("Brightness##wBr", &cfg.worldBrightnessEnabled, cfg.accent);
            if (cfg.worldBrightnessEnabled)
            {
                ImAdd::Slider("Level##wBr", &cfg.worldBrightness, 0.f, 20.f, cfg.accent, "%.2f");
                ImAdd::Slider("Intensity##wBr", &cfg.worldIntensity, 0.f, 3.f, cfg.accent, "%.2f");
                ImAdd::Slider("Exposure##wBr", &cfg.worldExposure, -1.f, 2.f, cfg.accent, "%.2f");
            }
            ImAdd::Divider(cfg.accent);
            ImAdd::Label("Colors", cfg.accent);
            ImAdd::CheckColorRow("Outdoor Ambient", &cfg.worldAmbienceEnabled, cfg.accent, &cfg.worldAmbient);
        }
        ImAdd::EndColumn();
        ImGui::SameLine(0, gapW);
        ImAdd::Column("##worldR", ImVec2(0, colH));
        {
            ImAdd::Label("Time & Atmosphere", cfg.accent);
            ImAdd::Checkbox("Time of Day", &cfg.worldTimeEnabled, cfg.accent);
            if (cfg.worldTimeEnabled)
                ImAdd::Slider("Hour##time", &cfg.worldTimeOfDay, 0.f, 24.f, cfg.accent, "%.2f");
            ImAdd::Divider(cfg.accent);
            ImAdd::CheckColorRow("Fog", &cfg.worldFogEnabled, cfg.accent, &cfg.worldFogColor, true);
            if (cfg.worldFogEnabled)
            {
                ImAdd::Slider("Start##wFog", &cfg.worldFogStart, 0.f, 20000.f, cfg.accent, "%.0f");
                ImAdd::Slider("End##wFog", &cfg.worldFogEnd, 10.f, 20000.f, cfg.accent, "%.0f");
            }
            ImAdd::Divider(cfg.accent);
            ImAdd::Label("Skybox", cfg.accent);
            ImAdd::Checkbox("Enabled##sky", &cfg.skyboxEnabled, cfg.accent);
            if (cfg.skyboxEnabled)
            {
                const char* sb[] = { "Default", "Arctic", "Nebula", "Realistic", "Sunset" };
                int s = cfg.skyboxPreset;
                if (s < 0 || s > 8) s = 0;
                if (ImAdd::Combo("Preset##sky", &s, sb, 5)) cfg.skyboxPreset = s;
                float sc = (float)cfg.skyStarCount;
                if (ImAdd::Slider("Stars##sky", &sc, 0.f, 10000.f, cfg.accent, "%.0f"))
                    cfg.skyStarCount = (int)sc;
            }
        }
        ImAdd::EndColumn();
    }
}

void Overlay::PageCharacter(Config& cfg)
{
    const float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 120.f) colH = 120.f;

    ImAdd::Column("##expL", ImVec2(colW, colH));
    ImAdd::Label("Speed & Motion", cfg.accent);
    ImAdd::Checkbox("Speed Hack", &cfg.speedEnabled, cfg.accent, &cfg.speedKey, &m_waitSpeedKey);
    if (cfg.speedEnabled)
    {
        ImAdd::Slider("Speed Amount", &cfg.speedAmount, 1.f, 500.f, cfg.accent, "%.0f");
    }
    ImAdd::Divider(cfg.accent);
    ImAdd::Checkbox("Flight Mode", &cfg.flightEnabled, cfg.accent, &cfg.flyKey, &m_waitFlyKey);
    if (cfg.flightEnabled)
    {
        ImAdd::Slider("Flight Speed", &cfg.flyAmount, 1.f, 500.f, cfg.accent, "%.0f");
    }
    ImAdd::EndColumn();

    ImGui::SameLine(0, gap);

    ImAdd::Column("##expR", ImVec2(0, colH));
    ImAdd::Label("Jump Boost", cfg.accent);
    ImAdd::Checkbox("Custom Jump Power", &cfg.jumpEnabled, cfg.accent);
    if (cfg.jumpEnabled)
    {
        ImAdd::Slider("Jump Power", &cfg.jumpPower, 1.f, 200.f, cfg.accent, "%.0f");
    }
    ImAdd::Divider(cfg.accent);
    ImAdd::Label("Camera View", cfg.accent);
    ImAdd::Checkbox("Custom FOV", &cfg.camFovEnabled, cfg.accent);
    if (cfg.camFovEnabled)
    {
        ImAdd::Slider("Field of View", &cfg.camFovAmount, 1.f, 150.f, cfg.accent, "%.0f");
    }
    ImAdd::EndColumn();
}

void Overlay::PageSettings(Config& cfg, const GlobalsData& globals)
{
    (void)globals;
    const float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 120.f) colH = 120.f;

    ImAdd::Column("##sL", ImVec2(colW, colH));
    ImAdd::Label("Menu Settings", cfg.accent);
    {
        static bool mk = true;
        ImAdd::Checkbox("Menu Key", &mk, cfg.accent, &cfg.menuKey, &m_waitMenuKey);
        mk = true;
    }
    ImAdd::ColorSq("##acc", &cfg.accent); ImGui::SameLine(0, 8.f); ImGui::Text("Accent Color");
    ImAdd::Checkbox("VSync", &cfg.vsync, cfg.accent);
    ImAdd::Checkbox("Streamproof", &cfg.streamProof, cfg.accent);
    ImAdd::Checkbox("Watermark", &cfg.showWatermark, cfg.accent);

    ImAdd::EndColumn();

    ImGui::SameLine(0, gap);

    ImAdd::Column("##sR", ImVec2(0, colH));

    ImGui::Dummy(ImVec2(0, 8.f));
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(avail.x, 30.f);
        ImGui::InvisibleButton("##unload", size);
        bool unhov = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 ts = ImGui::CalcTextSize("Unload Nyx");
        ImU32 uCol = unhov ? IM_COL32(180, 140, 220, 255) : IM_COL32(140, 100, 190, 255);
        dl->AddText({ pos.x + (size.x - ts.x) * 0.5f, pos.y + (size.y - ts.y) * 0.5f }, uCol, "Unload Nyx");
        if (ImGui::IsItemClicked())
            cfg.unload = true;
    }
    ImAdd::EndColumn();
}

void Overlay::PageConfig(Config& cfg)
{
    const float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 120.f) colH = 120.f;

    ImAdd::Column("##cfgL", ImVec2(colW, colH));
    ImAdd::Label("Config", cfg.accent);

    // Config name input
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 16.f);
    ImGui::InputText("##cfgName", cfg.configName, sizeof(cfg.configName));
    ImGui::PopItemWidth();
    ImGui::Spacing();

    // Save / Load / Delete
    {
        float bw = (ImGui::GetContentRegionAvail().x - 16.f) * 0.33f;
        if (ImAdd::ActionButton("Save", cfg.accent, ImVec2(bw, 26.f)))
            ConfigIO::Save(cfg);
        ImGui::SameLine(0, 4.f);
        if (ImAdd::ActionButton("Load", cfg.accent, ImVec2(bw, 26.f)))
        {
            Color4 savedAccent = cfg.accent;
            ConfigIO::Load(cfg);
            cfg.accent = savedAccent;
        }
        ImGui::SameLine(0, 4.f);
        {
            Color4 red = Color4::From(0.85f, 0.25f, 0.25f, 1.f);
            if (ImAdd::ActionButton("Delete", red, ImVec2(bw, 26.f)))
            {
                ConfigIO::Delete(cfg.configName);
                cfg.configName[0] = 0;
                std::snprintf(cfg.configName, sizeof(cfg.configName), "default");
            }
        }
    }

    ImAdd::EndColumn();
    ImGui::SameLine(0, gap);
    ImAdd::Column("##cfgR", ImVec2(0, colH));
    ImAdd::Label("Saved Configs", cfg.accent);

    // List saved configs (transparent background)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("##cfgList", ImVec2(0, colH - 30.f), ImGuiChildFlags_None);
    {
        auto configs = ConfigIO::List();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (size_t i = 0; i < configs.size(); ++i)
        {
            bool selected = (strcmp(cfg.configName, configs[i].c_str()) == 0);
            ImGui::PushID(static_cast<int>(i));
            const char* name = configs[i].c_str();
            ImVec2 ts = ImGui::CalcTextSize(name);
            float fullW = ImGui::GetContentRegionAvail().x;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 itemMin = pos;
            ImVec2 itemMax = { pos.x + fullW, pos.y + ts.y + 8.f };
            ImGui::InvisibleButton("##cfgitem", ImVec2(fullW, ts.y + 8.f));
            bool hovered = ImGui::IsItemHovered();
            bool clicked = ImGui::IsItemClicked();
            ImU32 textCol = selected
                ? ImAdd::Col(cfg.accent)
                : (hovered
                    ? IM_COL32(
                        (int)((std::min)(1.f, cfg.accent.r * 0.85f + 0.15f) * 255.f),
                        (int)((std::min)(1.f, cfg.accent.g * 0.85f + 0.15f) * 255.f),
                        (int)((std::min)(1.f, cfg.accent.b * 0.85f + 0.15f) * 255.f), 255)
                    : IM_COL32(170, 175, 190, 255));
            float textOffset = 8.f;
            dl->AddText({ pos.x + textOffset, pos.y + 4.f }, textCol, name);
            if (clicked)
            {
                Color4 savedAccent = cfg.accent;
                std::snprintf(cfg.configName, sizeof(cfg.configName), "%s", name);
                ConfigIO::Load(cfg);
                cfg.accent = savedAccent;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImAdd::EndColumn();
}

void Overlay::PagePlayers(Config& cfg, const PlayerFrame& players, const CameraData& camera)
{
    (void)camera;
    const float gap = 12.f;
    float colW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    float colH = ImGui::GetContentRegionAvail().y - 4.f;
    if (colH < 120.f) colH = 120.f;

    static char addBuf[64] = {};
    ImAdd::Column("##plL", ImVec2(colW, colH));
    ImAdd::Label("Players", cfg.accent);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 54.f);
    ImGui::InputText("##addName", addBuf, sizeof(addBuf));
    ImGui::SameLine(0, 4.f);
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 sz = ImVec2(46.f, ImGui::GetTextLineHeightWithSpacing());
        ImGui::InvisibleButton("##add", sz);
        bool hov = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 ts = ImGui::CalcTextSize("Add");
        float aR = cfg.accent.r, aG = cfg.accent.g, aB = cfg.accent.b;
        ImU32 c = hov
            ? IM_COL32((int)(aR*255), (int)(aG*255), (int)(aB*255), 255)
            : IM_COL32((int)(aR*255), (int)(aG*255), (int)(aB*255), 180);
        dl->AddText({ pos.x + (46.f - ts.x) * 0.5f, pos.y + (sz.y - ts.y) * 0.5f }, c, "Add");
        if (ImGui::IsItemClicked() && addBuf[0])
        {
            std::string name(addBuf);
            if (!name.empty())
            {
                ConfigToggleWhitelist(cfg, name);
                addBuf[0] = 0;
            }
        }
    }
    ImGui::Dummy(ImVec2(0, 4.f));
    ImGui::BeginChild("##playerList", ImVec2(0, colH - 68.f), ImGuiChildFlags_Borders);
    for (const auto& e : players.entities)
    {
        if (e.name.empty()) continue;
        bool whitelisted = ConfigIsWhitelisted(cfg, e.name);
        char label[128]{};
        std::snprintf(label, sizeof(label), "%s [%s]", e.displayName.c_str(), e.name.c_str());
        ImGui::PushID(&e);
        ImVec2 ts = ImGui::CalcTextSize(label);
        float fullW = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGui::InvisibleButton("##plrow", ImVec2(fullW, ts.y + 6.f));
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
        {
            std::snprintf(addBuf, sizeof(addBuf), "%s", e.name.c_str());
        }
        ImU32 col = hovered ? IM_COL32(180, 140, 220, 255) : IM_COL32(140, 100, 190, 255);
        dl->AddText({ pos.x + 6.f, pos.y + 3.f }, col, label);
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImAdd::EndColumn();
    ImGui::SameLine(0, gap);
    ImAdd::Column("##plR", ImVec2(0, colH));
    ImAdd::Label("Whitelist", cfg.accent);
    ImGui::BeginChild("##wlList", ImVec2(0, colH * 0.6f), ImGuiChildFlags_Borders);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = cfg.whitelistCount - 1; i >= 0; --i)
    {
        ImGui::PushID(i);
        bool present = false;
        for (const auto& e : players.entities)
            if (_stricmp(cfg.whitelist[i], e.name.c_str()) == 0) { present = true; break; }
        ImVec2 ts = ImGui::CalcTextSize(cfg.whitelist[i]);
        float fullW = ImGui::GetContentRegionAvail().x;
        float rowH = ts.y + 8.f;
        float btnW = 86.f;
        float btnH = rowH - 2.f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 rowMin = pos;
        ImVec2 rowMax = { pos.x + fullW, pos.y + rowH };

        // Row background
        bool hovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);
        ImU32 col = hovered ? IM_COL32(180, 140, 220, 255) : IM_COL32(140, 100, 190, 255);
        dl->AddText({ pos.x + 6.f, pos.y + (rowH - ts.y) * 0.5f }, col, cfg.whitelist[i]);

        // Copy the name BEFORE toggling, since toggle shifts the array
        char nameCopy[48]{};
        std::snprintf(nameCopy, sizeof(nameCopy), "%s", cfg.whitelist[i]);

        // Remove text (no box) — just glow on hover
        ImVec2 btnPos = { pos.x + fullW - btnW - 4.f, pos.y + 1.f };
        ImVec2 btnMax = { btnPos.x + btnW, btnPos.y + btnH };
        bool btnHovered = ImGui::IsMouseHoveringRect(btnPos, btnMax);
        ImVec2 btnTs = ImGui::CalcTextSize("Remove");
        ImU32 rmCol = btnHovered ? IM_COL32(180, 185, 200, 255) : IM_COL32(120, 125, 140, 180);
        dl->AddText({ btnPos.x + (btnW - btnTs.x) * 0.5f, btnPos.y + (btnH - btnTs.y) * 0.5f },
            rmCol, "Remove");

        ImGui::SetCursorScreenPos(btnPos);
        ImGui::InvisibleButton("##wl_rm", ImVec2(btnW, btnH));
        bool removeClicked = ImGui::IsItemClicked();

        // Reserve row space to grow parent boundaries properly
        ImGui::Dummy(ImVec2(fullW, 0.f));
        ImGui::SetCursorScreenPos({ pos.x, pos.y + rowH + 1.f });

        if (removeClicked)
        {
            ConfigToggleWhitelist(cfg, std::string(nameCopy));
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImAdd::EndColumn();
}

void Overlay::DrawCustomCursor()
{
    const ImGuiIO& io = ImGui::GetIO();
    if (!ImGui::IsMousePosValid(&io.MousePos)) return;

    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const ImVec2 m = io.MousePos;
    ImU32 col = IM_COL32(
        (int)(m_lastAccent.r * 255.f), (int)(m_lastAccent.g * 255.f),
        (int)(m_lastAccent.b * 255.f), 220);

    dl->AddCircleFilled(m, 4.f, col);
}

void Overlay::DrawWatermark(const Config& cfg)
{
    if (!cfg.showWatermark) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const float fps = ImGui::GetIO().Framerate;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  |  %s  |  %d FPS", kAppName, kAppTagline, (int)(fps + 0.5f));
    const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, buf);
    const float padX = 10.f, padY = 6.f;
    const float x0 = 12.f, y0 = 10.f;
    const float w = ts.x + padX * 2.f;
    const float h = ts.y + padY * 2.f;
    dl->AddRectFilled({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(10, 11, 15, 210), 6.f);
    dl->AddRect({ x0, y0 }, { x0 + w, y0 + h }, IM_COL32(255, 255, 255, 20), 6.f, 0, 1.f);
    dl->AddRectFilled({ x0, y0 }, { x0 + 2.5f, y0 + h }, ColorToU32(cfg.accent), 6.f, ImDrawFlags_RoundCornersLeft);
    dl->AddText(font, fs, { x0 + padX, y0 + padY }, IM_COL32(230, 234, 244, 245), buf);
}

void Overlay::RenderUi(Config& cfg, const GlobalsData& g, const PlayerFrame& p, const CameraData& c)
{
    DrawMenu(cfg, g, p, c);
}
void Overlay::RenderUi(Config& cfg)
{
    GlobalsData g{}; PlayerFrame p{}; CameraData c{};
    DrawMenu(cfg, g, p, c);
}
