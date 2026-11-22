#pragma once
#include <glow/fwd.hh>
#include "types/Properties.hh"

namespace gamedev
{
/// Kinds of buildings. Unused: the `StructureFactory` identifies presets by name.
enum class StructType
{
    Undefined = 0,
    Bonfire,
    WallA,
    WallB,
    Tower,
    TentA,
    TentB
};
}
