#pragma once
#include <glow/fwd.hh>
#include <string>
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"

namespace gamedev
{
/**
 * Unit commands.
 *
 * Unused: orders are set directly in the `Living` and `Attacker` components.
 */
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
