#pragma once
#include "GameHooks.hh"
#include "GameSettings.hh"
#include "benchmark/Options.hh"
#include "benchmark/Report.hh"

#include <cstdint>

/**
 * Reproducible performance measurements of the running game (`GameDevSS21 --benchmark <scenario>`).
 *
 * A benchmark run starts straight into play with a fixed RNG seed and without input, plays a scripted scenario
 * for a fixed number of 60 Hz ticks, writes the timings to a JSON file and closes the game.
 * `tools/compare_benchmarks.py` compares result files.
 *
 * The timings come from the game's `profiling::Profiler`, captured after a warm-up:
 * every per-tick and per-frame zone (system updates, render, GPU time, frame interval), plus the startup zones.
 *
 * Parts:
 *  - `Options.hh`, `Scenarios.hh`: the settings of a run and the scripted scenarios, which call the `GamelogicSystem`.
 *  - `Session`: drives one run through the `GameHooks` interface.
 *  - `Report.hh`: statistics and the result file, without game or GL dependencies.
 */
namespace gamedev::benchmark
{
/**
 * Drives one benchmark run: warm-up, profiler capture, scenario actions per tick, and the result file at the end.
 *
 * `main` owns it and passes it to `Game` as `GameHooks`, together with `Settings()`.
 */
class Session final : public GameHooks
{
public:
    /// Where a run stands.
    enum class Status : std::uint8_t
    {
        Running,     ///< Not all ticks played yet.
        Completed,   ///< All ticks played and the result file written.
        Aborted,     ///< The game closed before all ticks were played; no result file.
        WriteFailed, ///< All ticks played, but the result file could not be written.
    };

    /**
     * Prepares a run.
     *
     * @param options Settings; `options.scenario` must be set.
     */
    explicit Session(Options options);

    /// How the game must run for this benchmark: seed, no vsync, no input, straight into play, the scenario's daily waves.
    [[nodiscard]] GameSettings Settings() const;

    /// Where the run stands.
    [[nodiscard]] Status GetStatus() const { return mStatus; }

    /// Starts the capture right away if there is no warm-up.
    void OnStarted(profiling::Profiler& profiler) override;

    /**
     * Counts the tick, tracks the instance count, runs the scenario actions due now,
     * and starts or stops the profiler capture at the warm-up and run boundaries.
     *
     * @param context The game after the tick.
     * @return False once all ticks are played.
     */
    [[nodiscard]] bool OnTickEnd(TickContext const& context) override;

    /**
     * Writes the result file and logs a summary if all ticks were played; otherwise marks the run as aborted.
     *
     * @param context The game at the end.
     */
    void OnClosing(ClosingContext const& context) override;

private:
    /// Ticks to play, warm-up included.
    [[nodiscard]] int TotalTicks() const { return mOptions.warmupTicks + mOptions.ticks; }

    /// The report of a finished run.
    [[nodiscard]] Report BuildReport(ClosingContext const& context) const;

    Options mOptions;                 ///< Settings.
    int mPlayedTicks = 0;             ///< Ticks played so far, warm-up included.
    InstanceCounts mInstances;        ///< Live instances during the measurement.
    Status mStatus = Status::Running; ///< Where the run stands.
};
}
