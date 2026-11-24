#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "components/ShapeComp.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

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
 *  1. Static pass: destructible static colliders with health <= 0 are marked for destruction.
 *     Every dynamic collider overlapping a static one is moved back and sent `EventType::StaticCollision`;
 *     the overlapping pairs are then pushed apart in a second run.
 *  2. Dynamic pass: dynamic colliders are separated from each other.
 *
 * Overlaps are found with the separating axis theorem (`minimalPush`). Units inside a tower (`InTower`) are ignored.
 */
class CollisionSystem : public System
{
public:
    /**
     * Stores the ECS, sets up 1x1 hash maps and registers the position listener.
     *
     * Unused: `Game` calls the overload with the map bounds.
     *
     * @param ecs The ECS; must outlive this object.
     */
    void Init(EngineECS& ecs);

    /**
     * Stores the ECS, sets up the hash maps with one cell per world unit and registers the position listener.
     *
     * @param ecs The ECS; must outlive this object.
     * @param map_bounds World-space area covered by the hash maps (the terrain).
     */
    void Init(EngineECS& ecs, tg::aabb3 map_bounds);

    /// Sorts the entity into the static or dynamic set (by `Collider::dynamic`) and hash map.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;

    /**
     * Runs the static and dynamic pass (see class description).
     *
     * @param dt Time step in seconds (scales the sliding along obstacles).
     */
    void Update(float dt);

    /// Static pass: removes destroyed static colliders and resolves dynamic-vs-static overlaps.
    void ResolveStaticCollisions();

    /// Dynamic pass: resolves overlaps between dynamic colliders.
    void ResolveDynamicCollisions();

    // // ToDo: Give a position of intersection
    // tg::pos2 intersection(const BoxShape& boxA, const BoxShape& boxB);
    // tg::pos2 intersection(const BoxShape& box, const CircleShape& circle);
    // tg::pos2 intersection(const CircleShape& circle, const BoxShape& box);
    // tg::pos2 intersection(const CircleShape& circleA, const CircleShape& circleB);

private:
    float mDT = 0.0; ///< Time step of the current update in seconds.

