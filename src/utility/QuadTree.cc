/*

#include "QuadTree.hh"
#include <typed-geometry/types/objects/line.hh>


gamedev::QuadTree::QuadTree()
{
    mRoot = std::make_shared<QuadTree::Node>();
    mRoot->mParent = nullptr;
    mRoot->mRegion = tg::aabb2(tg::pos2::zero, tg::pos2::zero);
}

gamedev::QuadTree::QuadTree(tg::aabb2 global_region)
{
    mRoot = std::make_shared<QuadTree::Node>();
    mRoot->mParent = nullptr;
    mRoot->mRegion = global_region;
}

void gamedev::QuadTree::init(SharedEngineECS& engineECS) { ecs = engineECS; }

void gamedev::QuadTree::add(InstanceHandle& handle, Shape& shape, SharedNode& node)
{
    // Go as deep down as possible
    if (node->hasChildren())
    {
        int quadrantID = getQuadrant(shape, node);
        if (quadrantID != thisNode)
            return add(handle, shape, node->mChildren[quadrantID]);
    }

    // Finally insert object
    node->mObjects.emplace_back(handle);

    // Check split trigger
    if (node->mObjects.size() > splitTrigger)
    {
        tg::vec2 quadSize = node->mRegion.max - node->mRegion.min;

        // If split possible: split!
        if ((quadSize.x > minDimension.x) && (quadSize.y > minDimension.y))
            split(node);
    }
}

void gamedev::QuadTree::add(InstanceHandle handle)
{
    // Check if the tree is brand new
    tg::vec2 treeSize = mRoot->mRegion.max - mRoot->mRegion.min;
    if (treeSize == tg::vec2::zero)
        mRoot->mRegion = tg::aabb2(-3000, 3000);

    auto& shape = ecs->GetComponent<Shape>(handle);

    // Now go out into the tree and fulfill your oath!
    return add(handle, shape, mRoot);
}

int gamedev::QuadTree::getQuadrant(Shape& shape, SharedNode& node)
{ 
  auto quadrantSize = (node->mRegion.max - node->mRegion.min) / 2.0;

  auto verticalAxis = tg::line(tg::pos2(node->mRegion.min + quadrantSize), tg::dir2::pos_x);
  auto horizontalAxis = tg::line(tg::pos2(node->mRegion.min + quadrantSize), tg::dir2::pos_y);

  // Check split axes
  if (tg::intersects(shape.aabb, verticalAxis))
      return thisNode;

  if (tg::intersects(shape.aabb, horizontalAxis))
      return thisNode;

  // Check quadrants
  for (int i = 0; i < 4; i++)
      if (tg::intersects(shape.aabb, node->mChildren[i]->mRegion))
          return i;

  // Object is out of tree space, add to parent (which should be root)
  return thisNode;
}

void gamedev::QuadTree::split(SharedNode& node)
{
    // Split
    for (int i = 0; i < 4; i++)
    {
        node->mChildren[i] = std::make_shared<QuadTree::Node>();
        node->mChildren[i]->level = ++node->level;
        node->mChildren[i]->mParent = node;
    }

    // Distribute (bottom to front, easier to clear vector)
    for (int i = node->mObjects.size() - 1; i >= 0; i--)
    {
        auto& handle = node->mObjects[i];
        auto& shape = ecs->GetComponent<Shape>(handle);
        int quadrantID = getQuadrant(shape, node);
        if (quadrantID != thisNode)
        {
            add(handle, shape, node->mChildren[quadrantID]);
            node->mObjects.pop_back();
        }
    }

    // If here, keep
}

void gamedev::QuadTree::remove(InstanceHandle handle)
{
    auto& shape = ecs->GetComponent<Shape>(handle);
    remove(handle, shape, mRoot);
}

void gamedev::QuadTree::remove(InstanceHandle& handle, Shape& shape, SharedNode& node)
{
    int quadrantID = getQuadrant(shape, node);

    // If the object is assumed in this node, search here
    if ((quadrantID == thisNode) || (node->mChildren[quadrantID] == nullptr))
    {
        for (int i = 0; i < node->mChildren.size(); i++)
        {
            // Object found! Swap & pop (better for vector)
            if (node->mObjects[i] == handle)
            {
                std::swap(node->mObjects[i], node->mObjects.back());
                node->mObjects.pop_back();
                break;
            }
        }
    }
    // If not, continue search
    else
    {
        return remove(handle, shape, node->mChildren[quadrantID]);
    }
}

std::vector<gamedev::InstanceHandle> gamedev::QuadTree::search(Shape& collision_area)
{
    std::vector<InstanceHandle> possible_colliders;
    search(collision_area, mRoot, possible_colliders);
    return possible_colliders;

}

void gamedev::QuadTree::search(Shape& collision_area, SharedNode& node, std::vector<InstanceHandle>& possible_colliders)
{
    // Gather all objects which do not fit into a single quadrant, as they maybe lie on a splitting axis
    possible_colliders.insert(possible_colliders.end(), node->mObjects.begin(), node->mObjects.end());

    // If this is no leaf, continue search
    if (node->mChildren[0] != nullptr)
    {
        int quadrantID = getQuadrant(collision_area, node);

        // If thisNode was returned, then the collision_area does not fit completely into a quadrant. Therefore: search all!
        if (quadrantID == thisNode)
        {
            for (int i = 0; i < 4; i++)
            {
              
            }
        }
    }


}
*/