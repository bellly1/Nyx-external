#include "Engine.hpp"
#include "Offsets.hpp"
#include "Math.hpp"
#include <string>

void Engine::GlobalsThread()
{
    constexpr double kTargetMs = 3000.0;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        GlobalsData g{};
        g.moduleBase = m_memory.GetModuleBase(L"RobloxPlayerBeta.exe");
        if (g.moduleBase)
        {
            g.fakeDataModel = m_memory.Read<uintptr_t>(g.moduleBase + Offsets::FakeDataModel::Pointer);
            if (g.fakeDataModel)
            {
                g.dataModel = m_memory.Read<uintptr_t>(g.fakeDataModel + Offsets::FakeDataModel::RealDataModel);
                if (g.dataModel)
                {
                    g.workspace = m_memory.Read<uintptr_t>(g.dataModel + Offsets::DataModel::Workspace);
                    g.placeId = m_memory.Read<int64_t>(g.dataModel + Offsets::DataModel::PlaceId);
                    g.gameId = m_memory.Read<int64_t>(g.dataModel + Offsets::DataModel::GameId);
                    g.gameLoaded = m_memory.Read<uint8_t>(g.dataModel + Offsets::DataModel::GameLoaded) != 0;
                    g.overkillMode = IsOverkillPlace(g.placeId, g.gameId);

                    for (const uintptr_t child : ReadChildren(g.dataModel))
                    {
                        const std::string className = ReadClassName(child);
                        const std::string name = ReadName(child);
                        if (!g.players && (className == "Players" || name == "Players"))
                            g.players = child;
                        if (!g.mouseService && (className == "MouseService" || name == "MouseService"))
                            g.mouseService = child;
                        if (!g.lighting && (className == "Lighting" || name == "Lighting"))
                            g.lighting = child;
                    }

                    if (g.players)
                        g.localPlayer = m_memory.Read<uintptr_t>(g.players + Offsets::Player::LocalPlayer);

                    if (g.workspace)
                    {
                        g.currentCamera = m_memory.Read<uintptr_t>(g.workspace + Offsets::Workspace::CurrentCamera);

                        if (!g.overkillMode)
                            g.overkillMode = DetectOverkillWorkspace(g.workspace);
                    }

                    if (const HWND hwnd = FindRobloxWindow())
                    {
                        g.robloxHwnd = reinterpret_cast<uintptr_t>(hwnd);
                        RECT client{};
                        POINT tl{ 0, 0 };
                        GetClientRect(hwnd, &client);
                        ClientToScreen(hwnd, &tl);
                        g.clientWidth = client.right - client.left;
                        g.clientHeight = client.bottom - client.top;
                        g.clientLeft = tl.x;
                        g.clientTop = tl.y;
                    }

                    g.valid = g.workspace != 0 && (g.players != 0 || g.overkillMode);
                }
            }
        }

        g.sequence = m_seqGlobals.fetch_add(1, std::memory_order_relaxed) + 1;
        m_globals.Publish(std::move(g));

        const double ms = timer.End();
        m_timings.globals.Record(ms);
        SleepBudget(kTargetMs, ms);
    }
}
