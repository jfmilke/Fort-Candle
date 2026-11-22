#pragma once
#include <glow/fwd.hh>
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"
#include <string>

namespace gamedev
{
/// Unit commands. Unused.
enum UnitAction
{
    None = -1,
    Stay = 0,
    Move,
    Aim,
    Attack,
    SpecialPrimary,   ///< e.g. climb wall, produce gold
    SpecialSecondary, ///< e.g. repair
    SpecialTertiary,  ///< placeholder
};

/// Storage type of a `UnitAction`. Unused.
typedef std::uint8_t UnitActionType;
}
