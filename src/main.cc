#include "Game.hh"
#include "benchmark/Benchmark.hh"

#include <filesystem>
#include <iostream>
#include <vector>

// Exit codes: 0 success, 1 bad command line or working directory, 2 benchmark aborted (window closed), 3 benchmark results not written
int main(int argc, char** argv)
{
    std::vector<char const*> const args(argv, argv + argc);
    auto const request = gamedev::benchmark::ParseCommandLine(args);
    if (!request)
    {
        std::cerr << "Error: " << request.error() << "\nRun with --help for the usage.\n";
        return EXIT_FAILURE;
    }

    if (request->showHelp)
    {
        gamedev::benchmark::PrintUsage(std::cout);
        return EXIT_SUCCESS;
    }

    // make sure the executable is run from the correct path
    // (this way we can find all the resources in the data folder)
    if (!std::filesystem::exists("../data"))
    {
        std::cerr << "Working directory must be set to 'bin/'" << std::endl;
        return EXIT_FAILURE;
    }

    // create an instance of the game class
    Game game;
    if (request->benchmark)
        game.setBenchmark(*request->benchmark);

    // run the actual game, i.e. initialize and enter the main loop
    game.run();

    if (auto const benchmark = game.getBenchmark())
    {
        if (!benchmark->IsFinished())
        {
            std::cerr << "Benchmark aborted: the window was closed before all ticks were played." << std::endl;
            return 2;
        }
        if (!benchmark->WroteResults())
            return 3;
    }

    return EXIT_SUCCESS;
}
