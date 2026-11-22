#pragma once

namespace gamedev
{
  /// Uniform random number in [0, 1] from a global, randomly seeded Mersenne Twister (not thread-safe).
  float RandomFloat();

  /**
   * Returns a uniform random number in a range (from the same generator).
   *
   * @param from Lower bound.
   * @param to Upper bound.
   * @return Random number in [from, to].
   */
  float RandomFloat(float from, float to);
}