    /**
     * Collides two box shapes and, on overlap, moves `instance_dyn` out of `instance_any` (`resolve`).
     *
     * Both shapes are transformed to world space; bounding circles are tested first.
     *
     * @param instance_dyn Dynamic collider to move.
     * @param instance_any Static or dynamic collider it collides with.
     * @param box1 Shape of `instance_dyn`.
     * @param box2 Shape of `instance_any`.
     * @return True on collision.
     */
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box1, const BoxShape* box2);

    /**
     * See above; box (dynamic) vs. circle.
     *
     * @param instance_dyn Dynamic collider to move.
     * @param instance_any Static or dynamic collider it collides with.
     * @param box Shape of `instance_dyn`.
     * @param circle Shape of `instance_any`.
     * @return True on collision.
     */
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box, const CircleShape* circle);

    /**
     * See above; circle (dynamic) vs. box.
     *
     * @param instance_dyn Dynamic collider to move.
     * @param instance_any Static or dynamic collider it collides with.
     * @param circle Shape of `instance_dyn`.
     * @param box Shape of `instance_any`.
     * @return True on collision.
     */
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle, const BoxShape* box);

    /**
     * See above; circle vs. circle.
     *
     * @param instance_dyn Dynamic collider to move.
     * @param instance_any Static or dynamic collider it collides with.
     * @param circle1 Shape of `instance_dyn`.
     * @param circle2 Shape of `instance_any`.
     * @return True on collision.
     */
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle1, const CircleShape* circle2);

    /**
     * Moves an instance out of a collision.
     *
     * Unless `mSecondRun` is set, a moving instance (non-zero velocity) is reset to its `Physics::lastPosition`
     * and slides along the obstacle (friction only, no bounce).
     * Otherwise it is moved by `push`.
     *
     * @param instance1 Dynamic instance to move.
     * @param instance2 Instance it collides with. Unused.
     * @param push World-space push out of `instance2`, from `minimalPush`.
     */
    void resolve(Instance& instance1, Instance& instance2, tg::vec3 push);

    /**
     * Computes the minimal push between two boxes with the separating axis theorem.
     *
     * @param box1 World-space box to push out.
     * @param box2 World-space box it overlaps.
     * @return Push for `box1`, or zero if the boxes do not overlap.
     */
    tg::vec3 minimalPush(const tg::box3& box1, const tg::box3& box2);

    /**
     * Computes the push between two circles.
     *
     * @param circle1 World-space circle to push out.
     * @param circle2 World-space circle it overlaps.
     * @return Push for `circle1`, or zero if the circles do not overlap.
     */
    tg::vec3 minimalPush(const tg::sphere3& circle1, const tg::sphere3& circle2);

    /**
     * Like the box overload, with the circle approximated by a square aligned to the box.
     *
     * @param box World-space box to push out.
     * @param circle World-space circle it overlaps.
     * @return Push for `box`, or zero if the shapes do not overlap.
     */
    tg::vec3 minimalPush(const tg::box3& box, const tg::sphere3& circle);

    /**
     * Negated box-vs-circle push.
     *
     * @param circle_dynamic World-space circle to push out.
     * @param box_static World-space box it overlaps.
     * @return Push for `circle_dynamic`, or zero if the shapes do not overlap.
     */
    tg::vec3 minimalPush(const tg::sphere3& circle_dynamic, const tg::box3& box_static);

    /**
     * Tests one separating axis for `minimalPush`.
     *
     * @param axis Axis to project onto (need not be normalized).
     * @param points1 Corners of the first shape.
     * @param points2 Corners of the second shape.
     * @return Push along `axis` that separates the shapes (slightly more than the overlap), or zero if they are already separated on this axis.
     */
    tg::vec3 SAT_AxisTest(const tg::vec3& axis, const std::array<tg::pos3, 4>& points1, const std::array<tg::pos3, 4>& points2);

    // http://www.metanetsoftware.com/technique/tutorialA.html#section5
    // Collision Response Approach
    // 1. Project our of collision
    // 2. Split velocity vector into two components: parallel & perpendicular to the collision surface
    // 3. Calculate bounce using perpendicular component
    // 4. Calculate friction using parallel component

private:
    /// Collision shape in world space on the ground plane; 2D vectors hold world (x, z).
    struct WorldShape
    {
        bool box = false;     ///< Oriented box (`halfAxes`) if true, else a circle (`radius`).
        tg::pos2 center;      ///< Center of the shape.
        tg::vec2 halfAxes[2]; ///< Box only: half extents along the box's two axes.
        float radius = 0.f;   ///< Circle radius, or the box's bounding radius (half diagonal).
    };

    /**
     * Transforms an instance's `BoxShape` or `CircleShape` to world space (rotation about y, translation and scale).
     *
     * @param handle Live instance; without a shape the result is a point at its position.
     * @return The world-space shape.
     */
    WorldShape ComputeWorldShape(InstanceHandle handle);

    /**
     * Computes the rectangle a shape covers in the spatial hash maps, relative to the instance's position.
     *
     * Unlike `Instance::max_bounds`, it follows the instance's rotation and scale.
     *
     * @param shape World-space shape.
     * @param translation Instance position the result is relative to.
     * @return Bounds relative to `translation` (only x/z are set).
     */
    static tg::aabb3 BroadPhaseBounds(WorldShape const& shape, tg::vec3 const& translation);

    /**
     * `EventType::NewPosition` listener: moves the subject in the static or dynamic hash map and recomputes its bounds.
     *
     * @param e Position event; sender and subject are the moved instance.
     */
    void PositionListener(Event& e);

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).

    std::set<InstanceHandle> mStaticEntities;  ///< Colliders that never move.
    std::set<InstanceHandle> mDynamicEntities; ///< Colliders that move (units).

    bool mSecondRun = false; ///< True during the second run of the static pass (push out instead of reset).


    Hash2D mStaticColliderHashMap;  ///< Broad phase for static colliders.
    Hash2D mDynamicColliderHashMap; ///< Broad phase for dynamic colliders.
};
}
