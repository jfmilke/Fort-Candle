#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <cctype>
#include <iostream>
#include <cmath>

namespace gamedev
{
/// Returns `string` converted to lower case (ASCII), e.g. to normalize preset and asset names.
std::string lowerString(std::string string);

/**
 * Solves a*x^2 + b*x + c = 0. Returns both real solutions (s1 >= s2 for a > 0), or (0, 0) if there is none.
 * `a` must not be zero.
 */
template<typename T>
std::pair<float, float> solveQuadratic(T a, T b, T c)
{
    auto inner_sqrt = b * b - 4.0f * a * c;
    if (inner_sqrt >= (T)0.0f)
    {
        auto s1 = ((-1.0f * b) + std::sqrt(inner_sqrt)) / (2 * a);
        auto s2 = ((-1.0f * b) - std::sqrt(inner_sqrt)) / (2 * a);
        return {s1, s2};
    }
    else
    {
        return {0.0f, 0.0f};
    }
}
}
