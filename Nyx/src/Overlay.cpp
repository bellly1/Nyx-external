#include "Overlay.hpp"
#include "Math.hpp"
#include "Notify.hpp"
#include "ImAddWidgets.hpp"
#include "Protect.hpp"
#include "Harden.hpp"
#include "Config.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <dwmapi.h>
#include <chrono>
#include <filesystem>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Overlay* Overlay::s_instance = nullptr;

Overlay::Overlay(Memory& m, Engine& e) : m_memory(m), m_engine(e) { s_instance = this; m_status[0] = 0; }
Overlay::~Overlay() { Stop(); Join(); s_instance = nullptr; }

bool Overlay::Start()
{
    m_targetPid = m_memory.GetProcessId();
    if (m_thread.joinable()) return true;
    m_engine.running.store(true, std::memory_order_release);
    m_thread = std::thread(&Overlay::RenderThreadMain, this);
    return true;
}
void Overlay::Stop() { m_engine.running.store(false, std::memory_order_release); if (m_hwnd) PostMessageW(m_hwnd, WM_CLOSE, 0, 0); }
void Overlay::Join() { if (m_thread.joinable()) m_thread.join(); }

void Overlay::LoadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig cfg{};
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;

    std::string fontPath = "Roboto-Medium.ttf";
    {
        wchar_t mod[MAX_PATH]{};
        GetModuleFileNameW(nullptr, mod, MAX_PATH);
        std::filesystem::path p = std::filesystem::path(mod).parent_path() / L"Roboto-Medium.ttf";
        if (std::filesystem::exists(p))
            fontPath = p.string();
    }

    m_fontUi = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 13.0f, &cfg);

    if (!m_fontUi)
        m_fontUi = io.Fonts->AddFontDefault();

    io.FontDefault = m_fontUi;
}
void Overlay::PollHitEvents(const Config& cfg)
{
    const uint64_t seq = m_engine.hitEventSeq.load(std::memory_order_relaxed);
    if (seq == m_lastHitSeq) return;
    m_lastHitSeq = seq;
    if (!cfg.notificationsEnabled) return;
    const int kind = m_engine.hitEventKind.load(std::memory_order_relaxed);
    if (kind != 1)
        Notify::Hit(cfg, "Hit!");
}

bool Overlay::InitializeGraphics()
{
    if (!CreateOverlayWindow() || !CreateDeviceD3D()) return false;
    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGuiStyle& st = ImGui::GetStyle();
    st.AntiAliasedLines = false;
    st.AntiAliasedLinesUseTex = false;
    st.AntiAliasedFill = true;

    LoadFonts();

    Config cfg = m_engine.Settings().Get();
    ImAdd::SetMenuTheme(cfg.menuBgTheme);
    StyleImGui(cfg.accent);
    m_lastAccent = cfg.accent;
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_device, m_context);
    SetClickThrough(!cfg.menuOpen);
    ApplyStreamProof(cfg.streamProof);
    m_graphicsReady = true;

    return true;
}

void Overlay::ShutdownGraphics()
{
    if (!m_graphicsReady && !m_hwnd) return;
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
    m_graphicsReady = false;
}

bool Overlay::CreateOverlayWindow()
{
    m_wc = {}; m_wc.cbSize = sizeof(m_wc); m_wc.style = CS_HREDRAW | CS_VREDRAW;
    m_wc.lpfnWndProc = WndProc; m_wc.hInstance = GetModuleHandleW(nullptr);
    m_wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); m_wc.lpszClassName = L"RobloxiniOverlay";
    if (!RegisterClassExW(&m_wc)) return false;
    m_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        m_wc.lpszClassName, L"robloxini", WS_POPUP, 0, 0, 100, 100, nullptr, nullptr, m_wc.hInstance, nullptr);
    if (!m_hwnd) return false;
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS m = { -1 }; DwmExtendFrameIntoClientArea(m_hwnd, &m);
    return true;
}

