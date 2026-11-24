#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace gamedev
{
/**
 * How `Game` starts and runs; fixed for the whole run.
 *
 * The defaults are a normal game. Scripted runs (`benchmark::Session::Settings`) turn off what would make them non-reproducible.
 */
struct GameSettings
{
    std::optional<std::uint32_t> seed; ///< Seed for `SeedRandom`; empty: a random seed.
    bool vsync = true;                 ///< Synchronize rendering to the monitor refresh rate.
    bool inputEnabled = true;          ///< Mouse and keyboard control the camera, selection, orders, shortcuts and the UI.
    bool startPlaying = false;         ///< Start in play, without the main menu and the intro dialog.
    bool dailyWaves = true;            ///< Daily enemy waves and reinforcements at the start (the debug UI's "Hold Spawns" inverts it).
    std::string title = "Fort Candle"; ///< Window title.
};
}
