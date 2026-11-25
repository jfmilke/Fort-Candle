#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace gamedev
{
/// Ticks per second of game time: every tick advances the game by 1 / `kTickRate` seconds, whatever `GameSettings::speed` is.
inline constexpr double kTickRate = 60;

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
    bool dialogs = true;               ///< Show the intro, defeat and victory dialogs; without them, the HUD stays.
    bool muted = false;                ///< Listener gain 0: sounds still play, but nothing is heard (the volume settings are ignored).
    std::string title = "Fort Candle"; ///< Window title.

    /**
     * Real-time speed: ticks per second, as a multiple of `kTickRate`.
     *
     * Every tick still advances the game by 1 / `kTickRate` seconds, so the game plays the same, only faster.
     * If a tick and its frame take too long for the speed, the main loop skips time (logged as a frame skip) and the game runs slower.
     */
    double speed = 1;
};
}
