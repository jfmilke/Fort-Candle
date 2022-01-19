#include "utility/HashMap.hh"
#include <string>

void gamedev::Hash2D::Init(tg::size2 cellCount, tg::aabb2 bounds)
{
    mMaxCells = cellCount;
    // Is being used later for denormalization, so correct values for that
    mMaxCells.width -= 1;
    mMaxCells.height -= 1;
    mBounds = bounds;
}

void gamedev::Hash2D::Init(tg::size2 cellCount, tg::aabb3 bounds)
{
    tg::aabb2 bounds2D;
    bounds2D.min = {bounds.min.x, bounds.min.z};
    bounds2D.max = {bounds.max.x, bounds.max.z};

    Init(cellCount, bounds2D);
}

int gamedev::Hash2D::GetIndexID_(tg::ipos2 pos) { return pos.y * mMaxCells.width + pos.x; }

tg::ipos2 gamedev::Hash2D::PositionToGrid_(const tg::pos2& position)
{
    tg::vec2 spatial_coordinate;

    // Normalized world space coordinates within the grid
    spatial_coordinate.x = tg::saturate((position.x - mBounds.min.x) / (mBounds.max.x - mBounds.min.x));
    spatial_coordinate.y = tg::saturate((position.y - mBounds.min.y) / (mBounds.max.y - mBounds.min.y));

    // Output denormalized in cell space
    return tg::ipos2(spatial_coordinate * mMaxCells);
}

tg::ipos2 gamedev::Hash2D::PositionToGrid_(const tg::pos3& position)
{
    tg::vec2 spatial_coordinate;

    spatial_coordinate.x = tg::saturate((position.x - mBounds.min.x) / (mBounds.max.x - mBounds.min.x));
    spatial_coordinate.y = tg::saturate((position.z - mBounds.min.y) / (mBounds.max.y - mBounds.min.y));

    // Output denormalized in cell space
    return tg::ipos2(spatial_coordinate * mMaxCells);
}

void gamedev::Hash2D::Insert_(Hash2Client& c)
{
    auto i1 = PositionToGrid_(c.dimensions.min + c.translation);
    auto i2 = PositionToGrid_(c.dimensions.max + c.translation);

    c.indicies = {i1, i2};

    for (int x = i1.x; x <= i2.x; x++)
    {
        for (int y = i1.y; y <= i2.y; y++)
        {
            tg::ipos2 pos{x, y};
            //auto k = _Key(pos);
            auto k = GetIndexID_(pos);

            if (mBinMap.find(k) == mBinMap.end())
            {
                mBinMap.insert({k, std::vector<Hash2Client>()});
            }

            mBinMap[k].push_back(c);
            mClientMap.insert_or_assign(c.handle, c);
        }
    }
}

void gamedev::Hash2D::Remove_(Hash2Client& c)
{
    auto i1 = c.indicies.first;
    auto i2 = c.indicies.second;

    for (int x = i1.x; x <= i2.x; x++)
    {
        for (int y = i1.y; y <= i2.y; y++)
        {
            tg::ipos2 pos{x, y};
            //auto k = _Key(pos);
            auto k = GetIndexID_(pos);

            auto& container = mBinMap[k];
            for (int i = 0; i < container.size(); i++)
            {
                if (container[i].handle == c.handle)
                {
                    std::swap(container[i], container.back());
                    container.pop_back();
                }
            }
        }
    }
}

std::string gamedev::Hash2D::Key_(const tg::ipos2& p) { return std::to_string(p.x) + "." + std::to_string(p.y); }

void gamedev::Hash2D::AddInstance(const InstanceHandle handle, const tg::vec3& translation, const tg::aabb3& dimensions)
{
    if (mClientMap.find(handle) != mClientMap.end())
        return;

    tg::pos2 min = {dimensions.min.x, dimensions.min.z};
    tg::pos2 max = {dimensions.max.x, dimensions.max.z};

    // Create a new internal client which holds some easy access information
    Hash2Client hc2{handle, {translation.x, translation.z}, {min, max}, {}, 0};

    Insert_(hc2);
}

void gamedev::Hash2D::UpdateInstance(const InstanceHandle handle, const tg::vec3& translation)
{
    if (mClientMap.find(handle) != mClientMap.end())
    {
        auto& c = mClientMap[handle];
        c.translation.x = translation.x;
        c.translation.y = translation.z;

        Remove_(c);
        Insert_(c);
    }
}

void gamedev::Hash2D::UpdateInstance(const InstanceHandle handle, const tg::vec3& translation, const tg::aabb3& dimensions)
{
    if (mClientMap.find(handle) != mClientMap.end())
    {
        auto& c = mClientMap[handle];
        c.translation.x = translation.x;
        c.translation.y = translation.z;
        c.dimensions.min = {dimensions.min.x, dimensions.min.z};
        c.dimensions.max = {dimensions.max.x, dimensions.max.z};

        Remove_(c);
        Insert_(c);
    }
}

void gamedev::Hash2D::RemoveInstance(InstanceHandle handle)
{
    if (mClientMap.find(handle) == mClientMap.end())
    {
        return;
    }

    auto c = mClientMap[handle];
    Remove_(c);
}

std::unordered_set<gamedev::InstanceHandle, gamedev::InstanceHandleHash> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max)
{
    // find area to search for instances
    auto i1 = PositionToGrid_(min + translation);
    auto i2 = PositionToGrid_(max + translation);

    // instance container
    std::unordered_set<InstanceHandle, InstanceHandleHash> handles;

    // ensure uniqueness
    int queryID = mQueryIDs++;

    // search whole area
    for (int x = i1.x; x <= i2.x; x++)
    {
        for (int y = i1.y; y <= i2.y; y++)
        {
            tg::ipos2 pos{x, y};
            //auto k = _Key(pos);
            auto k = GetIndexID_(pos);

            // sanity check + instance retrieval
            if (mBinMap.find(k) != mBinMap.end())
            {
                for (auto& v : mBinMap[k])
                {
                    // uniqueness check: if instance was already added during this query, its queryID will match
                    if (v.queryID != queryID)
                    {
                        v.queryID = queryID;
                        handles.insert(v.handle);
                    }
                }
            }
        }
    }

    return handles;
}

std::unordered_set<gamedev::InstanceHandle, gamedev::InstanceHandleHash> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::aabb3& bounds)
{
    return FindNear(translation, bounds.min, bounds.max);
}

std::unordered_set<gamedev::InstanceHandle, gamedev::InstanceHandleHash> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::size3& bounds)
{
    return FindNear(translation, {-bounds.width, 0, -bounds.depth}, {bounds.width, 0, bounds.depth});
}

std::unordered_set<gamedev::InstanceHandle, gamedev::InstanceHandleHash> gamedev::Hash2D::FindNear(const tg::pos3& position)
{
    return FindNear(tg::vec3(position), tg::pos3::zero, tg::pos3::zero);
}

void gamedev::Hash2D::Clear()
{
    mBinMap.clear();
    mClientMap.clear();
}