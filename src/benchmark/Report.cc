#include "benchmark/Report.hh"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <format>
#include <fstream>
#include <numeric>
#include <system_error>

namespace gamedev::benchmark
{
namespace
{
/// `text` as a JSON string literal.
std::string JsonString(std::string_view text)
{
    std::string out = "\"";
    for (char const c : text)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                out += std::format("\\u{:04x}", int(c));
            else
                out += c;
        }
    }
    return out + "\"";
}

constexpr std::string_view OutcomeName(GameOutcome outcome)
{
    switch (outcome)
    {
    case GameOutcome::Won:
        return "won";
    case GameOutcome::Lost:
        return "lost";
    case GameOutcome::Running:
        break;
    }
    return "running";
}

std::string CompilerName()
{
#if defined(__clang__)
    return std::format("Clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    return std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    return std::format("MSVC {}.{}.{}", _MSC_FULL_VER / 10000000, _MSC_FULL_VER / 100000 % 100, _MSC_FULL_VER % 100000);
#else
    return "unknown";
#endif
}

constexpr std::string_view BuildConfig()
{
#ifdef GAMEDEV_BUILD_CONFIG
    return GAMEDEV_BUILD_CONFIG; // set by CMakeLists.txt
#else
    return "unknown";
#endif
}

constexpr std::string_view PlatformName()
{
#if defined(_WIN32)
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "unknown";
#endif
}

/// Last-write time of `executable` (UTC), i.e. when it was last linked; "unknown" if it cannot be read.
std::string BuildTime(std::filesystem::path const& executable)
{
    if (executable.empty())
        return "unknown";

    std::error_code error;
    auto const written = std::filesystem::last_write_time(executable, error);
    if (error)
        return "unknown";

    auto const utc = std::chrono::floor<std::chrono::seconds>(std::chrono::clock_cast<std::chrono::system_clock>(written));
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", utc);
}
}

Summary Summarize(std::span<double const> values)
{
    Summary summary;
    summary.samples = values.size();
    if (values.empty())
        return summary;

    std::vector<double> sorted(values.begin(), values.end());
    std::ranges::sort(sorted);
    auto const rank = [&](double p) { return sorted[std::size_t(std::ceil(p * double(sorted.size()))) - 1]; };

    summary.mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) / double(sorted.size());
    summary.median = rank(0.5);
    summary.p95 = rank(0.95);
    summary.p99 = rank(0.99);
    summary.min = sorted.front();
    summary.max = sorted.back();
    return summary;
}

BuildInfo DescribeBuild(std::filesystem::path const& executable)
{
    return BuildInfo{BuildTime(executable), std::string(BuildConfig()), CompilerName(), std::string(PlatformName())};
}

std::filesystem::path DefaultReportPath(std::string_view scenario, std::chrono::sys_seconds time)
{
    return std::filesystem::path("benchmarks") / std::format("{}-{:%Y%m%d-%H%M%S}.json", scenario, time);
}

std::string ToJson(Report const& report)
{
    std::string json;
    auto const add = [&json](std::string_view text) { json += text; };

    add("{\n");
    add(std::format("  \"format\": \"fort-candle-benchmark\",\n  \"version\": {},\n", kReportVersion));
    add(std::format("  \"scenario\": {},\n", JsonString(report.scenario)));
    add(std::format("  \"seed\": {},\n  \"warmup_ticks\": {},\n  \"ticks\": {},\n", report.seed, report.warmupTicks, report.ticks));
    add(std::format("  \"timestamp\": \"{:%Y-%m-%dT%H:%M:%SZ}\",\n", report.timestamp));
    add(std::format("  \"build\": {{\"time\": {}, \"config\": {}, \"compiler\": {}, \"platform\": {}}},\n", JsonString(report.build.time),
                    JsonString(report.build.config), JsonString(report.build.compiler), JsonString(report.build.platform)));
    add(std::format("  \"renderer\": {},\n  \"window\": [{}, {}],\n", JsonString(report.environment.renderer), report.environment.windowWidth,
                    report.environment.windowHeight));
    add(std::format("  \"outcome\": {},\n", JsonString(OutcomeName(report.outcome))));
    add(std::format("  \"instances\": {{\"start\": {}, \"max\": {}, \"end\": {}}},\n", report.instances.start, report.instances.max, report.instances.end));

    double startupTotal = 0;
    add("  \"startup_ms\": {");
    for (auto const& phase : report.startup)
    {
        add(std::format("{}: {:.3f}, ", JsonString(phase.name), phase.ms));
        startupTotal += phase.ms;
    }
    add(std::format("\"total\": {:.3f}}},\n", startupTotal));

    add("  \"channels_ms\": {");
    for (std::size_t i = 0; i < report.zones.size(); ++i)
    {
        auto const& zone = report.zones[i];
        auto const& s = zone.ms;
        add(std::format("{}\n    {}: {{\"samples\": {}", i ? "," : "", JsonString(zone.name), s.samples));
        if (s.samples > 0)
            add(std::format(", \"mean\": {:.6f}, \"median\": {:.6f}, \"p95\": {:.6f}, \"p99\": {:.6f}, \"min\": {:.6f}, \"max\": {:.6f}", s.mean,
                            s.median, s.p95, s.p99, s.min, s.max));
        add("}");
    }
    add("\n  }\n}\n");
    return json;
}

std::vector<std::string> FormatSummary(Report const& report)
{
    double startupTotal = 0;
    for (auto const& phase : report.startup)
        startupTotal += phase.ms;

    std::vector<std::string> lines;
    lines.push_back(std::format("Benchmark '{}': {} ticks after {} warm-up, seed {}, {} {}, built {}", report.scenario, report.ticks,
                                report.warmupTicks, report.seed, report.build.compiler, report.build.config, report.build.time));
    lines.push_back(std::format("  startup {:.0f} ms, instances {} -> {} (max {}), outcome {}", startupTotal, report.instances.start,
                                report.instances.end, report.instances.max, OutcomeName(report.outcome)));
    lines.push_back(std::format("  {:<12} {:>8} {:>10} {:>10} {:>10} {:>10}", "ms", "samples", "mean", "median", "p95", "max"));
    for (auto const& zone : report.zones)
    {
        auto const& s = zone.ms;
        lines.push_back(std::format("  {:<12} {:>8} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f}", zone.name, s.samples, s.mean, s.median, s.p95, s.max));
    }
    return lines;
}

std::expected<std::filesystem::path, std::string> WriteReport(std::string_view contents, std::filesystem::path const& path)
{
    std::error_code error;
    auto const absolute = std::filesystem::absolute(path, error);
    if (error)
        return std::unexpected(std::format("{}: {}", path.string(), error.message()));

    if (absolute.has_parent_path())
    {
        std::filesystem::create_directories(absolute.parent_path(), error);
        if (error)
            return std::unexpected(std::format("cannot create {}: {}", absolute.parent_path().string(), error.message()));
    }

    std::ofstream file(absolute, std::ios::binary);
    if (!file)
        return std::unexpected(std::format("cannot open {}: {}", absolute.string(), std::generic_category().message(errno)));

    file.write(contents.data(), std::streamsize(contents.size()));
    file.close();
    if (file.fail())
        return std::unexpected(std::format("cannot write {}", absolute.string()));

    return absolute;
}
}
