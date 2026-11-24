#include "benchmark/Session.hh"

#include "benchmark/Scenarios.hh"
#include "profiling/Profiler.hh"

#include <clean-core/assert.hh>
#include <glow/common/log.hh>

#include <algorithm>
#include <format>
#include <utility>

namespace gamedev::benchmark
{
Session::Session(Options options) : mOptions(std::move(options))
{
    CC_ASSERT_MSG(mOptions.scenario, "A benchmark session needs a scenario (app::ParseCommandLine sets it).");
}

GameSettings Session::Settings() const
{
    GameSettings settings;
    settings.seed = mOptions.seed;
    settings.vsync = false;        // measure the frame rate the game reaches
    settings.inputEnabled = false; // the mouse over the window must not change what is measured
    settings.startPlaying = true;  // the intro dialog would pause the game
    settings.dailyWaves = mOptions.scenario->dailyWaves;
    settings.title = std::format("Fort Candle - benchmark {}", mOptions.scenario->name);
    return settings;
}

void Session::OnStarted(profiling::Profiler& profiler)
{
    if (mOptions.warmupTicks == 0)
        profiler.StartCapture(std::size_t(mOptions.ticks));
}

bool Session::OnTickEnd(TickContext const& context)
{
    // the main loop may play a few more ticks in the frame that closes the game
    if (mPlayedTicks >= TotalTicks())
        return false;

    if (mPlayedTicks >= mOptions.warmupTicks)
    {
        if (mPlayedTicks == mOptions.warmupTicks)
            mInstances.start = context.liveInstances;
        mInstances.max = std::max(mInstances.max, context.liveInstances);
        mInstances.end = context.liveInstances;
    }

    mOptions.scenario->onTick(context.gamelogic, mPlayedTicks);
    ++mPlayedTicks;

    if (mPlayedTicks == mOptions.warmupTicks)
        context.profiler.StartCapture(std::size_t(mOptions.ticks));

    if (mPlayedTicks < TotalTicks())
        return true;

    context.profiler.StopCapture();
    return false;
}

void Session::OnClosing(ClosingContext const& context)
{
    if (mPlayedTicks < TotalTicks())
    {
        mStatus = Status::Aborted;
        glow::error() << std::format("Benchmark aborted after {} of {} ticks: the window was closed.", mPlayedTicks, TotalTicks());
        return;
    }

    auto const report = BuildReport(context);
    auto const path = mOptions.output.empty() ? DefaultReportPath(report.scenario, report.timestamp) : mOptions.output;
    auto const written = WriteReport(ToJson(report), path);

    for (auto const& line : FormatSummary(report))
        glow::info() << line;

    if (written)
    {
        glow::info() << "  results: " << written->string();
        mStatus = Status::Completed;
    }
    else
    {
        glow::error() << "Could not write the benchmark results: " << written.error();
        mStatus = Status::WriteFailed;
    }
}

Report Session::BuildReport(ClosingContext const& context) const
{
    Report report;
    report.scenario = std::string(mOptions.scenario->name);
    report.seed = mOptions.seed;
    report.warmupTicks = mOptions.warmupTicks;
    report.ticks = mOptions.ticks;
    report.timestamp = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    report.build = DescribeBuild(mOptions.executable);
    report.environment = context.environment;
    report.outcome = context.outcome;
    report.instances = mInstances;

    for (auto const& zone : context.profiler.Zones())
    {
        if (zone.cadence == profiling::Cadence::Once)
            report.startup.push_back({zone.name, zone.last.count()});
        else
            report.zones.push_back({zone.name, Summarize(zone.samples)});
    }
    return report;
}
}
