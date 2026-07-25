#pragma once

#include "Memory.hpp"
#include "Engine.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <cstdint>

struct ImFont;

class Overlay
{
public:
    Overlay(Memory& memory, Engine& engine);
    ~Overlay();

    bool Start();
    void Stop();
    void Join();

private:
    void RenderThreadMain();
    bool InitializeGraphics();
    void ShutdownGraphics();
    bool CreateOverlayWindow();
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void UpdateTargetWindow();
    void SetClickThrough(bool clickThrough);
    void ApplyStreamProof(bool enabled);
    void HandleInput(Config& cfg);
    void RenderEsp(const PlayerFrame& players, const CameraData& camera, const AimLockData& aim, const Config& cfg);
    void RenderUi(Config& cfg, const GlobalsData& globals, const PlayerFrame& players, const CameraData& camera);
    void RenderUi(Config& cfg);
    void DrawMenu(Config& cfg, const GlobalsData& globals, const PlayerFrame& players, const CameraData& camera);
    void PageCombat(Config& cfg);
    void PageVisuals(Config& cfg);
    void PageCharacter(Config& cfg);
    void PageSettings(Config& cfg, const GlobalsData& globals);
    void PageConfig(Config& cfg);
    void PagePlayers(Config& cfg, const PlayerFrame& players, const CameraData& camera);
    void StyleImGui(const Color4& accent);
    void UpdateMenuClickThrough(const Config& cfg);
    void LoadFonts();
    void PollHitEvents(const Config& cfg);
    void DrawCustomCursor();
    void DrawWatermark(const Config& cfg);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static Overlay* s_instance;

    Memory& m_memory;
    Engine& m_engine;
    std::thread m_thread;
    HWND m_hwnd = nullptr;
    HWND m_targetHwnd = nullptr;
    WNDCLASSEXW m_wc{};
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    DWORD m_targetPid = 0;
    int m_overlayW = 0;
    int m_overlayH = 0;
    bool m_clickThrough = false;
    bool m_menuKeyWasDown = false;
    bool m_graphicsReady = false;
    bool m_streamProofApplied = false;
    Color4 m_lastAccent{};
    bool m_waitAimKey = false;
    bool m_waitSilentKey = false;
    bool m_waitTriggerKey = false;
    bool m_waitMenuKey = false;
    bool m_waitSpeedKey = false;
    bool m_waitFlyKey = false;
    int m_bindPhase = 0;
    char m_status[64]{};

    float m_menuX = 0.f, m_menuY = 0.f, m_menuW = 0.f, m_menuH = 0.f;
    bool m_menuRectValid = false;
    bool m_dragging = false;
    uint64_t m_lastHitSeq = 0;
    ImFont* m_fontUi = nullptr;
};

