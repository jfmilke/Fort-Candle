#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gamedev
{
class GamelogicSystem;
}

/**
 * Reproducible performance measurements of the running game (`GameDevSS21 --benchmark <scenario>`).
 *
 * A benchmark run starts straight into play with a fixed RNG seed and without input, plays a scripted scenario
 * for a fixed number of 60 Hz ticks, writes the timings to a JSON file and closes the game.
 * `tools/compare_benchmarks.py` compares result files.
 *
 * Measured, after a warm-up:
 *  - every system update per tick, and the whole `Game::update` tick;
 *  - the time spent in `Game::render` and the interval between frames (with vsync off), per frame;
 *  - the startup phases of `Game::init`.
 *
 * Durations come from `std::chrono::steady_clock` (100 ns resolution on Windows), so systems that take well under 1 µs only show that floor.
 */
namespace gamedev::benchmark
{
/// Timed quantities. The order is the order in results and summaries.
enum class Channel : std::uint8_t
{
    Update,      ///< The whole `Game::update` tick (all systems and the wave logic).
    Audio,       ///< `AudioSystem::Update`.
    Particles,   ///< `ParticleSystem::Update`.
    Lights,      ///< `LightSystem::Update`.
    Physics,     ///< `PhysicsSystem::Update`.
    Collision,   ///< `CollisionSystem::Update`.
    Environment, ///< `EnvironmentSystem::Update`.
    Gamelogic,   ///< `GamelogicSystem::Update`.
    Animation,   ///< `AnimationSystem::Update`.
    Cleanup,     ///< `CleanupSystem::Update`.
    Render,      ///< Time spent in `Game::render`: submitting the GL work, plus driver waits when the GPU is the bottleneck.
    Frame,       ///< Time between two frame starts: the frame rate the whole game reaches.

    Count ///< Number of channels; not a channel.
};

/// Startup phases of `Game::init`, in order.
enum class Phase : std::uint8_t
{
    Window, ///< Window, GL context and ImGui.
    Engine, ///< ECS, systems, factories, terrain and render resources.
    Assets, ///< Mesh scan and preset registration.
    World,  ///< Start layout (`.fac` files) and player.

    Count ///< Number of phases; not a phase.
};

/// Settings of one benchmark run, from the command line.
struct Options
{
    std::string scenario;         ///< Name of a scenario in `Scenarios()`.
    int warmupTicks = 120;        ///< Played ticks before the measurement starts.
    int ticks = 3600;             ///< Measured ticks (60 per second of game time).
    std::uint32_t seed = 1;       ///< Seed for `SeedRandom`.
    std::filesystem::path output; ///< Result file; empty: `benchmarks/<scenario>-<UTC time>.json` in the working directory.
};

/// What `main` should do, according to the command line.
struct LaunchRequest
{
    std::optional<Options> benchmark; ///< Set: run this benchmark instead of a normal game.
    bool showHelp = false;            ///< `--help`: print the usage and exit.
};

/**
 * Parses the command line.
 *
 * @param args All arguments, including the program name at index 0.
 * @return What to launch, or an error message for unknown options, missing values and unknown scenarios.
 */
[[nodiscard]] std::expected<LaunchRequest, std::string> ParseCommandLine(std::span<char const* const> args);

/**
 * Prints the command-line usage with the list of scenarios.
 *
 * @param out Stream to print to.
 */
void PrintUsage(std::ostream& out);

/// The parts of the game a scenario drives; provided by `Game`.
struct ScenarioContext
{
    GamelogicSystem& gamelogic; ///< Gameplay actions (spawning, upgrades, ...).
    bool& holdSpawns;           ///< `Game`'s switch for the daily enemy waves and reinforcements.
};

/// A scripted sequence of gameplay actions.
struct Scenario
{
    std::string_view name;        ///< Name for `--benchmark`.
    std::string_view description; ///< One line for the usage text.
    bool holdSpawns;              ///< Suppress the daily waves from the start.

