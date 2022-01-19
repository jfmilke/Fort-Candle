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
template<>
struct less<tg::ipos2>
{
    bool operator()(const tg::ipos2& lhs, const tg::ipos2& rhs) const {return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x)); }
};
}

namespace gamedev
{
class Hash2D
{
public:
    void Init(tg::size2 cellCount, tg::aabb2 bounds);
    void Init(tg::size2 cellCount, tg::aabb3 bounds);

    void AddInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation);
    void UpdateInstance(const InstanceHandle identifier, const tg::vec3& translation, const tg::aabb3& dimensions);
    void RemoveInstance(const InstanceHandle identifier);
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::pos3& min, const tg::pos3& max);
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::aabb3& bounds);
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::vec3& translation, const tg::size3& bounds);
    std::unordered_set<gamedev::InstanceHandle, InstanceHandleHash> FindNear(const tg::pos3& position);
    void Clear();

private:
    tg::ipos2 PositionToGrid_(const tg::pos2& spatial_key);
    tg::ipos2 PositionToGrid_(const tg::pos3& spatial_key);
    std::string Key_(const tg::ipos2& p);
    void Insert_(Hash2Client& c);
    void Remove_(Hash2Client& c);

    int GetIndexID_(tg::ipos2 pos);


private:
    tg::size2 mMaxCells;
    tg::aabb2 mBounds;
    int mQueryIDs = 1;
    std::unordered_map<int, std::vector<Hash2Client>> mBinMap;                      // Point -> Instances
    std::unordered_map<InstanceHandle, Hash2Client, InstanceHandleHash> mClientMap; // Instance -> Hash2Client
};
}
