#include "profiling/Profiler.hh"

#include <clean-core/assert.hh>

#include <algorithm>

namespace gamedev::profiling
{
ZoneId Profiler::AddZone(std::string name, Cadence cadence)
{
    CC_ASSERT_MSG(std::ranges::find(mZones, name, &Zone::name) == mZones.end(), "Profiler zone names must be unique.");
    CC_ASSERT_MSG(mZones.size() < std::size_t(UINT16_MAX), "Too many profiler zones.");

    mZones.push_back(Zone{std::move(name), cadence, {}, {}});
    return ZoneId(mZones.size() - 1);
}

void Profiler::Record(ZoneId zone, Milliseconds duration)
{
    auto& entry = mZones[std::size_t(zone)];
    entry.last = duration;
    if (mCapturing)
        entry.samples.push_back(duration.count());
}

void Profiler::StartCapture(std::size_t expectedSamples)
{
    for (auto& zone : mZones)
    {
        zone.samples.clear();
        zone.samples.reserve(expectedSamples);
    }
    mCapturing = true;
}
}
