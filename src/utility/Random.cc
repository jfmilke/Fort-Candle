#include "utility/Random.hh"
#include <cmath>
#include <random>

namespace
{
// Randomly seeded on first use; SeedRandom replaces the state
std::mt19937& Generator()
{
    static auto generator = std::mt19937(std::random_device()());
    return generator;
}
}

float gamedev::RandomFloat()
{
    static auto distribution = std::uniform_real_distribution<float>(0.0, std::nextafterf(1.f, 11.f));

    return distribution(Generator());
}

float gamedev::RandomFloat(float from, float to) { return from + RandomFloat() * (to - from); }

void gamedev::SeedRandom(std::uint32_t seed) { Generator().seed(seed); }
