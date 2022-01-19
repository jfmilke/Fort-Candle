#pragma once
#include <glow/fwd.hh>
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"
#include <string>

namespace gamedev
{
// Enemies < 0
// Friends > 0
enum class NatureType
{
    Undefined = 0,
    Pine_0,
    Pine_1,
    Pine_2,
    Pine_Stump_0,
    Pine_Stump_1,
    Pine_Dead_0,
    Pine_Dead_1,
    Grass_0,
    Grass_1,
    Grass_2,
    Grass_3,
    Grass_4,
    Grass_5
};
}
