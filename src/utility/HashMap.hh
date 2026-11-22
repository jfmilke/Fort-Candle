#pragma once
#include "advanced/World.hh"
#include "utility/HashData.hh"
#include "typed-geometry/tg.hh"
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>

namespace std
{
/// Orders grid cells row by row (y first, then x), so `tg::ipos2` can be used as `std::map`/`std::set` key.
template<>
struct less<tg::ipos2>
{
    bool operator()(const tg::ipos2& lhs, const tg::ipos2& rhs) const {return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x)); }
};
}

namespace gamedev
{
/**
 * Uniform grid over the ground plane (world x/z) for fast neighbourhood queries.
 *
 *  - Each instance is stored in every cell its bounding rectangle overlaps.
 *  - Positions outside the map bounds are clamped to the border cells.
 *
 * Used as the ECS-wide spatial hash map (`EngineECS::GetHashMap`, set up by `Terrain::CreateGround`) and as broad
 * phase by the `CollisionSystem`.
 */
class Hash2D
{
public:
    /// Sets up a grid of `cellCount` cells covering `bounds` (x/y of `bounds` = world x/z). Does not clear entries.
    void Init(tg::size2 cellCount, tg::aabb2 bounds);

    /// Same as above, with the x/z part of 3D `bounds`.
    void Init(tg::size2 cellCount, tg::aabb3 bounds);

    /**
     * Inserts `identifier` at `translation` with local bounds `dimensions` (only x/z are used).
     * Does nothing if the handle is already known, even after `RemoveInstance` (see there).
     */
    void AddInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);

    /// Moves a known instance to `translation` (keeping its bounds).
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation);

    /// Moves a known instance to `translation` and replaces its bounds.
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);

    /**
     * Removes the instance from all cells.
     *
     * TODO: Its entry in `mClientMap` is kept, so `AddInstance` ignores a recycled handle; it is only re-inserted by
     * the next `UpdateInstance`.
     */
    void RemoveInstance(const InstanceHandle identifier);

    /**
     * Returns all instances in the cells overlapping the rectangle [min, max] (x/z) around `translation`.
     * The result may contain instances that are not inside the rectangle.
     */
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max);

    /// `FindNear` with `bounds` relative to `translation`.
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::aabb3& bounds);

    /// `FindNear` for the rectangle `translation` ± (width, depth) of `bounds`.
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::size3& bounds);

    /// Returns all instances in the cell that contains `position`.
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::pos3& position);

    /// Removes all instances.
    void Clear();

private:
    /// Cell containing the ground-plane position (world x, z), clamped to the grid.
    tg::ipos2 PositionToGrid_(const tg::pos2& spatial_key);

    /// Cell containing the x/z part of `spatial_key`, clamped to the grid.
    tg::ipos2 PositionToGrid_(const tg::pos3& spatial_key);

    /// String key of a cell. Unused (replaced by `GetIndexID_`).
    std::string Key_(const tg::ipos2& p);

    /// Stores `c` in all cells its rectangle overlaps and records it in `mClientMap`.
    void Insert_(Hash2Client& c);

    /// Removes `c` from the cells recorded in `c.indicies`.
    void Remove_(Hash2Client& c);

    /**
     * Integer key of a cell.
     *
     * TODO: Rows are `mMaxCells.width` (cells - 1) apart, so the last cell of a row shares its key with the first
     * cell of the next row.
     */
    int GetIndexID_(tg::ipos2 pos);


private:
    tg::size2 mMaxCells;                                                            ///< Highest cell index per axis (cell count - 1).
    tg::aabb2 mBounds;                                                              ///< Covered area (world x/z).
    int mQueryIDs = 1;                                                              ///< Counter to tag clients during a query.
    std::unordered_map<int, std::vector<Hash2Client>> mBinMap;                      ///< Cell key -> clients in that cell.
    std::unordered_map<InstanceHandle, Hash2Client, InstanceHandleHash> mClientMap; ///< Instance -> client (kept after removal).
};
}
