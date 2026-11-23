#pragma once
#include "GameHooks.hh"
#include "benchmark/Statistics.hh"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace gamedev::benchmark
{
/// Version of the result file format; `tools/compare_benchmarks.py` checks it.
inline constexpr int kReportVersion = 3;

/// The build that produced a result.
struct BuildInfo
{
    std::string time;     ///< Last-write time of the executable (UTC, ISO 8601), or "unknown".
    std::string config;   ///< CMake build config, e.g. "RelWithDebInfo".
    std::string compiler; ///< Compiler and version, e.g. "MSVC 19.52.37029".
    std::string platform; ///< Operating system family.
};

/// Live instance counts during the measurement.
struct InstanceCounts
{
    std::uint32_t start = 0; ///< After the first measured tick.
    std::uint32_t max = 0;   ///< Most after any measured tick.
    std::uint32_t end = 0;   ///< After the last tick.
};

/// Duration of a startup phase.
struct StartupPhase
{
    std::string name; ///< Profiler zone name.
    double ms = 0;    ///< Duration in milliseconds.
};

/// Statistics of a profiler zone over the measurement.
struct ZoneSummary
{
    std::string name; ///< Profiler zone name.
    Summary ms;       ///< Statistics of its durations in milliseconds.
};

/// Everything a benchmark result file contains.
struct Report
{
    std::string scenario;                 ///< Scenario name.
    std::uint32_t seed = 0;               ///< RNG seed.
    int warmupTicks = 0;                  ///< Ticks before the measurement.
    int ticks = 0;                        ///< Measured ticks.
    std::chrono::sys_seconds timestamp{}; ///< When the run ended (UTC).
    BuildInfo build;                      ///< Build that ran.
    RunEnvironment environment;           ///< Renderer and window size.
    GameOutcome outcome{};                ///< Game state at the end.
    InstanceCounts instances;             ///< Live instances during the measurement.
    std::vector<StartupPhase> startup;    ///< Startup phases in order.
    std::vector<ZoneSummary> zones;       ///< Per-tick and per-frame zones in profiler order.
};

/**
 * Describes the running build.
 *
 * @param executable Path of the running program; its last-write time stands for the build time.
 * @return Build time, config, compiler and platform.
 */
[[nodiscard]] BuildInfo DescribeBuild(std::filesystem::path const& executable);

/**
 * The default result file path, relative to the working directory.
 *
 * @param scenario Scenario name.
 * @param time Time of the run (UTC).
 * @return `benchmarks/<scenario>-<YYYYmmdd-HHMMSS>.json`.
 */
[[nodiscard]] std::filesystem::path DefaultReportPath(std::string_view scenario, std::chrono::sys_seconds time);

/// The report as a JSON document (format "fort-candle-benchmark", version `kReportVersion`).
[[nodiscard]] std::string ToJson(Report const& report);

/// A human-readable summary of the report, one line per entry (without line breaks).
[[nodiscard]] std::vector<std::string> FormatSummary(Report const& report);

/**
 * Writes a result file, creating missing parent directories.
 *
 * @param contents File contents.
 * @param path Target path.
 * @return The absolute path written, or why it could not be written.
 */
[[nodiscard]] std::expected<std::filesystem::path, std::string> WriteReport(std::string_view contents, std::filesystem::path const& path);
}
