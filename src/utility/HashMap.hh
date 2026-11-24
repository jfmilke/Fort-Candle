#pragma once
#include <unordered_map>
#include <utility>
#include <vector>
#include "advanced/World.hh"
#include "typed-geometry/tg.hh"

namespace gamedev
{
/**
 * Uniform grid over the ground plane (world x/z) for fast neighbourhood queries.
 *
 *  - Each instance is stored in every cell its bounding rectangle overlaps.
 *  - Positions outside the map bounds are clamped to the border cells.
 *  - The cells are a dense array that holds handles only; a move that stays in the same cells changes nothing.
 *
 * Used as the ECS-wide spatial hash map (`EngineECS::GetHashMap`, set up by `Terrain::CreateGround`) and as broad phase by the `CollisionSystem`.
 */
class Hash2D
{
public:
    /**
     * Sets up the grid and removes all instances.
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
     * Collects all instances in the cells overlapping a rectangle on the ground plane, without allocating once `result` is large enough.
     *
     * The result may contain instances that are not inside the rectangle.
     *
     * @param translation World position the rectangle is relative to.
     * @param bounds Rectangle relative to `translation` (only x/z are used).
     * @param[out] result Cleared, then filled with the handles of the instances in these cells, each once, sorted by handle value.
     */
    void FindNear(const tg::vec3& translation, const tg::aabb3& bounds, std::vector<InstanceHandle>& result) const;

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
    std::vector<InstanceHandle> FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max) const;

    /**
     * `FindNear` with the rectangle given as bounds.
     *
     * @param translation World position the rectangle is relative to.
     * @param bounds Rectangle relative to `translation` (only x/z are used).
     * @return Handles of the instances in these cells, each once, sorted by handle value.
     */
    std::vector<InstanceHandle> FindNear(const tg::vec3& translation, const tg::aabb3& bounds) const;

    /// Removes all instances.
    void Clear();

private:
    /// Inclusive range of cells (minimum cell, maximum cell).
    using CellRange = std::pair<tg::ipos2, tg::ipos2>;

    /// A stored instance.
    struct Entry
    {
        tg::aabb2 dimensions; ///< Bounds relative to the instance position (world x/z).
        CellRange cells;      ///< Cells the instance is stored in.
    };

    /**
     * Finds the cell containing a ground-plane position.
     *
     * @param position Position (world x, z).
     * @return Cell index, clamped to the grid.
     */
    tg::ipos2 PositionToGrid_(const tg::pos2& position) const;

    /**
     * Computes the cells a rectangle overlaps.
     *
     * @param translation World position the rectangle is relative to (only x/z are used).
     * @param min Minimum corner relative to `translation` (world x, z).
     * @param max Maximum corner relative to `translation` (world x, z).
     * @return Covered cells, clamped to the grid.
     */
    CellRange CellsOf_(const tg::vec3& translation, const tg::pos2& min, const tg::pos2& max) const;

    /**
     * Moves an entry to new cells, unless they are the cells it is already in.
     *
     * @param identifier Stored instance.
     * @param[in,out] entry Its entry; `cells` is set to `cells`.
     * @param cells New cells.
     */
    void Move_(const InstanceHandle identifier, Entry& entry, const CellRange& cells);

    /**
     * Stores an instance in a range of cells.
     *
     * @param identifier Instance to store.
     * @param cells Cells to store it in.
     */
    void Insert_(const InstanceHandle identifier, const CellRange& cells);

    /**
     * Removes an instance from a range of cells.
     *
     * @param identifier Instance to remove.
     * @param cells Cells it is stored in.
     */
    void Remove_(const InstanceHandle identifier, const CellRange& cells);

    /// Index of a cell in `mCells` (row-major).
    size_t CellIndex_(int x, int y) const { return size_t(y) * size_t(mMaxCells.width + 1) + size_t(x); }

private:
    tg::isize2 mMaxCells = {-1, -1};                    ///< Highest cell index per axis (cell count - 1).
    tg::aabb2 mBounds;                                  ///< Covered area (world x/z).
    std::vector<std::vector<InstanceHandle>> mCells;    ///< Handles stored per cell, row-major (`CellIndex_`).
    std::unordered_map<InstanceHandle, Entry> mEntries; ///< Stored instances.
};
}
