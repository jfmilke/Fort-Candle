#pragma once
#include <glow/fwd.hh>
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"
#include <string>

namespace gamedev
{
enum UnitAction
{
    None = -1,
    Stay = 0,
    Move,
    Aim,
    Attack,
    SpecialPrimary,     // i.e. climb wall, produce gold
    SpecialSecondary,   // i.e. repair
    SpecialTertiary,    // placeholder
};

typedef std::uint8_t UnitActionType;
}
