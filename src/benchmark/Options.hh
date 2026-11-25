#pragma once
#include <cstdint>
#include <filesystem>

namespace gamedev::benchmark
{
struct Scenario;

/// Settings of one benchmark run, from the command line (`app::ParseCommandLine`).
struct Options
{
    Scenario const* scenario = nullptr; ///< Scenario to play, from `Scenarios()`; required.
    int warmupTicks = 120;              ///< Played ticks before the measurement starts.
    int ticks = 3600;                   ///< Measured ticks (60 per second of game time).
    double speed = 2;                   ///< `GameSettings::speed`: ticks per real second as a multiple of 60; the game plays the same.
    std::uint32_t seed = 1;             ///< Seed for `SeedRandom`.
    std::filesystem::path output;       ///< Result file; empty: `DefaultReportPath`.
    std::filesystem::path executable;   ///< The running program; its last-write time identifies the build in the results.
};
}
