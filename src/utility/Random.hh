#pragma once

namespace gamedev
{
  /// Uniform random number in [0, 1] from a global, randomly seeded Mersenne Twister (not thread-safe).
  float RandomFloat();

  /// Uniform random number in [from, to].
  float RandomFloat(float from, float to);
}
