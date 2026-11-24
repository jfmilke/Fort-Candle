#pragma once
#include <set>
#include "advanced/World.hh"
#include "typed-geometry/tg-lean.hh"

/// Range of grid cells (min cell, max cell; both inclusive) covered by a 2D client.
typedef std::pair<tg::ipos2, tg::ipos2> Hash2Indices;

/// Range of grid cells covered by a 3D client. Unused.
typedef std::pair<tg::ipos3, tg::ipos3> Hash3Indices;

namespace gamedev
{
/**
 * Entry of a 2D spatial hash map: an instance with its ground-plane position and extent.
 *
 * Ordered and compared by handle only.
 */
struct Hash2Client
{
    InstanceHandle handle = {std::uint32_t(-1)}; ///< Instance this entry stands for.
    tg::vec2 translation;                        ///< Position on the ground plane (world x, z).
    tg::aabb2 dimensions;                        ///< Extent relative to `translation` (local x/z bounds).
    Hash2Indices indicies;                       ///< Cells covered by the rectangle.

    const bool operator<(const Hash2Client& rhs) const { return handle._value < rhs.handle._value; }
    const bool operator==(const Hash2Client& rhs) const { return handle._value == rhs.handle._value; }
};

/**
 * Entry of a 3D spatial hash map.
 *
 * Unused: there is no 3D hash map.
 */
struct Hash3Client
{
    InstanceHandle handle = {std::uint32_t(-1)}; ///< Instance this entry stands for.
    tg::vec3 translation;                        ///< Position.
    tg::aabb3 dimensions;                        ///< Extent relative to `translation`.
    Hash3Indices indicies;                       ///< Cells covered by the rectangle.
    int queryID = -1;                            ///< ID of the last query that returned this entry (deduplication).

    const bool operator<(const Hash3Client& rhs) const { return handle._value < rhs.handle._value; }
    const bool operator==(const Hash3Client& rhs) const { return handle._value == rhs.handle._value; }
};
}
