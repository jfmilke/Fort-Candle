#include "utility/misc.hh"

std::string gamedev::lowerString(std::string string)
{
    std::transform(string.begin(), string.end(), string.begin(), ::tolower);
    return string;
}
