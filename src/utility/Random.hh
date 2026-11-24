#pragma once
#include <cstdint>

namespace gamedev
{
  /// Uniform random number in [0, 1] from a global Mersenne Twister (not thread-safe), randomly seeded unless `SeedRandom` was called.
  float RandomFloat();

  /**
   * Returns a uniform random number in a range (from the same generator).
   *
   * @param from Lower bound.
   * @param to Upper bound.
   * @return Random number in [from, to].
   */
  float RandomFloat(float from, float to);

  /**
   * Restarts the generator of `RandomFloat` with a fixed seed, so that the same calls give the same numbers (used by benchmarks).
   *
   * @param seed Seed for the Mersenne Twister.
   */
  void SeedRandom(std::uint32_t seed);
}
