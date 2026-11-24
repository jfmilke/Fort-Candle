#pragma once
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include "advanced/World.hh"
#include "typed-geometry/tg.hh"
#include "utility/HashData.hh"

namespace std
{
/// Orders grid cells row by row (y first, then x), so `tg::ipos2` can be used as `std::map`/`std::set` key.
template <>
struct less<tg::ipos2>
{
    bool operator()(const tg::ipos2& lhs, const tg::ipos2& rhs) const { return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x)); }
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
 * Used as the ECS-wide spatial hash map (`EngineECS::GetHashMap`, set up by `Terrain::CreateGround`) and as broad phase by the `CollisionSystem`.
 */
class Hash2D
{
public:
    /**
     * Sets up the grid. Does not clear existing entries.
     *
     * @param cellCount Number of cells per axis.
     * @param bounds Covered area; x/y of `bounds` are world x/z.
     */
    void Init(tg::size2 cellCount, tg::aabb2 bounds);

    /**
     * Same as above, with the x/z part of 3D bounds.
     *
     * @param cellCount Number of cells per axis (x, z).
     * @param bounds Covered area in world space; y is ignored.
     */
    void Init(tg::size2 cellCount, tg::aabb3 bounds);

    /**
     * Inserts an instance into all cells its bounding rectangle overlaps.
     *
     * Does nothing if the instance is already stored (use `UpdateInstance` to move it).
     *
     * @param identifier Instance to insert.
     * @param translation Its world position (only x/z are used).
     * @param dimensions Its bounds relative to `translation` (only x/z are used).
     */
    void AddInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);

    /**
     * Moves a known instance, keeping its bounds. Does nothing for unknown instances.
     *
     * @param identifier Instance to move.
     * @param translation Its new world position (only x/z are used).
     */
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation);

    /**
     * Moves a known instance and replaces its bounds. Does nothing for unknown instances.
     *
     * @param identifier Instance to move.
     * @param translation Its new world position (only x/z are used).
     * @param dimensions Its new bounds relative to `translation` (only x/z are used).
     */
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);

    /**
     * Removes an instance from all cells; a later `AddInstance` stores it again.
     *
     * @param identifier Instance to remove; nothing happens for unknown instances.
     */
    void RemoveInstance(const InstanceHandle identifier);

    /**
     * Returns all instances in the cells overlapping a rectangle on the ground plane.
     *
     * The result may contain instances that are not inside the rectangle.
     *
     * @param translation World position the rectangle is relative to.
     * @param min Minimum corner relative to `translation` (only x/z are used).
     * @param max Maximum corner relative to `translation` (only x/z are used).
     * @return Handles of the instances in these cells, each once, sorted by handle value.
     */
    std::vector<InstanceHandle> FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max);

    /**
     * `FindNear` with the rectangle given as bounds.
     *
     * @param translation World position the rectangle is relative to.
     * @param bounds Rectangle relative to `translation` (only x/z are used).
     * @return Handles of the instances in these cells, each once, sorted by handle value.
     */
    std::vector<InstanceHandle> FindNear(const tg::vec3& translation, const tg::aabb3& bounds);

    /**
     * `FindNear` for a rectangle centered on a position.
     *
     * @param translation Center of the rectangle in world space.
     * @param bounds Half extents: `width` along x, `depth` along z.
     * @return Handles of the instances in these cells, each once, sorted by handle value.
     */
    std::vector<InstanceHandle> FindNear(const tg::vec3& translation, const tg::size3& bounds);

    /**
     * Returns all instances in the cell that contains a position.
     *
     * @param position World position (only x/z are used).
     * @return Handles of the instances in these cells, each once, sorted by handle value.
     */
    std::vector<InstanceHandle> FindNear(const tg::pos3& position);

    /// Removes all instances.
    void Clear();

private:
    /**
     * Finds the cell containing a ground-plane position.
     *
     * @param spatial_key Position (world x, z).
     * @return Cell index, clamped to the grid.
     */
    tg::ipos2 PositionToGrid_(const tg::pos2& spatial_key);

    /**
     * Finds the cell containing the x/z part of a position.
     *
     * @param spatial_key World position.
     * @return Cell index, clamped to the grid.
     */
    tg::ipos2 PositionToGrid_(const tg::pos3& spatial_key);

    /**
     * Builds the string key of a cell. Unused: replaced by `GetIndexID_`.
     *
     * @param p Cell index.
     * @return "x.y".
     */
    std::string Key_(const tg::ipos2& p);

    /**
     * Stores a client in all cells its rectangle overlaps and records it in `mClientMap`.
     *
     * @param[in,out] c Client; its `indicies` are set to the covered cells.
     */
    void Insert_(Hash2Client& c);

    /**
     * Removes a client from the cells recorded in its `indicies`.
     *
     * @param c Client to remove.
     */
    void Remove_(Hash2Client& c);

    /**
     * Computes the integer key of a cell (row-major, unique per cell).
     *
     * @param pos Cell index.
     * @return Key into `mBinMap`.
     */
    int GetIndexID_(tg::ipos2 pos);

private:
    tg::size2 mMaxCells;                                        ///< Highest cell index per axis (cell count - 1).
    tg::aabb2 mBounds;                                          ///< Covered area (world x/z).
    std::unordered_map<int, std::vector<Hash2Client>> mBinMap;  ///< Cell key -> clients in that cell.
    std::unordered_map<InstanceHandle, Hash2Client> mClientMap; ///< Stored instance -> client.
};
}
