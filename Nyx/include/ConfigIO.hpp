#pragma once

#include "Config.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace ConfigIO
{
namespace fs = std::filesystem;

inline std::string ExeDir()
{
    char buf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
}

inline std::string ConfigDir()
{
    return (fs::path(ExeDir()) / "configs").string();
}

inline bool EnsureConfigDir()
{
    std::error_code ec;
    fs::create_directories(ConfigDir(), ec);
    return fs::is_directory(ConfigDir(), ec);
}

inline std::string SanitizeName(const char* name)
{
    std::string s = name ? name : "";

    while (!s.empty() && (unsigned char)s.back() <= ' ') s.pop_back();
    size_t i = 0;
    while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
    s = s.substr(i);
    if (s.empty()) s = "default";
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        if (std::isalnum(c) || c == '_' || c == '-' || c == ' ')
            out.push_back(c == ' ' ? '_' : (char)c);
        else if (c == '.')
            out.push_back('_');
    }
    if (out.empty()) out = "default";
    if (out.size() > 48) out.resize(48);
    return out;
}

inline std::string PathFor(const char* name)
{
    EnsureConfigDir();
    return (fs::path(ConfigDir()) / (SanitizeName(name) + ".cfg")).string();
}

inline std::string LegacyPath()
{
    return (fs::path(ExeDir()) / "robloxini.cfg").string();
}

inline std::string Path(const Config& cfg)
{
    return PathFor(cfg.configName);
}

inline std::vector<std::string> List()
{
    std::vector<std::string> out;
    EnsureConfigDir();
    std::error_code ec;

    if (fs::exists(LegacyPath(), ec) && !fs::exists(PathFor("default"), ec))
    {
        fs::copy_file(LegacyPath(), PathFor("default"), ec);
    }
    for (auto& ent : fs::directory_iterator(ConfigDir(), ec))
    {
        if (!ent.is_regular_file(ec)) continue;
        if (ent.path().extension() != ".cfg") continue;
        out.push_back(ent.path().stem().string());
    }
    std::sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
        return _stricmp(a.c_str(), b.c_str()) < 0;
    });
    return out;
}

inline bool Delete(const char* name)
{
    std::error_code ec;
    return fs::remove(PathFor(name), ec) || !ec;
}

inline void WriteColor(std::ofstream& o, const char* k, const Color4& c)
{
    o << k << '=' << c.r << ',' << c.g << ',' << c.b << ',' << c.a << '\n';
}

inline bool ParseColor(const std::string& v, Color4& c)
{
    return sscanf_s(v.c_str(), "%f,%f,%f,%f", &c.r, &c.g, &c.b, &c.a) == 4;
}

