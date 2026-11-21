#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "components/ShapeComp.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
/**
 * 2D collision detection and response on the ground plane (x/z) between box (OBB) and circle shapes.
 *
 * Entities: all instances with a `Collider` (plus a `BoxShape` or `CircleShape`).
 * Static and dynamic colliders are kept in separate sets and spatial hash maps (broad phase);
 * `EventType::NewPosition` keeps the hash maps up to date.
 *
 * Every update:
 *
 *  1. Static pass: destructible static colliders with health <= 0 are marked for destruction. Every dynamic collider
 *     overlapping a static one is moved back and sent `EventType::StaticCollision`; the overlapping pairs are then
 *     pushed apart in a second run.
 *  2. Dynamic pass: dynamic colliders are separated from each other.
 *
 * Overlaps are found with the separating axis theorem (`minimalPush`). Units inside a tower (`InTower`) are ignored.
 */
class CollisionSystem : public System
{
public:
    /// Stores the ECS, sets up 1x1 hash maps and registers the position listener. Unused; see the overload.
    void Init(std::shared_ptr<EngineECS>& ecs);

    /**
     * Stores the ECS, sets up the hash maps with one cell per world unit over `map_bounds` and registers the position
     * listener.
     */
    void Init(std::shared_ptr<EngineECS>& ecs, tg::aabb3 map_bounds);

    /// Sorts the entity into the static or dynamic set (by `Collider::dynamic`) and hash map.
    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /**
     * Runs the static and dynamic pass (see class description) with `dt` seconds. Returns the elapsed time in
     * microseconds.
     */
    int Update(float dt);

    /// Static pass: removes destroyed static colliders and resolves dynamic-vs-static overlaps.
    void ResolveStaticCollisions();

    /// Dynamic pass: resolves overlaps between dynamic colliders.
    void ResolveDynamicCollisions();

    // TODO: The intersects() overloads are declared but not implemented (and unused). Remove them.
    bool intersects(const BoxShape* boxA, const BoxShape* boxB);
    bool intersects(const BoxShape* box, const CircleShape* circle);
    bool intersects(const CircleShape* circle, const BoxShape* box) { return intersects(box, circle); }
    bool intersects(const CircleShape* circleA, const CircleShape* circleB);

    bool intersects(const tg::box3& boxA, const tg::box3& boxB);
    bool intersects(const tg::box3& box, const tg::sphere3& circle);
    bool intersects(const tg::sphere3& circle, const tg::box3& box) { return intersects(box, circle); }
    bool intersects(const tg::sphere3& circleA, const tg::sphere3& circleB);

    // // ToDo: Give a position of intersection
    // tg::pos2 intersection(const BoxShape& boxA, const BoxShape& boxB);
    // tg::pos2 intersection(const BoxShape& box, const CircleShape& circle);
    // tg::pos2 intersection(const CircleShape& circle, const BoxShape& box);
    // tg::pos2 intersection(const CircleShape& circleA, const CircleShape& circleB);

private:
    float mDT = 0.0; ///< Time step of the current update in seconds.

    /**
     * Transforms both shapes to world space, tests for overlap (bounding circles first) and, on overlap, moves
     * `instance_dyn` out of `instance_any` (`resolve`). Returns true on collision.
     */
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box1, const BoxShape* box2);

    /// See above; box (dynamic) vs. circle.
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box, const CircleShape* circle);

    /// See above; circle (dynamic) vs. box.
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle, const BoxShape* box);

    /// See above; circle vs. circle.
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle1, const CircleShape* circle2);

    /**
     * Moves `instance1` out of a collision with `instance2`. In the first run, a moving instance is reset to its
     * `Physics::lastPosition` and slides along the obstacle; otherwise it is moved by `push`.
     */
    void resolve(Instance& instance1, Instance& instance2, tg::vec3 push);

    /// Minimal vector (separating axis theorem) that pushes `box1` out of `box2`, or zero if they do not overlap.
    tg::vec3 minimalPush(const tg::box3& box1, const tg::box3& box2);

    /// Vector that pushes `circle1` out of `circle2`, or zero if they do not overlap.
    tg::vec3 minimalPush(const tg::sphere3& circle1, const tg::sphere3& circle2);

    /// Like the box overload, with the circle approximated by a square aligned to the box.
    tg::vec3 minimalPush(const tg::box3& box, const tg::sphere3& circle);

    /// Vector that pushes the circle out of the box (negated box-vs-circle push).
    tg::vec3 minimalPush(const tg::sphere3& circle_dynamic, const tg::box3& box_static);

    /**
     * Projects both point sets onto `axis`; returns the push along `axis` that separates them (slightly more than the
     * overlap), or zero if they are already separated on this axis. Part of `minimalPush`.
     */
    tg::vec3 SAT_AxisTest(const tg::vec3& axis, const std::array<tg::pos3, 4>& points1, const std::array<tg::pos3, 4>& points2);

    // http://www.metanetsoftware.com/technique/tutorialA.html#section5
    // Collision Response Approach
    // 1. Project our of collision
    // 2. Split velocity vector into two components: parallel & perpendicular to the collision surface
    // 3. Calculate bounce using perpendicular component
    // 4. Calculate friction using parallel component

private:
    /// `EventType::NewPosition` listener: moves the sender in the static or dynamic hash map.
    void PositionListener(Event& e);

private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.

    std::set<InstanceHandle> mStaticEntities;  ///< Colliders that never move.
    std::set<InstanceHandle> mDynamicEntities; ///< Colliders that move (units).

    bool mSecondRun = false; ///< True during the second run of the static pass (push out instead of reset).


    Hash2D mStaticColliderHashMap;  ///< Broad phase for static colliders.
    Hash2D mDynamicColliderHashMap; ///< Broad phase for dynamic colliders.
};
}