bool Overlay::CreateDeviceD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 }; D3D_FEATURE_LEVEL fl{};
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &fl, &m_context)))
        return false;
    CreateRenderTarget(); return true;
}
void Overlay::CleanupDeviceD3D() { CleanupRenderTarget(); if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; } if (m_context) { m_context->Release(); m_context = nullptr; } if (m_device) { m_device->Release(); m_device = nullptr; } }
void Overlay::CreateRenderTarget() { ID3D11Texture2D* bb = nullptr; m_swapChain->GetBuffer(0, IID_PPV_ARGS(&bb)); if (bb) { m_device->CreateRenderTargetView(bb, nullptr, &m_rtv); bb->Release(); } }
void Overlay::CleanupRenderTarget() { if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; } }

void Overlay::SetClickThrough(bool ct)
{
    if (!m_hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
    const bool wantTransparent = ct;
    const bool hasTransparent = (ex & WS_EX_TRANSPARENT) != 0;
    if (m_clickThrough == ct && hasTransparent == wantTransparent) return;
    m_clickThrough = ct;
    if (ct)
    {
        ex |= WS_EX_TRANSPARENT;
        ex |= WS_EX_NOACTIVATE;
    }
    else
    {
        ex &= ~WS_EX_TRANSPARENT;
        ex &= ~WS_EX_NOACTIVATE;
    }
    SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, ex);
}

void Overlay::UpdateMenuClickThrough(const Config& cfg)
{
    if (!m_hwnd) return;

    if (!cfg.menuOpen)
    {
        if (!m_clickThrough) SetClickThrough(true);
        return;
    }

    bool overMenu = true;
    if (m_menuRectValid)
    {
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        overMenu =
            pt.x >= m_menuX && pt.x <= m_menuX + m_menuW &&
            pt.y >= m_menuY && pt.y <= m_menuY + m_menuH;
    }

    const bool wantClickThrough = !overMenu;
    if (m_clickThrough == wantClickThrough) return;
    SetClickThrough(wantClickThrough);
    if (!wantClickThrough)
        SetFocus(m_hwnd);
}

void Overlay::ApplyStreamProof(bool en)
{
    if (!m_hwnd) return;
    if (en)
    {
        if (!SetWindowDisplayAffinity(m_hwnd, 0x11u))
            SetWindowDisplayAffinity(m_hwnd, 0x01u);
        HWND console = GetConsoleWindow();
        if (console)
        {
            ShowWindow(console, SW_HIDE);
            LONG_PTR ex = GetWindowLongPtrW(console, GWL_EXSTYLE);
            ex &= ~WS_EX_APPWINDOW;
            ex |= WS_EX_TOOLWINDOW;
            SetWindowLongPtrW(console, GWL_EXSTYLE, ex);
        }
        FreeConsole();
    }
    else
    {
        SetWindowDisplayAffinity(m_hwnd, 0x00u);
        if (AllocConsole())
        {
            HWND console = GetConsoleWindow();
            if (console)
            {
                ShowWindow(console, SW_SHOW);
                LONG_PTR ex = GetWindowLongPtrW(console, GWL_EXSTYLE);
                ex |= WS_EX_APPWINDOW;
                ex &= ~WS_EX_TOOLWINDOW;
                SetWindowLongPtrW(console, GWL_EXSTYLE, ex);
                SetForegroundWindow(console);
            }
            FILE* fDummy = nullptr;
            freopen_s(&fDummy, "CONOUT$", "w", stdout);
            freopen_s(&fDummy, "CONOUT$", "w", stderr);
            freopen_s(&fDummy, "CONIN$", "r", stdin);
        }
    }
    m_streamProofApplied = en;
}
void Overlay::UpdateTargetWindow()
{
    struct ED { DWORD pid; HWND r; } d{ m_targetPid, nullptr };
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* e = (ED*)lp; DWORD p = 0; GetWindowThreadProcessId(h, &p);
        if (p != e->pid || !IsWindowVisible(h)) return TRUE;
        wchar_t t[256]{}; GetWindowTextW(h, t, 256); if (!t[0]) return TRUE;
        RECT rc{}; if (!GetClientRect(h, &rc) || rc.right < 100 || rc.bottom < 100) return TRUE;
        e->r = h; return FALSE;
    }, (LPARAM)&d);
    if (!d.r) { m_targetHwnd = nullptr; return; }
    m_targetHwnd = d.r;
    RECT c{}; POINT tl{ 0, 0 }; GetClientRect(m_targetHwnd, &c); ClientToScreen(m_targetHwnd, &tl);
    int w = c.right - c.left, h = c.bottom - c.top; if (w <= 0 || h <= 0) return;
    SetWindowPos(m_hwnd, HWND_TOPMOST, tl.x, tl.y, w, h, SWP_NOACTIVATE);
    if (w != m_overlayW || h != m_overlayH) {
        m_overlayW = w; m_overlayH = h; CleanupRenderTarget();
        m_swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0); CreateRenderTarget();
    }
}

