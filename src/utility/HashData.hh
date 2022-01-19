#pragma once
#include "advanced/World.hh"
#include "typed-geometry/tg-lean.hh"
#include <set>

typedef std::pair<tg::ipos2, tg::ipos2> Hash2Indices;
typedef std::pair<tg::ipos3, tg::ipos3> Hash3Indices;

namespace gamedev
{
// For 2D Spatial Hash Maps
struct Hash2Client
{
    InstanceHandle handle = {std::uint32_t(-1)};
    tg::vec2 translation;
    tg::aabb2 dimensions;
    Hash2Indices indicies;  // form aabb inside the spatial grid
    int queryID = -1;       // to deduplicate on retrieval

    const bool operator<(const Hash2Client& rhs) const { return handle._value < rhs.handle._value; }
    const bool operator==(const Hash2Client& rhs) const { return handle._value == rhs.handle._value; }
};

// For 3D Spatial Hash Maps (not implement yet - will they ever?)
struct Hash3Client
{
    InstanceHandle handle = {std::uint32_t(-1)};
    tg::vec3 translation;
    tg::aabb3 dimensions;
    Hash3Indices indicies;  // form aabb inside the spatial grid
    int queryID = -1;       // to deduplicate on retrieval

    const bool operator<(const Hash3Client& rhs) const { return handle._value < rhs.handle._value; }
    const bool operator==(const Hash3Client& rhs) const { return handle._value == rhs.handle._value; }
};
}