inline bool SaveToPath(const Config& cfg, const std::string& path)
{
    std::ofstream o(path, std::ios::trunc);
    if (!o) return false;
    o << "menuKey=" << cfg.menuKey << '\n';
    o << "vsync=" << (cfg.vsync ? 1 : 0) << '\n';
    o << "showWatermark=" << (cfg.showWatermark ? 1 : 0) << '\n';
    o << "showKeybindList=" << (cfg.showKeybindList ? 1 : 0) << '\n';
    o << "keybindListX=" << cfg.keybindListX << '\n';
    o << "keybindListY=" << cfg.keybindListY << '\n';
    o << "streamProof=" << (cfg.streamProof ? 1 : 0) << '\n';
    o << "notificationsEnabled=" << (cfg.notificationsEnabled ? 1 : 0) << '\n';
    o << "notifyHits=" << (cfg.notifyHits ? 1 : 0) << '\n';
    o << "notifyToggles=" << (cfg.notifyToggles ? 1 : 0) << '\n';
    o << "notifSound=" << (cfg.notifSound ? 1 : 0) << '\n';
    o << "notifDuration=" << cfg.notifDuration << '\n';
    o << "menuBgTheme=" << cfg.menuBgTheme << '\n';
    o << "aimEnabled=" << (cfg.aimEnabled ? 1 : 0) << '\n';
    o << "aimAlwaysOn=" << (cfg.aimAlwaysOn ? 1 : 0) << '\n';
    o << "aimMethod=" << static_cast<int>(cfg.aimMethod) << '\n';
    o << "smoothMethod=" << static_cast<int>(cfg.smoothMethod) << '\n';
    o << "aimBone=" << static_cast<int>(cfg.aimBone) << '\n';
    o << "aimKey=" << cfg.aimKey << '\n';
    o << "aimFov=" << cfg.aimFov << '\n';
    o << "aimSmooth=" << cfg.aimSmooth << '\n';
    o << "aimSensitivity=" << cfg.aimSensitivity << '\n';
    o << "aimExpo=" << cfg.aimExpo << '\n';
    o << "aimConstantSpeed=" << cfg.aimConstantSpeed << '\n';
    o << "aimDistance=" << cfg.aimDistance << '\n';
    o << "aimPredictionEnabled=" << (cfg.aimPredictionEnabled ? 1 : 0) << '\n';
    o << "aimPrediction=" << cfg.aimPrediction << '\n';
    o << "lockOnSmoothing=" << (cfg.lockOnSmoothing ? 1 : 0) << '\n';
    o << "lockOnStrength=" << cfg.lockOnStrength << '\n';
    o << "lockOnDelay=" << cfg.lockOnDelay << '\n';
    o << "aimSticky=" << (cfg.aimSticky ? 1 : 0) << '\n';
    o << "aimTeamCheck=" << (cfg.aimTeamCheck ? 1 : 0) << '\n';
    o << "aimHealthCheck=" << (cfg.aimHealthCheck ? 1 : 0) << '\n';
    o << "aimVisibleCheck=" << (cfg.aimVisibleCheck ? 1 : 0) << '\n';
    o << "drawFov=" << (cfg.drawFov ? 1 : 0) << '\n';
    o << "drawAimLine=" << (cfg.drawAimLine ? 1 : 0) << '\n';
    o << "silentEnabled=" << (cfg.silentEnabled ? 1 : 0) << '\n';
    o << "silentAlwaysOn=" << (cfg.silentAlwaysOn ? 1 : 0) << '\n';
    o << "silentKey=" << cfg.silentKey << '\n';
    o << "silentFov=" << cfg.silentFov << '\n';
    o << "silentDistance=" << cfg.silentDistance << '\n';
    o << "silentPredictionEnabled=" << (cfg.silentPredictionEnabled ? 1 : 0) << '\n';
    o << "silentPrediction=" << cfg.silentPrediction << '\n';
    o << "silentBone=" << static_cast<int>(cfg.silentBone) << '\n';
    o << "silentSticky=" << (cfg.silentSticky ? 1 : 0) << '\n';
    o << "silentTeamCheck=" << (cfg.silentTeamCheck ? 1 : 0) << '\n';
    o << "silentHealthCheck=" << (cfg.silentHealthCheck ? 1 : 0) << '\n';
    o << "silentVisibleCheck=" << (cfg.silentVisibleCheck ? 1 : 0) << '\n';
    o << "drawSilentFov=" << (cfg.drawSilentFov ? 1 : 0) << '\n';
    o << "triggerEnabled=" << (cfg.triggerEnabled ? 1 : 0) << '\n';
    o << "triggerAlwaysOn=" << (cfg.triggerAlwaysOn ? 1 : 0) << '\n';
    o << "triggerKey=" << cfg.triggerKey << '\n';
    o << "triggerDelay=" << cfg.triggerDelay << '\n';
    o << "triggerRange=" << cfg.triggerRange << '\n';
    o << "triggerVisCheck=" << (cfg.triggerVisCheck ? 1 : 0) << '\n';


    o << "espEnabled=" << (cfg.espEnabled ? 1 : 0) << '\n';
    o << "boxes=" << (cfg.boxes ? 1 : 0) << '\n';
    o << "boxFill=" << (cfg.boxFill ? 1 : 0) << '\n';
    o << "skeleton=" << (cfg.skeleton ? 1 : 0) << '\n';
    o << "names=" << (cfg.names ? 1 : 0) << '\n';
    o << "healthBar=" << (cfg.healthBar ? 1 : 0) << '\n';
    o << "healthText=" << (cfg.healthText ? 1 : 0) << '\n';
    o << "distance=" << (cfg.distance ? 1 : 0) << '\n';
    o << "tracers=" << (cfg.tracers ? 1 : 0) << '\n';
    o << "npcEsp=" << (cfg.npcEsp ? 1 : 0) << '\n';
    o << "visCheckEsp=" << (cfg.visCheckEsp ? 1 : 0) << '\n';
    o << "bulletTracers=0\n";
    o << "bulletTracerDuration=" << cfg.bulletTracerDuration << '\n';
    o << "bulletTracerThickness=" << cfg.bulletTracerThickness << '\n';
    o << "headDot=" << (cfg.headDot ? 1 : 0) << '\n';
    o << "equippedItem=" << (cfg.equippedItem ? 1 : 0) << '\n';
    o << "showLocal=" << (cfg.showLocal ? 1 : 0) << '\n';
    o << "teamCheckEsp=" << (cfg.teamCheckEsp ? 1 : 0) << '\n';
    o << "teamBasedColor=" << (cfg.teamBasedColor ? 1 : 0) << '\n';
    o << "textBackground=" << (cfg.textBackground ? 1 : 0) << '\n';
    o << "textGradient=" << (cfg.textGradient ? 1 : 0) << '\n';
    o << "textOutline=" << (cfg.textOutline ? 1 : 0) << '\n';
    o << "boxOutline=" << (cfg.boxOutline ? 1 : 0) << '\n';
    o << "glowEsp=" << (cfg.glowEsp ? 1 : 0) << '\n';
    o << "boxFillMaxDist=" << cfg.boxFillMaxDist << '\n';
    o << "rainbowSpeed=" << cfg.rainbowSpeed << '\n';
    o << "ambientStrength=" << cfg.ambientStrength << '\n';
    o << "pulseSpeed=" << cfg.pulseSpeed << '\n';
    o << "boxPadding=" << cfg.boxPadding << '\n';
    o << "textScale=" << cfg.textScale << '\n';
    o << "renderDistance=" << cfg.renderDistance << '\n';
    o << "boxType=" << static_cast<int>(cfg.boxType) << '\n';
    o << "nameMode=" << static_cast<int>(cfg.nameMode) << '\n';
    o << "tracerOrigin=" << static_cast<int>(cfg.tracerOrigin) << '\n';
    o << "colorMode=" << static_cast<int>(cfg.colorMode) << '\n';
    o << "speedEnabled=" << (cfg.speedEnabled ? 1 : 0) << '\n';
    o << "speedAmount=" << cfg.speedAmount << '\n';
    o << "speedKey=" << cfg.speedKey << '\n';
    o << "flightEnabled=" << (cfg.flightEnabled ? 1 : 0) << '\n';
    o << "flyAmount=" << cfg.flyAmount << '\n';
    o << "flyKey=" << cfg.flyKey << '\n';
    o << "jumpEnabled=" << (cfg.jumpEnabled ? 1 : 0) << '\n';
    o << "jumpPower=" << cfg.jumpPower << '\n';
    o << "camFovEnabled=" << (cfg.camFovEnabled ? 1 : 0) << '\n';
    o << "camFovAmount=" << cfg.camFovAmount << '\n';
    o << "worldAmbienceEnabled=" << (cfg.worldAmbienceEnabled ? 1 : 0) << '\n';
    o << "worldFogEnabled=" << (cfg.worldFogEnabled ? 1 : 0) << '\n';
    o << "worldBrightnessEnabled=" << (cfg.worldBrightnessEnabled ? 1 : 0) << '\n';
    o << "worldBrightness=" << cfg.worldBrightness << '\n';
    o << "worldIntensity=" << cfg.worldIntensity << '\n';
    o << "worldExposure=" << cfg.worldExposure << '\n';
    o << "worldFogStart=" << cfg.worldFogStart << '\n';
    o << "worldFogEnd=" << cfg.worldFogEnd << '\n';
    WriteColor(o, "worldAmbient", cfg.worldAmbient);
    WriteColor(o, "worldFogColor", cfg.worldFogColor);
    o << "skyboxEnabled=" << (cfg.skyboxEnabled ? 1 : 0) << '\n';
    o << "skyboxPreset=" << cfg.skyboxPreset << '\n';
    o << "skyStarCount=" << cfg.skyStarCount << '\n';
    o << "worldTimeEnabled=" << (cfg.worldTimeEnabled ? 1 : 0) << '\n';
    o << "worldTimeOfDay=" << cfg.worldTimeOfDay << '\n';
    WriteColor(o, "accent", cfg.accent);
    WriteColor(o, "colBox", cfg.colBox);
    WriteColor(o, "colBoxFill", cfg.colBoxFill);
    WriteColor(o, "colSkeleton", cfg.colSkeleton);
    WriteColor(o, "colName", cfg.colName);
    WriteColor(o, "colHealthText", cfg.colHealthText);
    WriteColor(o, "colHealthHigh", cfg.colHealthHigh);
    WriteColor(o, "colHealthLow", cfg.colHealthLow);
    WriteColor(o, "colFov", cfg.colFov);
    WriteColor(o, "colAimLine", cfg.colAimLine);
    WriteColor(o, "colTracer", cfg.colTracer);
    WriteColor(o, "colVisible", cfg.colVisible);
    WriteColor(o, "colHidden", cfg.colHidden);
    WriteColor(o, "colBulletTracer", cfg.colBulletTracer);
    WriteColor(o, "colDistance", cfg.colDistance);
    WriteColor(o, "colWeapon", cfg.colWeapon);
    WriteColor(o, "colHeadDot", cfg.colHeadDot);
    WriteColor(o, "colNpc", cfg.colNpc);
    return true;
}

