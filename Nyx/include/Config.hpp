#pragma once

#include <Windows.h>
#include <mutex>
#include <functional>
#include <cstdio>
#include <cstring>
#include <string>

enum class AimMethod : int { Camera = 0, Mouse = 1 };
enum class SmoothMethod : int { None = 0, Linear = 1, Exponential = 2, Constant = 3, Smoothstep = 4, LockOn = 5 };
enum class AimBone : int { Head = 0, UpperTorso = 1, LowerTorso = 2, Arms = 3, Legs = 4, ClosestToCursor = 5 };
enum class TracerOrigin : int { Bottom = 0, Top = 1, Crosshair = 2, Center = 3 };
enum class NameMode : int { Username = 0, DisplayName = 1, Both = 2 };
enum class BoxType : int { Box2D = 0, Corner = 1 };
enum class EspColorMode : int { Static = 0, Rainbow = 1, Health = 2, Distance = 3, Pulse = 4, Ambient = 5 };

struct Color4
{
    float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
    float* Data() { return &r; }
    const float* Data() const { return &r; }
    static Color4 From(float r, float g, float b, float a = 1.f) { return { r, g, b, a }; }
};

inline constexpr const char* kAppName = "Nyx";
inline constexpr const char* kAppTagline = "External";

struct Config
{

    bool menuOpen = true;
    int menuKey = VK_RSHIFT;
    int mainTab = 0;
    int combatSub = 0;
    int visualsSub = 0;
    bool vsync = false;
    bool streamProof = false;
    bool showWatermark = false;
    bool showKeybindList = false;
    float keybindListX = -1.f; // -1 = auto (bottom-left)
    float keybindListY = -1.f;
    bool unload = false; // set to true to exit Nyx
    Color4 accent = Color4::From(1.f, 1.f, 1.f, 1.f); // default white
    char configName[64] = "default";

    bool notificationsEnabled = false;
    bool notifyHits = false;         // legacy field (combat uses notificationsEnabled)
    bool notifyToggles = false;
    bool notifSound = false;         // removed from UI — kept for old configs
    float notifDuration = 2.6f;
    int menuBgTheme = 1;             // 0 Soft · 1 Dark · 2 Darker · 3 Midnight

    bool aimEnabled = false;
    bool aimAlwaysOn = false; // no hold key required when true
    bool aimTeamCheck = false;
    bool aimHealthCheck = false;
    bool aimVisibleCheck = false; // leave OFF unless walls should block aim
    bool aimSticky = true;
    AimMethod aimMethod = AimMethod::Mouse; // more reliable default than camera write
    SmoothMethod smoothMethod = SmoothMethod::None; // None = snappy; Linear was double-smoothed before
    AimBone aimBone = AimBone::Head;
    int aimKey = VK_RBUTTON;
    float aimFov = 350.f;
    float aimSmooth = 1.0f;
    float aimConstantSpeed = 10.f;
    float aimExpo = 6.f;
    float aimSensitivity = 1.0f;
    float aimDistance = 1000.f;
    bool aimPredictionEnabled = false;
    float aimPrediction = 0.08f;
    bool lockOnSmoothing = false;
    float lockOnStrength = 1.0f;
    float lockOnDelay = 200.f;
    bool drawFov = false;
    bool drawAimLine = false;

    bool silentEnabled = false;
    bool silentAlwaysOn = false;
    bool silentTeamCheck = false;
    bool silentHealthCheck = false;
    bool silentVisibleCheck = false;
    bool silentSticky = false;
    int silentKey = VK_RBUTTON;
    float silentFov = 200.f;
    float silentDistance = 1000.f;
    bool silentPredictionEnabled = false;
    float silentPrediction = 0.12f;
    AimBone silentBone = AimBone::Head;
    bool drawSilentFov = false;

    bool triggerEnabled = false;
    bool triggerAlwaysOn = false;
    int triggerKey = VK_XBUTTON2;
    float triggerDelay = 0.f;
    float triggerRange = 150.f;
    bool triggerVisCheck = false;

    bool espEnabled = false;
    bool boxes = false;
    bool boxFill = false;
    float boxFillMaxDist = 90.f;
    bool skeleton = false;
    bool names = false;
    bool healthBar = false;
    bool healthText = false;
    bool distance = false;
    bool tracers = false;

