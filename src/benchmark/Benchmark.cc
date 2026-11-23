#include "benchmark/Benchmark.hh"

#include "systems/GamelogicSystem.hh"

#include <clean-core/assert.hh>
#include <glow/common/log.hh>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <fstream>
#include <numeric>
#include <ostream>
#include <utility>

namespace gamedev::benchmark
{
namespace
{
// Names in result files and summaries, indexed by Channel / Phase
constexpr std::array<std::string_view, std::size_t(Channel::Count)> kChannelNames = {
    "update", "audio", "particles", "lights", "physics", "collision", "environment", "gamelogic", "animation", "cleanup", "render", "frame",
};
constexpr std::array<std::string_view, std::size_t(Phase::Count)> kPhaseNames = {"window", "engine", "assets", "world"};

// Scenarios
// ---------

void NoActions(ScenarioContext&, int) {}

void Stress(ScenarioContext& context, int tick)
{
    if (tick == 0)
    {
        context.gamelogic.SpawnEnemyWave(60);
        context.gamelogic.SpawnReinforcements(30);
    }
}

// Covers the gameplay paths that create, re-parent and toggle entities: building, recruiting, walls, repairs, supply routes
void Exercise(ScenarioContext& context, int tick)
{
    auto& game = context.gamelogic;

    if (tick == 0)
    {
        for (int i = 0; i < 20; ++i)
            game.CRAZY_CHEAT_YOU_WONT_BELIEVE(); // gold for everything below

        for (int i = 0; i < 4; ++i)
        {
            game.ConstructHome();
            game.UpgradeProduction();
            game.AddWood();
        }

        for (int i = 0; i < 3; ++i)
        {
            game.AssignArtisan();
            game.AssignSoldier();
        }
    }

    if (tick == 100 || tick == 200 || tick == 300)
        game.UpgradeFortifications();

    if (tick == 400 || tick == 500)
        game.UpgradeSupply();

    if (tick == 600)
    {
        game.SpawnEnemyWave(80);
        context.holdSpawns = true; // keep the load comparable: only this wave
    }

    if (tick >= 600 && tick % 600 == 0)
    {
        game.CRAZY_CHEAT_YOU_WONT_BELIEVE();
        game.RepairFortifications(); // turns destroyed walls (prototypes) back into live entities
    }
}

constexpr std::array kScenarios = {
    Scenario{"idle", "Start camp without enemy waves: the baseline cost of the world", true, NoActions},
    Scenario{"waves", "Normal play: daily enemy waves and reinforcements (a day lasts 1800 ticks)", false, NoActions},
    Scenario{"stress", "60 monsters and 30 reinforcements at once, plus the daily waves", false, Stress},
    Scenario{"exercise", "Building, recruiting, wall upgrades and repairs, supply routes, an 80-monster wave", false, Exercise},
};

// Command line
// ------------

/// Parses a non-negative integer that fits `T`.
template <class T>
std::optional<T> ParseNumber(std::string_view text)
{
    T value{};
    auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size() || value < T(0))
        return std::nullopt;
    return value;
}

// Statistics and output
// ---------------------

struct Stats
{
    std::size_t samples = 0;
    double mean = 0, median = 0, p95 = 0, p99 = 0, min = 0, max = 0;
};

/// Statistics of `values` (milliseconds); percentiles use the nearest-rank method.
Stats ComputeStats(std::vector<double> values)
{
    Stats stats;
    stats.samples = values.size();
    if (values.empty())
        return stats;

    std::ranges::sort(values);
    auto const rank = [&](double p) { return values[std::size_t(std::ceil(p * double(values.size()))) - 1]; };

    stats.mean = std::accumulate(values.begin(), values.end(), 0.0) / double(values.size());
    stats.median = rank(0.5);
    stats.p95 = rank(0.95);
    stats.p99 = rank(0.99);
    stats.min = values.front();
    stats.max = values.back();
    return stats;
}

/// `text` as a JSON string literal.
std::string JsonString(std::string_view text)
{
    std::string out = "\"";
    for (char const c : text)
    {
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                out += std::format("\\u{:04x}", int(c));
            else
                out += c;
        }
    }
    return out + "\"";
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

/// Last-write time of the executable (UTC), i.e. when it was last linked; "unknown" if it cannot be read.
std::string BuildTime(std::filesystem::path const& executable)
{
    std::error_code error;
    auto const written = std::filesystem::last_write_time(executable, error);
    if (error || executable.empty())
        return "unknown";

    auto const utc = std::chrono::floor<std::chrono::seconds>(std::chrono::clock_cast<std::chrono::system_clock>(written));
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", utc);
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
}

std::expected<LaunchRequest, std::string> ParseCommandLine(std::span<char const* const> args)
{
    LaunchRequest request;
    Options options;

    std::error_code error;
    if (!args.empty())
        options.executable = std::filesystem::absolute(args[0], error);
    bool benchmark = false;
    bool benchmarkSettings = false; // an option that only makes sense with --benchmark

    for (std::size_t i = 1; i < args.size(); ++i)
    {
        // "--name value" and "--name=value"
        std::string_view arg = args[i];
        std::optional<std::string_view> inlineValue;
        if (auto const eq = arg.find('='); arg.starts_with("--") && eq != std::string_view::npos)
        {
            inlineValue = arg.substr(eq + 1);
            arg = arg.substr(0, eq);
        }

        auto const value = [&]() -> std::expected<std::string_view, std::string> {
            if (inlineValue)
                return *inlineValue;
            if (i + 1 < args.size())
                return std::string_view(args[++i]);
            return std::unexpected(std::format("{} needs a value", arg));
        };

        if (arg == "--help" || arg == "-h")
        {
            request.showHelp = true;
        }
        else if (arg == "--benchmark")
        {
            auto const name = value();
            if (!name)
                return std::unexpected(name.error());
            if (!FindScenario(*name))
                return std::unexpected(std::format("unknown benchmark scenario '{}'", *name));

            options.scenario = std::string(*name);
            benchmark = true;
        }
        else if (arg == "--ticks" || arg == "--warmup" || arg == "--seed")
        {
            auto const text = value();
            if (!text)
                return std::unexpected(text.error());

            if (arg == "--seed")
            {
                auto const seed = ParseNumber<std::uint32_t>(*text);
                if (!seed)
                    return std::unexpected(std::format("--seed needs an unsigned 32-bit number, not '{}'", *text));
                options.seed = *seed;
            }
            else
            {
                auto const number = ParseNumber<int>(*text);
                if (!number || (arg == "--ticks" && *number == 0))
                    return std::unexpected(std::format("{} needs a {} number, not '{}'", arg, arg == "--ticks" ? "positive" : "non-negative", *text));
                (arg == "--ticks" ? options.ticks : options.warmupTicks) = *number;
            }
            benchmarkSettings = true;
        }
        else if (arg == "--output")
        {
            auto const path = value();
            if (!path)
                return std::unexpected(path.error());
            options.output = std::filesystem::path(*path);
            benchmarkSettings = true;
        }
        else
        {
            return std::unexpected(std::format("unknown option '{}'", args[i]));
        }
    }

    if (benchmarkSettings && !benchmark)
        return std::unexpected("--ticks, --warmup, --seed and --output need --benchmark <scenario>");

    if (benchmark)
        request.benchmark = std::move(options);
    return request;
}

void PrintUsage(std::ostream& out)
{
    Options const defaults;
    out << "Usage: GameDevSS21 [--benchmark <scenario> [options]]\n"
           "Run from a direct subfolder of the repository root (e.g. bin/), so that ../data exists.\n\n"
           "Without arguments, the game starts normally.\n\n"
           "Benchmark (plays the scenario without input, writes the timings as JSON and exits):\n"
           "  --benchmark <scenario>  Scenario to play (see below)\n"
        << std::format("  --ticks <n>             Measured 60 Hz ticks (default {})\n", defaults.ticks)
        << std::format("  --warmup <n>            Ticks played before measuring (default {})\n", defaults.warmupTicks)
        << std::format("  --seed <n>              Random seed (default {})\n", defaults.seed)
        << "  --output <file>         Result file (default benchmarks/<scenario>-<UTC time>.json)\n"
           "  --help                  Show this text\n\n"
           "Scenarios:\n";
    for (auto const& scenario : Scenarios())
        out << std::format("  {:<10} {}\n", scenario.name, scenario.description);
    out << "\nCompare results: python tools/compare_benchmarks.py <baseline> <candidate>\n";
}

std::span<Scenario const> Scenarios() { return kScenarios; }

Scenario const* FindScenario(std::string_view name)
{
    auto const it = std::ranges::find(kScenarios, name, &Scenario::name);
    return it != kScenarios.end() ? &*it : nullptr;
}

Runner::Runner(Options options) : mOptions(std::move(options))
{
    auto const scenario = FindScenario(mOptions.scenario);
    CC_ASSERT_MSG(scenario, "Unknown benchmark scenario (ParseCommandLine checks the name).");
    mScenario = scenario ? *scenario : kScenarios.front();

    for (auto& samples : mSamples)
        samples.reserve(std::size_t(mOptions.ticks));
}

void Runner::BeginStartup() { mPhaseStart = Clock::now(); }

void Runner::EndPhase(Phase phase)
{
    auto const now = Clock::now();
    mPhases[std::size_t(phase)] = std::chrono::duration<double, std::milli>(now - mPhaseStart).count();
    mPhaseStart = now;
}

void Runner::FrameStarted()
{
    auto const now = Clock::now();
    if (mLastFrame)
        Record(Channel::Frame, now - *mLastFrame);
    mLastFrame = now;
}

void Runner::Record(Channel channel, Clock::duration duration)
{
    if (IsMeasuring() && !mFinished)
        mSamples[std::size_t(channel)].push_back(std::chrono::duration<double, std::milli>(duration).count());
}

bool Runner::EndTick(ScenarioContext& context, std::uint32_t liveInstances)
{
    if (mFinished)
        return false;

    if (IsMeasuring())
    {
        if (mPlayedTicks == mOptions.warmupTicks)
            mInstancesAtStart = liveInstances;
        mInstancesMax = std::max(mInstancesMax, liveInstances);
        mInstancesAtEnd = liveInstances;
    }

    mScenario.onTick(context, mPlayedTicks);
    ++mPlayedTicks;

    return mPlayedTicks >= mOptions.warmupTicks + mOptions.ticks;
}

bool Runner::Finish(RunInfo const& info)
{
    mFinished = true;

    auto const now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    auto path = mOptions.output;
    if (path.empty())
        path = std::filesystem::path("benchmarks") / std::format("{}-{:%Y%m%d-%H%M%S}.json", mOptions.scenario, now);

    std::array<Stats, std::size_t(Channel::Count)> stats;
    for (std::size_t c = 0; c < stats.size(); ++c)
        stats[c] = ComputeStats(mSamples[c]);

    // Result file
    std::string json;
    auto const add = [&json](std::string_view text) { json += text; };

    add("{\n");
    add("  \"format\": \"fort-candle-benchmark\",\n  \"version\": 2,\n");
    add(std::format("  \"scenario\": {},\n", JsonString(mOptions.scenario)));
    add(std::format("  \"seed\": {},\n  \"warmup_ticks\": {},\n  \"ticks\": {},\n", mOptions.seed, mOptions.warmupTicks, mOptions.ticks));
    add(std::format("  \"timestamp\": \"{:%Y-%m-%dT%H:%M:%SZ}\",\n", now));
    add(std::format("  \"build\": {{\"time\": {}, \"config\": {}, \"compiler\": {}, \"platform\": {}}},\n", JsonString(BuildTime(mOptions.executable)),
                    JsonString(BuildConfig()), JsonString(CompilerName()), JsonString(PlatformName())));
    add(std::format("  \"renderer\": {},\n  \"window\": [{}, {}],\n", JsonString(info.renderer), info.windowWidth, info.windowHeight));
    add(std::format("  \"outcome\": {},\n", JsonString(info.outcome)));
    add(std::format("  \"instances\": {{\"start\": {}, \"max\": {}, \"end\": {}}},\n", mInstancesAtStart, mInstancesMax, mInstancesAtEnd));

    double startupTotal = 0;
    add("  \"startup_ms\": {");
    for (std::size_t p = 0; p < mPhases.size(); ++p)
    {
        add(std::format("{}\"{}\": {:.3f}", p ? ", " : "", kPhaseNames[p], mPhases[p]));
        startupTotal += mPhases[p];
    }
    add(std::format(", \"total\": {:.3f}}},\n", startupTotal));

    add("  \"channels_ms\": {\n");
    for (std::size_t c = 0; c < stats.size(); ++c)
    {
        auto const& s = stats[c];
        add(std::format("    \"{}\": {{\"samples\": {}", kChannelNames[c], s.samples));
        if (s.samples > 0)
            add(std::format(", \"mean\": {:.6f}, \"median\": {:.6f}, \"p95\": {:.6f}, \"p99\": {:.6f}, \"min\": {:.6f}, \"max\": {:.6f}", s.mean,
                            s.median, s.p95, s.p99, s.min, s.max));
        add(c + 1 < stats.size() ? "},\n" : "}\n");
    }
    add("  }\n}\n");

    std::error_code error;
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path(), error);

    std::ofstream file(path, std::ios::binary);
    file << json;
    file.close();
    bool const written = !file.fail();
    mWroteResults = written;

    // Summary
    glow::info() << std::format("Benchmark '{}': {} ticks after {} warm-up, seed {}, {} {}, built {}", mOptions.scenario, mOptions.ticks,
                                mOptions.warmupTicks, mOptions.seed, CompilerName(), BuildConfig(), BuildTime(mOptions.executable));
    glow::info() << std::format("  startup {:.0f} ms, instances {} -> {} (max {}), outcome {}", startupTotal, mInstancesAtStart, mInstancesAtEnd,
                                mInstancesMax, info.outcome);
    glow::info() << std::format("  {:<12} {:>8} {:>10} {:>10} {:>10} {:>10}", "ms", "samples", "mean", "median", "p95", "max");
    for (std::size_t c = 0; c < stats.size(); ++c)
    {
        auto const& s = stats[c];
        glow::info() << std::format("  {:<12} {:>8} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f}", kChannelNames[c], s.samples, s.mean, s.median, s.p95, s.max);
    }

    if (written)
        glow::info() << "  results: " << std::filesystem::absolute(path).string();
    else
        glow::error() << "Could not write benchmark results to " << path.string();

    return written;
}
}
