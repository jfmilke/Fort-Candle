#include "Game.hh"
#include "app/CommandLine.hh"
#include "benchmark/Session.hh"

#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

namespace
{
/// Process exit codes (also listed by `--help`).
enum class ExitCode : int
{
    Success = 0,           ///< Normal game ended, or benchmark results written.
    BadLaunch = 1,         ///< Invalid command line or wrong working directory.
    BenchmarkAborted = 2,  ///< The window was closed before the benchmark finished.
    ResultsNotWritten = 3, ///< The benchmark finished, but its result file could not be written.
};

ExitCode ToExitCode(gamedev::benchmark::Session::Status status)
{
    using Status = gamedev::benchmark::Session::Status;
    switch (status)
    {
    case Status::Completed:
        return ExitCode::Success;
    case Status::WriteFailed:
        return ExitCode::ResultsNotWritten;
    case Status::Running:
    case Status::Aborted:
        break;
    }
    return ExitCode::BenchmarkAborted;
}
}

int main(int argc, char** argv)
{
    std::vector<char const*> const args(argv, argv + argc);
    auto const launch = gamedev::app::ParseCommandLine(args);
    if (!launch)
    {
        std::cerr << "Error: " << launch.error() << "\nRun with --help for the usage.\n";
        return int(ExitCode::BadLaunch);
    }

    if (launch->showHelp)
    {
        gamedev::app::PrintUsage(std::cout);
        return int(ExitCode::Success);
    }

    // all asset paths are relative to a direct subfolder of the repository root, e.g. bin/
    if (!std::filesystem::exists("../data"))
    {
        std::cerr << "Working directory must be set to 'bin/'" << std::endl;
        return int(ExitCode::BadLaunch);
    }

    std::optional<gamedev::benchmark::Session> benchmark;
    if (launch->benchmark)
        benchmark.emplace(*launch->benchmark);

    Game game(benchmark ? benchmark->Settings() : gamedev::GameSettings{}, benchmark ? &*benchmark : nullptr);
    game.run(); // initializes and runs the main loop until the window closes

    return int(benchmark ? ToExitCode(benchmark->GetStatus()) : ExitCode::Success);
}
