#include "utility/HashMap.hh"
#include <clean-core/assert.hh>
#include <algorithm>

void gamedev::Hash2D::Init(tg::size2 cellCount, tg::aabb2 bounds)
{
    // Highest index per axis; used to denormalize positions into cells
    mMaxCells = {tg::max(int(cellCount.width), 1) - 1, tg::max(int(cellCount.height), 1) - 1};
    mBounds = bounds;

    mEntries.clear();
    mCells.assign(size_t(mMaxCells.width + 1) * size_t(mMaxCells.height + 1), {});
}

void gamedev::Hash2D::Init(tg::size2 cellCount, tg::aabb3 bounds)
{
    tg::aabb2 bounds2D;
    bounds2D.min = {bounds.min.x, bounds.min.z};
    bounds2D.max = {bounds.max.x, bounds.max.z};

    Init(cellCount, bounds2D);
}

tg::ipos2 gamedev::Hash2D::PositionToGrid_(const tg::pos2& position) const
{
    // Normalized world space coordinates within the grid
    auto const x = tg::saturate((position.x - mBounds.min.x) / (mBounds.max.x - mBounds.min.x));
    auto const y = tg::saturate((position.y - mBounds.min.y) / (mBounds.max.y - mBounds.min.y));

    // Denormalized in cell space
    return {int(x * float(mMaxCells.width)), int(y * float(mMaxCells.height))};
}

gamedev::Hash2D::CellRange gamedev::Hash2D::CellsOf_(const tg::vec3& translation, const tg::pos2& min, const tg::pos2& max) const
{
    CC_ASSERT_MSG(!mCells.empty(), "Hash2D used before Init.");

    tg::vec2 const offset = {translation.x, translation.z};
    return {PositionToGrid_(min + offset), PositionToGrid_(max + offset)};
}

void gamedev::Hash2D::Insert_(const InstanceHandle identifier, const CellRange& cells)
{
    for (int y = cells.first.y; y <= cells.second.y; y++)
        for (int x = cells.first.x; x <= cells.second.x; x++)
            mCells[CellIndex_(x, y)].push_back(identifier);
}

void gamedev::Hash2D::Remove_(const InstanceHandle identifier, const CellRange& cells)
{
    for (int y = cells.first.y; y <= cells.second.y; y++)
    {
        for (int x = cells.first.x; x <= cells.second.x; x++)
        {
            // Each instance is stored at most once per cell
            auto& handles = mCells[CellIndex_(x, y)];
            if (auto it = std::ranges::find(handles, identifier); it != handles.end())
            {
                *it = handles.back();
                handles.pop_back();
            }
        }
    }
}

void gamedev::Hash2D::Move_(const InstanceHandle identifier, Entry& entry, const CellRange& cells)
{
    if (cells == entry.cells)
        return;

    Remove_(identifier, entry.cells);
    Insert_(identifier, cells);
    entry.cells = cells;
}

void gamedev::Hash2D::AddInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions)
{
    if (mEntries.contains(identifier))
        return;

    Entry entry;
    entry.dimensions = {{dimensions.min.x, dimensions.min.z}, {dimensions.max.x, dimensions.max.z}};
    entry.cells = CellsOf_(translation, entry.dimensions.min, entry.dimensions.max);

    Insert_(identifier, entry.cells);
    mEntries.emplace(identifier, entry);
}

void gamedev::Hash2D::UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions)
{
    if (auto it = mEntries.find(identifier); it != mEntries.end())
    {
        auto& entry = it->second;
        entry.dimensions = {{dimensions.min.x, dimensions.min.z}, {dimensions.max.x, dimensions.max.z}};
        Move_(identifier, entry, CellsOf_(translation, entry.dimensions.min, entry.dimensions.max));
    }
}

void gamedev::Hash2D::RemoveInstance(const InstanceHandle identifier)
{
    auto it = mEntries.find(identifier);
    if (it == mEntries.end())
        return;

    Remove_(identifier, it->second.cells);
    mEntries.erase(it);
}

void gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::aabb3& bounds, std::vector<InstanceHandle>& result) const
{
    result.clear();

    auto const cells = CellsOf_(translation, {bounds.min.x, bounds.min.z}, {bounds.max.x, bounds.max.z});
    for (int y = cells.first.y; y <= cells.second.y; y++)
    {
        for (int x = cells.first.x; x <= cells.second.x; x++)
        {
            auto const& handles = mCells[CellIndex_(x, y)];
            result.insert(result.end(), handles.begin(), handles.end());
        }
    }

    // Instances overlapping several cells were collected once per cell
    std::ranges::sort(result);
    auto const duplicates = std::ranges::unique(result);
    result.erase(duplicates.begin(), duplicates.end());
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max) const
{
    tg::aabb3 bounds;
    bounds.min = min;
    bounds.max = max;
    return FindNear(translation, bounds);
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::aabb3& bounds) const
{
    std::vector<InstanceHandle> result;
    FindNear(translation, bounds, result);
    return result;
}

void gamedev::Hash2D::Clear()
{
    mEntries.clear();
    for (auto& handles : mCells)
        handles.clear();
}
