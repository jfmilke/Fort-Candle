#include "benchmark/Scenarios.hh"

#include "ScenarioActions.hh"

#include <algorithm>
#include <array>

namespace gamedev::benchmark
{
namespace
{
void NoActions(ScenarioActions&, int) {}

void Stress(ScenarioActions& actions, int tick)
{
    if (tick == 0)
    {
        actions.SpawnEnemyWave(60);
        actions.SpawnReinforcements(30);
    }
}

// Covers the gameplay paths that create, re-parent and toggle entities: building, recruiting, walls, repairs, supply routes
void Exercise(ScenarioActions& actions, int tick)
{
    if (tick == 0)
    {
        actions.AddGold(400); // for everything below

        for (int i = 0; i < 4; ++i)
        {
            actions.ConstructHome();
            actions.UpgradeProduction();
            actions.BuyWood();
        }

        for (int i = 0; i < 3; ++i)
        {
            actions.AssignArtisan();
            actions.AssignSoldier();
        }
    }

    if (tick == 100 || tick == 200 || tick == 300)
        actions.UpgradeFortifications();

    if (tick == 400 || tick == 500)
        actions.UpgradeSupply();

    if (tick == 600)
    {
        actions.SpawnEnemyWave(80);
        actions.SetDailyWaves(false); // keep the load comparable: only this wave
    }

    if (tick >= 600 && tick % 600 == 0)
    {
        actions.AddGold(20);
        actions.RepairFortifications(); // turns destroyed walls (prototypes) back into live entities
    }
}

constexpr std::array kScenarios = {
    Scenario{"idle", "Start camp without enemy waves: the baseline cost of the world", false, NoActions},
    Scenario{"waves", "Normal play: daily enemy waves and reinforcements (a day lasts 1800 ticks)", true, NoActions},
    Scenario{"stress", "60 monsters and 30 reinforcements at once, plus the daily waves", true, Stress},
    Scenario{"exercise", "Building, recruiting, wall upgrades and repairs, supply routes, an 80-monster wave", true, Exercise},
};
}

std::span<Scenario const> Scenarios() { return kScenarios; }

Scenario const* FindScenario(std::string_view name)
{
    auto const it = std::ranges::find(kScenarios, name, &Scenario::name);
    return it != kScenarios.end() ? &*it : nullptr;
}
}
