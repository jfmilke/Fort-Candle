#include "utility/Random.hh"
#include <random>

float gamedev::RandomFloat()
{
    static auto seed_generator = std::random_device();
    static auto number_generator = std::mt19937(seed_generator());
    static auto distribution = std::uniform_real_distribution<float>(0.0, std::nextafterf(1.f, 11.f));

    return distribution(number_generator);
}

float gamedev::RandomFloat(float from, float to) { return from + RandomFloat() * (to - from); }

