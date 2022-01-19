#pragma once
/*
#include "advanced/World.hh"
#include "advanced/transform.hh"
#include "Component.hh"
#include "ecs/Engine.hh"

#include <typed-geometry/types/objects/aabb.hh>
#include <glow/fwd.hh>
#include <typed-geometry/tg-lean.hh>
#include <array>

namespace gamedev
{

// Inspired by https://www.gamedev.net/articles/programming/general-and-gameplay-programming/introduction-to-octrees-r3529/
//             https://thatgamesguy.co.uk/cpp-game-dev-16/

class QuadTree
{
    GLOW_SHARED(struct, Node);

public:
    // Properties
    const int mMaxObjects = 8;
    const int mMinSize = 1;

public:
    QuadTree();
    QuadTree(tg::aabb2 global_region);

private:
    // 0..3: NE, NW, SW,SE
    struct Node
    {
        tg::aabb2 mRegion;                          // defines the quadrant
        SharedNode mParent;
        std::array<SharedNode, 4> mChildren;        // points to sub-quadrants (alias: NE, NW, SW, SE)
        std::vector<InstanceHandle> mObjects; // elements hold by this quadrant (either node is leaf or elements lie on separating axis)

        int level;
        bool hasChildren() { return mChildren[0] != nullptr; }
    };

    std::vector<std::pair<InstanceHandle, tg::aabb2>> _queue;
    SharedNode mRoot;

    bool mTreeReady;
    bool mTreeBuilt;

    public:
    void init(SharedEngineECS& engineECS);

    void add(InstanceHandle handle);
    void remove(InstanceHandle handle);
    std::vector<InstanceHandle> search(Shape& collision_area);
    void clearTree();
    void buildTree();
    void getBounds();

    private:
    void add(InstanceHandle& handle, Shape& shape, SharedNode& node);
    void remove(InstanceHandle& handle, Shape& shape, SharedNode& node);
    void search(Shape& collision_area, SharedNode& node, std::vector<InstanceHandle>& possible_colliders);

    int getQuadrant(Shape& shape, SharedNode& node);
    void getBounds(SharedNode& node);
    void split(SharedNode& node);

    static const int thisNode = -1;
    static const int NE = 0;
    static const int NW = 1;
    static const int SW = 2;
    static const int SE = 3;

    const int splitTrigger = 4; // Maximum objects before split is done
    const tg::vec2 minDimension = tg::vec2::one; // Minimum dimension of each node

private:
    SharedEngineECS ecs;
};
}
*/