    // ESP visibility colors (replaces bullet tracers)
    bool visCheckEsp = false;
    Color4 colVisible = Color4::From(0.30f, 0.95f, 0.45f, 1.f);   // seen / line of sight
    Color4 colHidden  = Color4::From(0.95f, 0.30f, 0.35f, 1.f);   // behind wall
    bool headDot = false;
    bool equippedItem = false;
    bool showLocal = false;
    bool teamCheckEsp = false;
    bool teamBasedColor = false;
    bool textBackground = false;
    bool textOutline = true;
    bool textGradient = false;
    bool boxOutline = true;
    bool glowEsp = false;
    bool npcEsp = false;
    bool profileEsp = false; // reserved / future avatar ESP

    bool chamsEnabled = false;
    bool chamsBody = false;
    bool chamsHands = false;
    bool chamsGun = false;
    bool chamsGlass = false;
    bool chamsOutline = true;
    float chamsAlpha = 0.35f;
    float textScale = 1.f;
    float boxPadding = 0.f;
    float renderDistance = 2000.f;
    float rainbowSpeed = 0.35f;
    float ambientStrength = 0.55f;
    float pulseSpeed = 2.5f;
    NameMode nameMode = NameMode::DisplayName;
    TracerOrigin tracerOrigin = TracerOrigin::Bottom;
    BoxType boxType = BoxType::Box2D;
    EspColorMode colorMode = EspColorMode::Static;
    Color4 colBox = Color4::From(1.f, 1.f, 1.f, 1.f);
    Color4 colBoxFill = Color4::From(1.f, 1.f, 1.f, 0.10f);
    Color4 colSkeleton = Color4::From(0.90f, 0.92f, 1.f, 1.f);
    Color4 colName = Color4::From(0.98f, 0.98f, 1.f, 1.f);
    Color4 colHealthHigh = Color4::From(0.25f, 0.95f, 0.4f, 1.f); // main health bar color
    Color4 colHealthLow = Color4::From(0.95f, 0.25f, 0.3f, 1.f);  // kept for health color-mode only
    Color4 colHealthText = Color4::From(0.95f, 0.95f, 0.98f, 1.f);
    Color4 colDistance = Color4::From(0.55f, 0.90f, 1.f, 1.f);
    Color4 colWeapon = Color4::From(1.f, 0.78f, 0.35f, 1.f);
    Color4 colTracer = Color4::From(0.30f, 0.85f, 0.95f, 0.90f);
    // legacy fields kept so old configs still load without errors
    bool bulletTracers = false;
    float bulletTracerDuration = 0.55f;
    float bulletTracerThickness = 3.0f;
    Color4 colBulletTracer = Color4::From(0.45f, 0.85f, 1.f, 0.95f);
    Color4 colFov = Color4::From(0.20f, 0.82f, 0.92f, 0.50f);
    Color4 colSilentFov = Color4::From(0.95f, 0.60f, 0.20f, 0.50f);
    Color4 colAimLine = Color4::From(0.95f, 0.40f, 0.45f, 0.90f);
    Color4 colHeadDot = Color4::From(1.f, 1.f, 1.f, 1.f);
    Color4 colNpc = Color4::From(1.f, 0.55f, 0.15f, 1.f);
    Color4 colTeamEnemy = Color4::From(1.f, 0.35f, 0.35f, 1.f);
    Color4 colTeamAlly = Color4::From(0.35f, 0.65f, 1.f, 1.f);
    Color4 colAmbient = Color4::From(0.35f, 0.75f, 1.f, 1.f);
    Color4 colNear = Color4::From(1.f, 0.35f, 0.35f, 1.f);
    Color4 colFar = Color4::From(0.35f, 0.85f, 1.f, 1.f);
    Color4 colChamsBody = Color4::From(0.35f, 0.85f, 1.f, 0.40f);
    Color4 colChamsHands = Color4::From(1.f, 0.55f, 0.20f, 0.55f);
    Color4 colChamsGun = Color4::From(1.f, 0.25f, 0.35f, 0.55f);
    Color4 colChamsGlass = Color4::From(0.55f, 0.95f, 1.f, 0.30f);

    bool speedEnabled = false;
    float speedAmount = 50.f;
    int speedKey = 0;

