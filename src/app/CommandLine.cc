#include "app/CommandLine.hh"

#include "benchmark/Scenarios.hh"

#include <charconv>
#include <format>
#include <ostream>
#include <string_view>
#include <utility>

namespace gamedev::app
{
namespace
{
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
}

std::expected<LaunchOptions, std::string> ParseCommandLine(std::span<char const* const> args)
{
    LaunchOptions launch;
    benchmark::Options options;

    std::error_code error;
    if (!args.empty())
        options.executable = std::filesystem::absolute(args[0], error);
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
            launch.showHelp = true;
        }
        else if (arg == "--benchmark")
        {
            auto const name = value();
            if (!name)
                return std::unexpected(name.error());

            options.scenario = benchmark::FindScenario(*name);
            if (!options.scenario)
                return std::unexpected(std::format("unknown benchmark scenario '{}'", *name));
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

    if (benchmarkSettings && !options.scenario)
        return std::unexpected("--ticks, --warmup, --seed and --output need --benchmark <scenario>");

    if (options.scenario)
        launch.benchmark = std::move(options);
    return launch;
}

void PrintUsage(std::ostream& out)
{
    benchmark::Options const defaults;
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
    for (auto const& scenario : benchmark::Scenarios())
        out << std::format("  {:<10} {}\n", scenario.name, scenario.description);
    out << "\nExit codes: 0 done, 1 bad command line or working directory, 2 benchmark window closed early, 3 benchmark results not written.\n"
           "Compare results: python tools/compare_benchmarks.py <baseline> <candidate>\n";
}
}