inline bool Save(const Config& cfg)
{
    EnsureConfigDir();
    return SaveToPath(cfg, Path(cfg));
}

inline bool SaveAs(Config& cfg, const char* name)
{
    if (name && name[0])
    {
        std::string s = SanitizeName(name);
        std::snprintf(cfg.configName, sizeof(cfg.configName), "%s", s.c_str());
    }
    return Save(cfg);
}

inline bool LoadFromPath(Config& cfg, const std::string& path)
{
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line))
    {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr(0, eq);
        const std::string v = line.substr(eq + 1);
        auto asI = [&](int& o) { o = std::atoi(v.c_str()); };
        auto asF = [&](float& o) { o = static_cast<float>(std::atof(v.c_str())); };
        auto asB = [&](bool& o) { o = (v == "1" || v == "true"); };

        // Section 1: Menu + Combat
        if (k == "menuKey") asI(cfg.menuKey);
        else if (k == "vsync") asB(cfg.vsync);
        else if (k == "showWatermark") asB(cfg.showWatermark);
        else if (k == "showKeybindList") asB(cfg.showKeybindList);
        else if (k == "keybindListX") asF(cfg.keybindListX);
        else if (k == "keybindListY") asF(cfg.keybindListY);
        else if (k == "streamProof") asB(cfg.streamProof);
        else if (k == "notificationsEnabled") asB(cfg.notificationsEnabled);
        else if (k == "notifyHits") asB(cfg.notifyHits);
        else if (k == "notifyToggles") asB(cfg.notifyToggles);
        else if (k == "notifSound") asB(cfg.notifSound);
        else if (k == "notifDuration") asF(cfg.notifDuration);
        else if (k == "menuBgTheme") asI(cfg.menuBgTheme);
        else if (k == "aimEnabled") asB(cfg.aimEnabled);
        else if (k == "aimAlwaysOn") asB(cfg.aimAlwaysOn);
        else if (k == "aimMethod") { int i = 0; asI(i); cfg.aimMethod = static_cast<AimMethod>(i); }
        else if (k == "smoothMethod") { int i = 0; asI(i); cfg.smoothMethod = static_cast<SmoothMethod>(i); }
        else if (k == "aimBone") { int i = 0; asI(i); cfg.aimBone = static_cast<AimBone>(i); }
        else if (k == "aimKey") asI(cfg.aimKey);
        else if (k == "aimFov") asF(cfg.aimFov);
        else if (k == "aimSmooth") asF(cfg.aimSmooth);
        else if (k == "aimSensitivity") asF(cfg.aimSensitivity);
        else if (k == "aimExpo") asF(cfg.aimExpo);
        else if (k == "aimConstantSpeed") asF(cfg.aimConstantSpeed);
        else if (k == "aimDistance") asF(cfg.aimDistance);
        else if (k == "aimPredictionEnabled") asB(cfg.aimPredictionEnabled);
        else if (k == "aimPrediction") asF(cfg.aimPrediction);
        else if (k == "lockOnSmoothing") asB(cfg.lockOnSmoothing);
        else if (k == "lockOnStrength") asF(cfg.lockOnStrength);
        else if (k == "lockOnDelay") asF(cfg.lockOnDelay);
        else if (k == "aimSticky") asB(cfg.aimSticky);
        else if (k == "aimTeamCheck") asB(cfg.aimTeamCheck);
        else if (k == "aimHealthCheck") asB(cfg.aimHealthCheck);
        else if (k == "aimVisibleCheck") asB(cfg.aimVisibleCheck);
        else if (k == "drawFov") asB(cfg.drawFov);
        else if (k == "drawAimLine") asB(cfg.drawAimLine);
        else if (k == "silentEnabled") asB(cfg.silentEnabled);
        else if (k == "silentAlwaysOn") asB(cfg.silentAlwaysOn);
        else if (k == "silentKey") asI(cfg.silentKey);
        else if (k == "silentFov") asF(cfg.silentFov);
        else if (k == "silentDistance") asF(cfg.silentDistance);
        else if (k == "silentPredictionEnabled") asB(cfg.silentPredictionEnabled);
        else if (k == "silentPrediction") asF(cfg.silentPrediction);
        else if (k == "silentBone") { int i = 0; asI(i); cfg.silentBone = static_cast<AimBone>(i); }
        else if (k == "silentSticky") asB(cfg.silentSticky);
        else if (k == "silentTeamCheck") asB(cfg.silentTeamCheck);
        else if (k == "silentHealthCheck") asB(cfg.silentHealthCheck);
        else if (k == "silentVisibleCheck") asB(cfg.silentVisibleCheck);
        else if (k == "drawSilentFov") asB(cfg.drawSilentFov);
        else if (k == "triggerEnabled") asB(cfg.triggerEnabled);
        else if (k == "triggerAlwaysOn") asB(cfg.triggerAlwaysOn);
        else if (k == "triggerKey") asI(cfg.triggerKey);
        else if (k == "triggerDelay") asF(cfg.triggerDelay);
        else if (k == "triggerRange") asF(cfg.triggerRange);
        else if (k == "triggerVisCheck") asB(cfg.triggerVisCheck);

        // Section 2: ESP (new if chain)
        if (k == "espEnabled") asB(cfg.espEnabled);
        else if (k == "boxes") asB(cfg.boxes);
        else if (k == "boxFill") asB(cfg.boxFill);
        else if (k == "skeleton") asB(cfg.skeleton);
        else if (k == "names") asB(cfg.names);
        else if (k == "healthBar") asB(cfg.healthBar);
        else if (k == "healthText") asB(cfg.healthText);
        else if (k == "distance") asB(cfg.distance);
        else if (k == "tracers") asB(cfg.tracers);
        else if (k == "npcEsp") asB(cfg.npcEsp);
        else if (k == "visCheckEsp") asB(cfg.visCheckEsp);
        else if (k == "bulletTracerDuration") asF(cfg.bulletTracerDuration);
        else if (k == "bulletTracerThickness") asF(cfg.bulletTracerThickness);
        else if (k == "headDot") asB(cfg.headDot);
        else if (k == "equippedItem") asB(cfg.equippedItem);
        else if (k == "showLocal") asB(cfg.showLocal);
        else if (k == "teamCheckEsp") asB(cfg.teamCheckEsp);
        else if (k == "teamBasedColor") asB(cfg.teamBasedColor);
        else if (k == "textBackground") asB(cfg.textBackground);
        else if (k == "textGradient") asB(cfg.textGradient);
        else if (k == "textOutline") asB(cfg.textOutline);
        else if (k == "boxOutline") asB(cfg.boxOutline);
        else if (k == "glowEsp") asB(cfg.glowEsp);
        else if (k == "boxFillMaxDist") asF(cfg.boxFillMaxDist);
        else if (k == "rainbowSpeed") asF(cfg.rainbowSpeed);
        else if (k == "ambientStrength") asF(cfg.ambientStrength);
        else if (k == "pulseSpeed") asF(cfg.pulseSpeed);
        else if (k == "boxPadding") asF(cfg.boxPadding);
        else if (k == "textScale") asF(cfg.textScale);
        else if (k == "renderDistance") asF(cfg.renderDistance);
        else if (k == "boxType") { int i = 0; asI(i); cfg.boxType = static_cast<BoxType>(i); }
        else if (k == "nameMode") { int i = 0; asI(i); cfg.nameMode = static_cast<NameMode>(i); }
        else if (k == "tracerOrigin") { int i = 0; asI(i); cfg.tracerOrigin = static_cast<TracerOrigin>(i); }
        else if (k == "colorMode") { int i = 0; asI(i); cfg.colorMode = static_cast<EspColorMode>(i); }

        // Section 3: Movement + Exploits + World + Colors
        if (k == "speedEnabled") asB(cfg.speedEnabled);
        else if (k == "speedAmount") asF(cfg.speedAmount);
        else if (k == "speedKey") asI(cfg.speedKey);
        else if (k == "flightEnabled") asB(cfg.flightEnabled);
        else if (k == "flyAmount") asF(cfg.flyAmount);
        else if (k == "flyKey") asI(cfg.flyKey);
        else if (k == "jumpEnabled") asB(cfg.jumpEnabled);
        else if (k == "jumpPower") asF(cfg.jumpPower);
        else if (k == "camFovEnabled") asB(cfg.camFovEnabled);
        else if (k == "camFovAmount") asF(cfg.camFovAmount);
        else if (k == "worldAmbienceEnabled") asB(cfg.worldAmbienceEnabled);
        else if (k == "worldFogEnabled") asB(cfg.worldFogEnabled);
        else if (k == "worldBrightnessEnabled") asB(cfg.worldBrightnessEnabled);
        else if (k == "worldBrightness") asF(cfg.worldBrightness);
        else if (k == "worldIntensity") asF(cfg.worldIntensity);
        else if (k == "worldExposure") asF(cfg.worldExposure);
        else if (k == "worldFogStart") asF(cfg.worldFogStart);
        else if (k == "worldFogEnd") asF(cfg.worldFogEnd);
        else if (k == "worldAmbient") ParseColor(v, cfg.worldAmbient);
        else if (k == "worldFogColor") ParseColor(v, cfg.worldFogColor);
        else if (k == "skyboxEnabled") asB(cfg.skyboxEnabled);
        else if (k == "skyboxPreset") asI(cfg.skyboxPreset);
        else if (k == "skyStarCount") asI(cfg.skyStarCount);
        else if (k == "worldTimeEnabled") asB(cfg.worldTimeEnabled);
        else if (k == "worldTimeOfDay") asF(cfg.worldTimeOfDay);
        else if (k == "accent") ParseColor(v, cfg.accent);
        else if (k == "colBox") ParseColor(v, cfg.colBox);
        else if (k == "colBoxFill") ParseColor(v, cfg.colBoxFill);
        else if (k == "colSkeleton") ParseColor(v, cfg.colSkeleton);
        else if (k == "colName") ParseColor(v, cfg.colName);
        else if (k == "colHealthText") ParseColor(v, cfg.colHealthText);
        else if (k == "colHealthHigh") ParseColor(v, cfg.colHealthHigh);
        else if (k == "colHealthLow") ParseColor(v, cfg.colHealthLow);
        else if (k == "colFov") ParseColor(v, cfg.colFov);
        else if (k == "colAimLine") ParseColor(v, cfg.colAimLine);
        else if (k == "colTracer") ParseColor(v, cfg.colTracer);
        else if (k == "colVisible") ParseColor(v, cfg.colVisible);
        else if (k == "colHidden") ParseColor(v, cfg.colHidden);
        else if (k == "colBulletTracer") ParseColor(v, cfg.colBulletTracer);
        else if (k == "colDistance") ParseColor(v, cfg.colDistance);
        else if (k == "colWeapon") ParseColor(v, cfg.colWeapon);
        else if (k == "colHeadDot") ParseColor(v, cfg.colHeadDot);
        else if (k == "colNpc") ParseColor(v, cfg.colNpc);
    }
    return true;
}

inline bool Load(Config& cfg)
{

    if (LoadFromPath(cfg, Path(cfg))) return true;
    return LoadFromPath(cfg, LegacyPath());
}

inline bool LoadNamed(Config& cfg, const char* name)
{
    if (!name || !name[0]) return false;
    std::string s = SanitizeName(name);
    if (!LoadFromPath(cfg, PathFor(s.c_str()))) return false;
    std::snprintf(cfg.configName, sizeof(cfg.configName), "%s", s.c_str());
    return true;
}
}

