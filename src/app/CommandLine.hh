#pragma once
#include "benchmark/Options.hh"

#include <expected>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>

/// Program startup: the command line of `GameDevSS21`.
namespace gamedev::app
{
/// What `main` should do, according to the command line.
struct LaunchOptions
{
    std::optional<benchmark::Options> benchmark; ///< Set: run this benchmark instead of a normal game.
    bool showHelp = false;                       ///< `--help`: print the usage and exit.
};

/**
 * Parses the command line: `[--benchmark <scenario> [--ticks n] [--warmup n] [--seed n] [--output file]] [--help]`.
 *
 * Options take their value as the next argument.
 *
 * @param args All arguments, including the program name at index 0.
 * @return What to launch, or an error message for unknown options, missing or invalid values and unknown scenarios.
 */
[[nodiscard]] std::expected<LaunchOptions, std::string> ParseCommandLine(std::span<char const* const> args);

/**
 * Prints the command-line usage with the list of benchmark scenarios and the exit codes.
 *
 * @param out Stream to print to.
 */
void PrintUsage(std::ostream& out);
}
