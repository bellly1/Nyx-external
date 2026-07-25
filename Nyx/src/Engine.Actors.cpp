#include "Engine.hpp"
#include "Offsets.hpp"
#include <string>

void Engine::ActorsThread()
{
    constexpr double kTargetMs = 350.0;
    constexpr double kOverkillMs = 400.0;

    while (running.load(std::memory_order_acquire))
    {
        Timer timer;
        timer.Start();

        ActorCluster cluster{};
        const auto globals = m_globals.Read();
        if (globals && globals->valid)
        {
            if (globals->overkillMode && globals->workspace)
            {

                DiscoverOverkillPlayers(globals->workspace, cluster.actors);

                cluster.localPlayer = FindLocalModelFromCamera(globals->workspace);
                if (!cluster.localPlayer && globals->localPlayer)
                {
                    const std::string lpName = ReadName(globals->localPlayer);
                    cluster.localPlayer = ResolveCharacter(globals->localPlayer, lpName, globals->workspace);
                    if (!cluster.localPlayer)
                    {

                        for (uintptr_t m : cluster.actors)
                        {
                            if (ReadName(m) == lpName)
                            {
                                cluster.localPlayer = m;
                                break;
                            }
                        }
                    }
                }
                cluster.playersService = globals->players;
                cluster.valid = true;
            }
            else if (globals->players)
            {
                cluster.playersService = globals->players;
                cluster.localPlayer = globals->localPlayer;
                cluster.actors.reserve(32);

                for (const uintptr_t child : ReadChildren(globals->players))
                {
                    if (ReadClassName(child) == "Player")
                        cluster.actors.push_back(child);
                }
                cluster.valid = true;
            }
        }

        cluster.sequence = m_seqActors.fetch_add(1, std::memory_order_relaxed) + 1;
        m_actors.Publish(std::move(cluster));

        const double ms = timer.End();
        m_timings.actors.Record(ms);
        const double budget = (globals && globals->overkillMode) ? kOverkillMs : kTargetMs;
        SleepBudget(budget, ms);
    }
}
