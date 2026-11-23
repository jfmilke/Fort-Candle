#pragma once
#include <span>
#include <string_view>

namespace gamedev
{
class ScenarioActions;
}

namespace gamedev::benchmark
{
/// A scripted sequence of gameplay actions for a benchmark run.
struct Scenario
{
    std::string_view name;        ///< Name for `--benchmark`.
    std::string_view description; ///< One line for the usage text.
    bool dailyWaves;              ///< Daily enemy waves and reinforcements at the start.

    /**
     * Called after every played tick (warm-up included); triggers the actions due at that tick.
     *
     * @param actions Gameplay commands.
     * @param tick Index of the tick that just ended, counted from the first played tick (0).
     */
    void (*onTick)(ScenarioActions& actions, int tick);
};

/// All scenarios, in the order of the usage text.
[[nodiscard]] std::span<Scenario const> Scenarios();

/// The scenario called `name`, or nullptr.
[[nodiscard]] Scenario const* FindScenario(std::string_view name);
}
