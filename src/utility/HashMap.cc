#include "utility/HashMap.hh"
#include <algorithm>
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

// mMaxCells holds the highest index, so a row has mMaxCells.width + 1 cells
int gamedev::Hash2D::GetIndexID_(tg::ipos2 pos) { return pos.y * (mMaxCells.width + 1) + pos.x; }

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
            auto k = GetIndexID_(pos);

            mBinMap[k].push_back(c);
        }
    }

    mClientMap.insert_or_assign(c.handle, c);
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
            auto k = GetIndexID_(pos);

            auto bin = mBinMap.find(k);
            if (bin == mBinMap.end())
                continue;

            // Each client is stored at most once per cell
            auto& clients = bin->second;
            auto it = std::ranges::find(clients, c.handle, &Hash2Client::handle);
            if (it != clients.end())
            {
                *it = clients.back();
                clients.pop_back();
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
    Hash2Client hc2{handle, {translation.x, translation.z}, {min, max}, {}};

    Insert_(hc2);
}

void gamedev::Hash2D::UpdateInstance(const InstanceHandle handle, const tg::vec3& translation)
{
    if (auto client = mClientMap.find(handle); client != mClientMap.end())
    {
        auto& c = client->second;
        c.translation.x = translation.x;
        c.translation.y = translation.z;

        Remove_(c);
        Insert_(c);
    }
}

void gamedev::Hash2D::UpdateInstance(const InstanceHandle handle, const tg::vec3& translation, const tg::aabb3& dimensions)
{
    if (auto client = mClientMap.find(handle); client != mClientMap.end())
    {
        auto& c = client->second;
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
    auto client = mClientMap.find(handle);
    if (client == mClientMap.end())
        return;

    Remove_(client->second);
    mClientMap.erase(client);
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max)
{
    // find area to search for instances
    auto i1 = PositionToGrid_(min + translation);
    auto i2 = PositionToGrid_(max + translation);

    std::vector<InstanceHandle> handles;

    // search whole area
    for (int x = i1.x; x <= i2.x; x++)
    {
        for (int y = i1.y; y <= i2.y; y++)
        {
            tg::ipos2 pos{x, y};

            if (auto bin = mBinMap.find(GetIndexID_(pos)); bin != mBinMap.end())
                for (auto const& client : bin->second)
                    handles.push_back(client.handle);
        }
    }

    // instances overlapping several cells were collected once per cell
    std::ranges::sort(handles);
    auto const duplicates = std::ranges::unique(handles);
    handles.erase(duplicates.begin(), duplicates.end());

    return handles;
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::aabb3& bounds)
{
    return FindNear(translation, bounds.min, bounds.max);
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::vec3& translation, const tg::size3& bounds)
{
    return FindNear(translation, {-bounds.width, 0, -bounds.depth}, {bounds.width, 0, bounds.depth});
}

std::vector<gamedev::InstanceHandle> gamedev::Hash2D::FindNear(const tg::pos3& position)
{
    return FindNear(tg::vec3(position), tg::pos3::zero, tg::pos3::zero);
}

void gamedev::Hash2D::Clear()
{
    mBinMap.clear();
    mClientMap.clear();
}