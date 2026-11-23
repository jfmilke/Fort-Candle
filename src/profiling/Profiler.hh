#pragma once
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

/**
 * CPU timing of named zones (startup phases, system updates, render, frames).
 *
 * `Game` owns one `Profiler`, registers its zones once and records them every tick and frame, in every mode.
 * The debug UI shows the latest value per zone; a benchmark captures every sample between `StartCapture` and `StopCapture`.
 */
namespace gamedev::profiling
{
using Clock = std::chrono::steady_clock;                        ///< Clock of all measurements (100 ns resolution on Windows).
using Milliseconds = std::chrono::duration<double, std::milli>; ///< Unit of all recorded durations.

/// How often a zone is recorded; decides how the debug UI and benchmark results group it.
enum class Cadence : std::uint8_t
{
    Once,  ///< Once per run, e.g. a startup phase.
    Tick,  ///< Once per 60 Hz simulation tick.
    Frame, ///< Once per rendered frame.
};

/// Handle of a zone, from `Profiler::AddZone`; only valid for the profiler that returned it.
enum class ZoneId : std::uint16_t
{
};

/**
 * Records durations per zone: always the latest one, and every one while a capture runs.
 *
 * Zones are registered up front with `AddZone`, so recording never looks up names and allocates only while capturing.
 * Durations come from `ScopedZone`/`Measure` or are passed to `Record` directly (e.g. frame intervals, GPU times).
 * Not thread-safe: record from the main thread only.
 */
class Profiler
{
public:
    /// A registered zone and its measurements.
    struct Zone
    {
        std::string name;            ///< Unique name, used in the debug UI and as key in benchmark results.
        Cadence cadence;             ///< How often it is recorded.
        Milliseconds last{};         ///< Latest recorded duration (zero before the first one).
        std::vector<double> samples; ///< Durations in milliseconds recorded during the current or last capture.
    };

    /**
     * Registers a zone.
     *
     * @param name Unique name (asserted).
     * @param cadence How often the zone will be recorded.
     * @return Handle for `Record` and `ScopedZone`.
     */
    [[nodiscard]] ZoneId AddZone(std::string name, Cadence cadence);

    /**
     * Records one duration of a zone: sets its latest value and, while capturing, appends it to its samples.
     *
     * @param zone Zone of this profiler.
     * @param duration Measured duration.
     */
    void Record(ZoneId zone, Milliseconds duration);

    /**
     * Calls `fn` and records how long it took.
     *
     * @param zone Zone of this profiler.
     * @param fn Callable without arguments.
     */
    template <class Fn>
    void Measure(ZoneId zone, Fn&& fn);

    /// The zone behind a handle.
    [[nodiscard]] Zone const& GetZone(ZoneId zone) const { return mZones[std::size_t(zone)]; }

    /// All zones, in registration order (index = `ZoneId`).
    [[nodiscard]] std::span<Zone const> Zones() const { return mZones; }

    /**
     * Discards the samples of a previous capture and starts keeping every recorded duration.
     *
     * @param expectedSamples Samples to reserve per zone, to keep allocations out of the measurement.
     */
    void StartCapture(std::size_t expectedSamples = 0);

    /// Stops keeping samples; the captured ones stay available until the next `StartCapture`.
    void StopCapture() { mCapturing = false; }

    /// True between `StartCapture` and `StopCapture`.
    [[nodiscard]] bool IsCapturing() const { return mCapturing; }

private:
    std::vector<Zone> mZones; ///< Registered zones, indexed by `ZoneId`.
    bool mCapturing = false;  ///< Samples are kept.
};

/// Measures the lifetime of the object (RAII) and records it as one duration of a zone.
class ScopedZone
{
public:
    /**
     * Starts the measurement.
     *
     * @param profiler Profiler to record into; must outlive this object.
     * @param zone Zone of `profiler`.
     */
    ScopedZone(Profiler& profiler, ZoneId zone) : mProfiler(profiler), mZone(zone), mStart(Clock::now()) {}

    /// Records the time since construction.
    ~ScopedZone() { mProfiler.Record(mZone, Clock::now() - mStart); }

    ScopedZone(ScopedZone const&) = delete;
    ScopedZone& operator=(ScopedZone const&) = delete;

private:
    Profiler& mProfiler;      ///< Profiler to record into.
    ZoneId mZone;             ///< Measured zone.
    Clock::time_point mStart; ///< Start of the measurement.
};

template <class Fn>
void Profiler::Measure(ZoneId zone, Fn&& fn)
{
    ScopedZone const scope(*this, zone);
    std::forward<Fn>(fn)();
}
}
