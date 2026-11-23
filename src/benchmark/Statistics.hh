#pragma once
#include <cstddef>
#include <span>

namespace gamedev::benchmark
{
/// Summary statistics of a series of durations (same unit as the input; zero if there are no samples).
struct Summary
{
    std::size_t samples = 0; ///< Number of values.
    double mean = 0;         ///< Arithmetic mean.
    double median = 0;       ///< 50th percentile.
    double p95 = 0;          ///< 95th percentile.
    double p99 = 0;          ///< 99th percentile.
    double min = 0;          ///< Smallest value.
    double max = 0;          ///< Largest value.
};

/**
 * Computes the summary statistics of `values`; percentiles use the nearest-rank method.
 *
 * @param values Values in any order.
 * @return Their statistics.
 */
[[nodiscard]] Summary Summarize(std::span<double const> values);
}
