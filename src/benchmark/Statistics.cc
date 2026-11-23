#include "benchmark/Statistics.hh"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace gamedev::benchmark
{
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
}
