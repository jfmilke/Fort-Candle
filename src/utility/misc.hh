#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <cctype>
#include <iostream>
#include <cmath>

namespace gamedev
{
/**
 * Converts a string to lower case (ASCII), e.g. to normalize preset and asset names.
 *
 * @param string String to convert.
 * @return The lower-case copy.
 */
std::string lowerString(std::string string);

/**
 * Solves the quadratic equation a*x^2 + b*x + c = 0.
 *
 * @param a Quadratic coefficient; must not be zero.
 * @param b Linear coefficient.
 * @param c Constant term.
 * @return Both real solutions (s1 >= s2 for a > 0), or (0, 0) if there is none.
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
