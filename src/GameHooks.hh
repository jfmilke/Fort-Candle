#pragma once
#include <cstdint>
#include <string>

namespace gamedev
{
class GamelogicSystem;

namespace profiling
{
class Profiler;
}

/// Where a game stands.
enum class GameOutcome : std::uint8_t
{
    Running, ///< Neither won nor lost yet.
    Won,     ///< All supply route upgrades were built.
    Lost,    ///< The firewood dropped below 0.
};

/// Graphics setup of a run.
struct RunEnvironment
{
    std::string renderer; ///< `GL_RENDERER` string.
    int windowWidth = 0;  ///< Framebuffer width in pixels.
    int windowHeight = 0; ///< Framebuffer height in pixels.
};

/// What `GameHooks::OnTickEnd` gets.
struct TickContext
{
    GamelogicSystem& gamelogic;    ///< Gameplay commands, the same ones the HUD buttons use.
    profiling::Profiler& profiler; ///< The game's profiler; this tick's zones are already recorded.
    std::uint32_t liveInstances;   ///< Live instances after the tick.
};

/// What `GameHooks::OnClosing` gets.
struct ClosingContext
{
    profiling::Profiler const& profiler; ///< The game's profiler.
    RunEnvironment environment;          ///< Graphics setup.
    GameOutcome outcome;                 ///< Where the game stands at shutdown (after the frame in which `OnTickEnd` asked to close).
};

/**
 * Observer interface for code that drives or watches a running game from outside, like a benchmark.
 *
 * `Game` calls it at three points and otherwise does not know who is listening:
 *  1. `OnStarted` at the end of `Game::init`,
 *  2. `OnTickEnd` after every simulation tick played (not while paused or in the menu),
 *  3. `OnClosing` when the game shuts down, while the GL context still exists.
 *
 * Not owned by `Game` (see `Game::Game`); it must outlive the game's main loop.
 */
class GameHooks
{
public:
    virtual ~GameHooks() = default;

    /**
     * Called once the world is set up, before the first tick.
     *
     * @param profiler The game's profiler; the startup zones are recorded.
     */
    virtual void OnStarted(profiling::Profiler& profiler) {}

    /**
     * Called after every simulation tick played.
     *
     * @param context The game after the tick.
     * @return True to keep playing; false closes the window, and the main loop ends after this frame.
     */
    [[nodiscard]] virtual bool OnTickEnd(TickContext const& context) = 0;

    /**
     * Called once when the game shuts down, by request or because the window was closed.
     *
     * After `OnTickEnd` returns false, the main loop still finishes its frame (remaining ticks, UI and render) before this call,
     * so the outcome may differ from the one at that tick.
     *
     * @param context The game at the end.
     */
    virtual void OnClosing(ClosingContext const& context) {}
};
}
