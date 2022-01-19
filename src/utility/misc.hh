#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <cctype>
#include <iostream>
#include <cmath>

namespace gamedev
{
std::string lowerString(std::string string);

// For formulas: ax^2 + bx + c
// Returns both solutions as (s1, s2) tuple if real, else it returns (0,0)
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
