#include "benchmark/Scenarios.hh"

#include "systems/GamelogicSystem.hh"

#include <algorithm>
#include <array>

namespace gamedev::benchmark
{
namespace
{
void NoActions(GamelogicSystem&, int) {}

void Stress(GamelogicSystem& gamelogic, int tick)
{
    if (tick == 0)
    {
        gamelogic.SpawnEnemyWave(60);
        gamelogic.SpawnReinforcements(30);
    }
}

// Covers the gameplay paths that create, re-parent and toggle entities: building, recruiting, walls, repairs, supply routes
void Exercise(GamelogicSystem& gamelogic, int tick)
{
    if (tick == 0)
    {
        gamelogic.AddGold(400); // for everything below

        for (int i = 0; i < 4; ++i)
        {
            gamelogic.ConstructHome();
            gamelogic.UpgradeProduction();
            gamelogic.AddWood();
        }

        for (int i = 0; i < 3; ++i)
        {
            gamelogic.AssignArtisan();
            gamelogic.AssignSoldier();
        }
    }

    if (tick == 100 || tick == 200 || tick == 300)
        gamelogic.UpgradeFortifications();

    if (tick == 400 || tick == 500)
        gamelogic.UpgradeSupply();

    if (tick == 600)
        gamelogic.SpawnEnemyWave(80); // the only wave: daily waves are off, to keep the load comparable

    if (tick >= 600 && tick % 600 == 0)
    {
        gamelogic.AddGold(20);
        gamelogic.RepairFortifications(); // turns destroyed walls (prototypes) back into live entities
    }
}

constexpr std::array kScenarios = {
    Scenario{"idle", "Start camp without enemy waves: the baseline cost of the world", false, NoActions},
    Scenario{"waves", "Normal play: daily enemy waves and reinforcements (a day lasts 1800 ticks)", true, NoActions},
    Scenario{"stress", "60 monsters and 30 reinforcements at once, plus the daily waves", true, Stress},
    Scenario{"exercise", "Building, recruiting, wall upgrades and repairs, supply routes, an 80-monster wave", false, Exercise},
};
}

std::span<Scenario const> Scenarios() { return kScenarios; }

Scenario const* FindScenario(std::string_view name)
{
    auto const it = std::ranges::find(kScenarios, name, &Scenario::name);
    return it != kScenarios.end() ? &*it : nullptr;
}
}