void Overlay::HandleInput(Config& cfg)
{
    if (m_waitAimKey || m_waitSilentKey || m_waitTriggerKey || m_waitMenuKey || m_waitSpeedKey || m_waitFlyKey) return;
    bool down = (GetAsyncKeyState(cfg.menuKey) & 0x8000) != 0;
    if (down && !m_menuKeyWasDown) {
        cfg.menuOpen = !cfg.menuOpen;
        if (!cfg.menuOpen) { m_menuRectValid = false; SetClickThrough(true); }
        m_engine.Settings().Set(cfg);
    }
    m_menuKeyWasDown = down;
    UpdateMenuClickThrough(cfg);
}

void Overlay::StyleImGui(const Color4& accent)
{
    ImGui::StyleColorsDark();
    ImAdd::ApplyStyle(accent, ImAdd::MenuTheme());
}

void Overlay::RenderThreadMain()
{
    if (!InitializeGraphics()) { m_engine.running.store(false); return; }
    auto frameStart = std::chrono::steady_clock::now();
    while (m_engine.running.load(std::memory_order_acquire))
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) m_engine.running.store(false);
        }
        if (!m_engine.running.load()) break;

        if (!Protect::LicensedHot())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        {
            static auto s_lastAlive = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();
            if (now - s_lastAlive >= std::chrono::milliseconds(1000))
            {
                s_lastAlive = now;
                if (!m_memory.IsProcessAlive())
                {
                    m_engine.running.store(false, std::memory_order_release);
                    break;
                }
                if (Harden::DebuggerAttachedNow())
                {
                    Protect::CombatUnlocked().store(false, std::memory_order_release);
                    Protect::BurnSeal();
                    m_engine.running.store(false, std::memory_order_release);
                    break;
                }
            }
        }

        {
            static int s_frameCount = 0;
            if (++s_frameCount >= 30) { s_frameCount = 0; UpdateTargetWindow(); }
            else if (!m_targetHwnd) UpdateTargetWindow();
        }
        Config cfg = m_engine.Settings().Get();
        HandleInput(cfg);

        if (cfg.unload) { m_engine.running.store(false, std::memory_order_release); break; }

        if (cfg.streamProof != m_streamProofApplied)
            ApplyStreamProof(cfg.streamProof);

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        if (m_fontUi) ImGui::PushFont(m_fontUi);
        {
            auto g = m_engine.Globals(); auto p = m_engine.Players(); auto c = m_engine.Camera(); auto a = m_engine.AimLock();

            RenderEsp(*p, *c, *a, cfg);
            RenderUi(cfg, *g, *p, *c);
        }
        DrawCustomCursor();
        DrawWatermark(cfg);

        if (m_fontUi) ImGui::PopFont();
        ImGui::Render();
        float clear[4] = { 0,0,0,0 };
        m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
        m_context->ClearRenderTargetView(m_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        const UINT syncInterval = cfg.vsync ? 1 : 0;
        m_swapChain->Present(syncInterval, 0);

        if (!cfg.vsync && cfg.menuOpen) {
            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart).count();
            if (elapsed < 2000)
                YieldProcessor();
            else if (elapsed < 4000)
                std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        frameStart = std::chrono::steady_clock::now();
    }
    ShutdownGraphics();
}

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
    switch (msg)
    {
    case WM_SIZE:
        if (s_instance && s_instance->m_device && wParam != SIZE_MINIMIZED) {
            s_instance->CleanupRenderTarget();
            s_instance->m_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            s_instance->CreateRenderTarget();
            s_instance->m_overlayW = LOWORD(lParam); s_instance->m_overlayH = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY:
        if (s_instance) s_instance->m_engine.running.store(false);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