    bool flightEnabled = false;
    float flyAmount = 50.f;
    int flyKey = 'F';

    bool jumpEnabled = false;
    float jumpPower = 50.f;

    bool camFovEnabled = false;
    float camFovAmount = 70.f;



    // World modules (Visuals → World)
    bool worldAmbienceEnabled = false;
    bool worldFogEnabled = false;
    bool worldBrightnessEnabled = false;
    float worldBrightness = 2.0f;
    float worldIntensity = 1.35f;
    float worldExposure = 0.35f;
    float worldFogStart = 0.f;
    float worldFogEnd = 500.f;
    Color4 worldAmbient = Color4::From(0.75f, 0.55f, 1.f, 1.f);
    Color4 worldFogColor = Color4::From(0.70f, 0.50f, 1.f, 1.f);

    bool skyboxEnabled = false;
    int skyboxPreset = 0;
    int skyStarCount = 3000;

    bool worldTimeEnabled = false;
    float worldTimeOfDay = 14.f;

    static constexpr int kMaxWhitelist = 32;
    char whitelist[kMaxWhitelist][48]{};
    int whitelistCount = 0;
};

inline bool ConfigIsWhitelisted(const Config& cfg, const std::string& name)
{
    if (name.empty() || cfg.whitelistCount <= 0) return false;
    for (int i = 0; i < cfg.whitelistCount && i < Config::kMaxWhitelist; ++i)
    {
        if (cfg.whitelist[i][0] && _stricmp(cfg.whitelist[i], name.c_str()) == 0)
            return true;
    }
    return false;
}

inline bool ConfigToggleWhitelist(Config& cfg, const std::string& name)
{
    if (name.empty()) return false;
    for (int i = 0; i < cfg.whitelistCount && i < Config::kMaxWhitelist; ++i)
    {
        if (_stricmp(cfg.whitelist[i], name.c_str()) == 0)
        {

            for (int j = i; j < cfg.whitelistCount - 1; ++j)
                std::memcpy(cfg.whitelist[j], cfg.whitelist[j + 1], sizeof(cfg.whitelist[0]));
            cfg.whitelist[cfg.whitelistCount - 1][0] = 0;
            --cfg.whitelistCount;
            return false;
        }
    }
    if (cfg.whitelistCount >= Config::kMaxWhitelist) return true;
    std::snprintf(cfg.whitelist[cfg.whitelistCount], sizeof(cfg.whitelist[0]), "%s", name.c_str());
    ++cfg.whitelistCount;
    return true;
}

class ConfigStore
{
public:
    Config Get() const { std::lock_guard<std::mutex> l(m_mutex); return m_cfg; }
    void Set(const Config& c) { std::lock_guard<std::mutex> l(m_mutex); m_cfg = c; }
    void Update(const std::function<void(Config&)>& fn) { std::lock_guard<std::mutex> l(m_mutex); fn(m_cfg); }
private:
    mutable std::mutex m_mutex;
    Config m_cfg{};
};

inline const char* KeyName(int vk)
{
    switch (vk)
    {
    case 0: return "always";
    case VK_LBUTTON: return "m1";
    case VK_RBUTTON: return "m2";
    case VK_MBUTTON: return "m3";
    case VK_XBUTTON1: return "m4";
    case VK_XBUTTON2: return "m5";
    case VK_SHIFT: return "shift";
    case VK_LSHIFT: return "lshift";
    case VK_RSHIFT: return "rshift";
    case VK_CONTROL: return "ctrl";
    case VK_MENU: return "alt";
    case VK_SPACE: return "space";
    case VK_INSERT: return "ins";
    case VK_ESCAPE: return "esc";
    default: break;
    }
    static char buf[16];
    if (vk >= 0x30 && vk <= 0x5A) { buf[0] = (char)(vk | 32); buf[1] = 0; return buf; }
    std::snprintf(buf, sizeof(buf), "0x%02x", vk);
    return buf;
}

inline unsigned int ColorToU32(const Color4& c)
{
    auto ch = [](float v) -> int {
        if (v < 0.f) return 0; if (v > 1.f) return 255; return (int)(v * 255.f + 0.5f);
    };
    return (ch(c.a) << 24) | (ch(c.b) << 16) | (ch(c.g) << 8) | ch(c.r);
}