    /**
     * Called after every played tick (warm-up included); triggers the actions due at that tick.
     *
     * @param context The game.
     * @param tick Index of the tick that just ended, counted from the first played tick (0).
     */
    void (*onTick)(ScenarioContext& context, int tick);
};

/// All scenarios.
[[nodiscard]] std::span<Scenario const> Scenarios();

/// The scenario called `name`, or nullptr.
[[nodiscard]] Scenario const* FindScenario(std::string_view name);

/// Facts about the run that `Game` knows and the results report.
struct RunInfo
{
    std::string renderer; ///< `GL_RENDERER` string.
    int windowWidth = 0;  ///< Framebuffer width in pixels.
    int windowHeight = 0; ///< Framebuffer height in pixels.
    std::string outcome;  ///< Game state at the end: "running", "lost" or "won".
};

/**
 * Drives one benchmark run: collects the samples, runs the scenario and writes the results.
 *
 * Owned by `Game` (only in benchmark mode), which reports startup phases, frames, ticks and timings to it.
 * Samples are only kept after the warm-up; startup phases are always kept.
 */
class Runner
{
public:
    using Clock = std::chrono::steady_clock;

    /**
     * Prepares a run.
     *
     * @param options Settings; `options.scenario` must name an existing scenario.
     */
    explicit Runner(Options options);

    /// The settings of this run.
    [[nodiscard]] Options const& GetOptions() const { return mOptions; }

    /// The scenario of this run.
    [[nodiscard]] Scenario const& GetScenario() const { return mScenario; }

    /// Starts the startup clock; call at the beginning of `Game::init`.
    void BeginStartup();

    /**
     * Ends a startup phase; its duration is the time since the previous phase ended (or `BeginStartup`).
     *
     * @param phase The phase that just ended.
     */
    void EndPhase(Phase phase);

    /// Records the interval since the previous frame start (`Channel::Frame`); call at every frame start.
    void FrameStarted();

    /**
     * Adds a sample, if the measurement has started.
     *
     * @param channel What was timed.
     * @param duration How long it took.
     */
    void Record(Channel channel, Clock::duration duration);

    /**
     * Ends a played tick: runs the scenario actions due now and tracks the instance count.
     *
     * @param context The game, for the scenario.
     * @param liveInstances Number of live instances after the tick.
     * @return True when all ticks are played; then call `Finish`.
     */
    bool EndTick(ScenarioContext& context, std::uint32_t liveInstances);

    /**
     * Writes the result file and prints a summary.
     *
     * @param info Facts about the run from `Game`.
     * @return False if the result file could not be written (the summary is printed anyway).
     */
    bool Finish(RunInfo const& info);

    /// True once `Finish` was called.
    [[nodiscard]] bool IsFinished() const { return mFinished; }

    /// True if `Finish` wrote the result file.
    [[nodiscard]] bool WroteResults() const { return mWroteResults; }

private:
    /// True while samples are kept (after the warm-up).
    [[nodiscard]] bool IsMeasuring() const { return mPlayedTicks >= mOptions.warmupTicks; }

    Options mOptions;           ///< Settings.
    Scenario mScenario{};       ///< Copy of the scenario.
    int mPlayedTicks = 0;       ///< Ticks played so far, warm-up included.
    bool mFinished = false;     ///< `Finish` was called; later samples and ticks are ignored.
    bool mWroteResults = false; ///< `Finish` wrote the result file.

    std::array<std::vector<double>, std::size_t(Channel::Count)> mSamples; ///< Durations in milliseconds per channel.
    std::array<double, std::size_t(Phase::Count)> mPhases{};               ///< Startup phase durations in milliseconds.

    Clock::time_point mPhaseStart;               ///< End of the previous startup phase.
    std::optional<Clock::time_point> mLastFrame; ///< Start of the previous frame.

    std::uint32_t mInstancesAtStart = 0; ///< Live instances when the measurement started.
    std::uint32_t mInstancesMax = 0;     ///< Most live instances during the measurement.
    std::uint32_t mInstancesAtEnd = 0;   ///< Live instances after the last tick.
};
}
