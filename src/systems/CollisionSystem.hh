#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "components/ShapeComp.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

#include <optional>
#include <vector>

namespace gamedev
{
/**
 * 2D collision detection and response on the ground plane (x/z) between oriented boxes and circles.
 *
 * Entities: all instances with a `Collider` (plus a `BoxShape` or `CircleShape`); units inside a tower (`InTower`) take no part.
 * Static and dynamic colliders are kept in separate spatial hash maps (broad phase).
 * `EventType::NewPosition` keeps the hash maps and the cached world-space shapes up to date for moves outside this system.
 *
 * Every update:
 *
 *  1. The world-space shapes and hash map entries of the dynamic colliders are refreshed (their physics moves are not published yet),
 *     and the candidate pairs (dynamic-dynamic and dynamic-static) are collected from the hash maps.
 *  2. Overlaps are resolved by moving positions only, in `kIterations` rounds.
 *     Each round first pushes overlapping dynamic colliders apart (half the overlap each),
 *     then pushes dynamic colliders fully out of static ones, so static colliders always win.
 *  3. A dynamic collider squeezed between static colliders goes back to where it was before this update's physics move,
 *     or to the nearest free spot.
 *  4. `EventType::StaticCollision` is sent once for every dynamic collider that touches or overlaps a static one.
 *
 * Velocities are not changed: a unit walking into an obstacle is pushed back along the contact normal every update,
 * so it slides along the obstacle with the tangential part of its velocity.
 * Moved instances get `Instance::new_position`; `Game` publishes them right after this step (`EngineECS::PublishPositions`).
 */
class CollisionSystem : public System
{
public:
    /**
     * Stores the ECS, sets up the hash maps with one cell per world unit and registers the position listener.
     *
     * @param ecs The ECS; must outlive this object.
     * @param map_bounds World-space area covered by the hash maps (the terrain).
     */
    void Init(EngineECS& ecs, tg::aabb3 map_bounds);

    /// Sorts the entity into the static or dynamic set (by `Collider::dynamic`) and hash map; `InTower` units leave the hash map.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;

    /**
     * Resolves all overlaps (see class description).
     *
     * @param dt Time step in seconds. Unused: the response only moves positions.
     */
    void Update(float dt);

    /// Where a segment enters a static collider (`FindSegmentHit`).
    struct SegmentHit
    {
        InstanceHandle body; ///< The static collider.
        float fraction;      ///< Position along the segment where it enters, in [0, 1].
    };

    /**
     * Finds the first static collider a straight segment enters, each taken as its ground-plane shape extruded from its bottom to its top.
     *
     * Only colliders with `Collider::blocksArrows` count.
     * Archers use it to pick an arrow arc that clears structures (`GamelogicSystem::AimArrow`), and straight shots stick in what they hit.
     *
     * @param from Start of the segment (world space).
     * @param to End of the segment (world space).
     * @param exempt Colliders whose ground-plane shape contains this point (x/z) are ignored, e.g. the tower an archer stands in.
     * @return The collider entered first, or nothing.
     */
    std::optional<SegmentHit> FindSegmentHit(tg::pos3 from, tg::pos3 to, tg::pos3 exempt);

    /**
     * Tests whether a straight segment enters a static collider (see `FindSegmentHit`).
     *
     * @param from Start of the segment (world space).
     * @param to End of the segment (world space).
     * @param exempt Colliders whose ground-plane shape contains this point (x/z) are ignored.
     * @return True if the segment enters a static collider.
     */
    bool IsSegmentBlocked(tg::pos3 from, tg::pos3 to, tg::pos3 exempt) { return FindSegmentHit(from, to, exempt).has_value(); }

private:
    static constexpr int kIterations = 3;            ///< Resolution rounds per update.
    static constexpr float kSkin = 1e-3f;            ///< Gap left between shapes after a push, in world units.
    static constexpr float kContactDistance = 2e-3f; ///< Shapes closer than this touch (`StaticCollision`).
    static constexpr float kPairMargin = 0.25f;      ///< Extra broad-phase range for movement during the rounds, in world units.
    static constexpr float kWedgeTolerance = 1e-2f;  ///< Overlap with a static collider after all rounds that triggers `RevertWedgedColliders`.
    static constexpr float kEscapeStep = 0.05f;      ///< Distance between the rings of candidate positions in `FindFreeSpot`, in world units.
    static constexpr float kEscapeDistance = 1.f;    ///< Largest distance `FindFreeSpot` searches, in world units.
    static constexpr int kEscapeDirections = 16;     ///< Candidate directions per ring in `FindFreeSpot`.

    /// Collision shape in world space on the ground plane; 2D vectors hold world (x, z).
    struct WorldShape
    {
        bool box = false;     ///< Oriented box (`halfAxes`) if true, else a circle (`radius`).
        tg::pos2 center;      ///< Center of the shape.
        tg::vec2 halfAxes[2]; ///< Box only: half extents along the box's two axes.
        float radius = 0.f;   ///< Circle radius, or the box's bounding radius (half diagonal).
        float bottom = 0.f;   ///< World height of the bottom (`BoxShape::minY`/`CircleShape::minY`, scaled and moved).
        float top = 0.f;      ///< World height of the top.
    };

    /// Result of a narrow-phase test between two shapes.
    struct Contact
    {
        tg::vec2 normal = {1, 0}; ///< Unit direction in which the first shape has to move to separate.
        float separation = 0.f;   ///< Distance between the shapes along `normal`; negative if they overlap.
    };

    /// A dynamic collider near a static one.
    struct StaticPair
    {
        InstanceHandle dynamic; ///< Dynamic collider.
        InstanceHandle body;    ///< Static collider.
        bool touching = false;  ///< The shapes touched or overlapped in this update.
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
     * @param margin Extra range on every side, in world units.
     * @return Bounds relative to `translation` (only x/z are set).
     */
    static tg::aabb3 BroadPhaseBounds(WorldShape const& shape, tg::vec3 const& translation, float margin = 0.f);

    /// Cached world-space shape of a live collider, indexed by its `World` slot.
    WorldShape& CachedShape(InstanceHandle handle);

    /**
     * Narrow phase: tests two shapes against each other.
     *
     * @param a Shape to move.
     * @param b Shape it collides with.
     * @return Contact normal (direction for `a`) and separation.
     */
    static Contact ComputeContact(WorldShape const& a, WorldShape const& b);

    /**
     * Circle-vs-circle test.
     *
     * @param centerA Center of the circle to move.
     * @param radiusA Its radius.
     * @param centerB Center of the other circle.
     * @param radiusB Its radius.
     * @return Contact for the first circle; coincident centers separate along +x.
     */
    static Contact CircleCircle(tg::pos2 centerA, float radiusA, tg::pos2 centerB, float radiusB);

    /**
     * Circle-vs-box test with the closest point on the box.
     *
     * @param circle Circle to move.
     * @param box Box it collides with.
     * @return Contact for the circle.
     */
    static Contact CircleBox(WorldShape const& circle, WorldShape const& box);

    /**
     * Box-vs-box test with the separating axis theorem (the four box axes).
     *
     * @param a Box to move.
     * @param b Box it collides with.
     * @return Contact for `a`; the normal is the axis of least penetration.
     */
    static Contact BoxBox(WorldShape const& a, WorldShape const& b);

    /**
     * Clips a segment against a shape extruded from its bottom to its top.
     *
     * @param shape World-space shape.
     * @param from Start of the segment (world space).
     * @param to End of the segment (world space).
     * @return Position along the segment where it enters the shape, in [0, 1], or nothing if it misses.
     */
    static std::optional<float> SegmentEntry(WorldShape const& shape, tg::pos3 from, tg::pos3 to);

    /**
     * Tests whether a point lies inside a shape on the ground plane.
     *
     * @param shape World-space shape.
     * @param point Point (world x, z).
     * @return True if inside or on the border.
     */
    static bool Contains(WorldShape const& shape, tg::pos2 point);

    /// Collects the candidate pairs of this update from the hash maps.
    void CollectPairs();

    /**
     * Frees dynamic colliders that still overlap a static one after all rounds.
     *
     * This happens when a unit is squeezed into a gap between static colliders that is narrower than the unit,
     * or when a structure is built on top of it: pushing it out of one collider pushes it into the other.
     * The unit goes back to `Physics::lastPosition` (x/z) if that position is free, else to the nearest free spot (`FindFreeSpot`).
     */
    void RevertWedgedColliders();

    /**
     * Searches rings of positions around a dynamic collider for the nearest one that overlaps no static collider.
     *
     * @param handle Live dynamic collider.
     * @param shape Its world shape.
     * @return Offset (x, z) to the free position, or nothing if there is none within `kEscapeDistance`.
     */
    std::optional<tg::vec2> FindFreeSpot(InstanceHandle handle, WorldShape const& shape);

    /**
     * Moves a dynamic collider on the ground plane and flags its new position.
     *
     * @param handle Live dynamic collider.
     * @param[in,out] shape Its cached world shape, moved along.
     * @param offset World-space offset (x, z).
     */
    void Move(InstanceHandle handle, WorldShape& shape, tg::vec2 offset);

    /**
     * `EventType::NewPosition` listener: recomputes the subject's world shape and moves it in its hash map.
     *
     * @param e Position event; sender and subject are the moved instance.
     */
    void PositionListener(Event& e);

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).

    std::set<InstanceHandle> mStaticEntities;  ///< Colliders that never move.
    std::set<InstanceHandle> mDynamicEntities; ///< Colliders that move (units).

    std::vector<WorldShape> mShapes;                                      ///< World-space shapes by `World` slot (`MAX_INSTANCES` entries).
    std::vector<std::pair<InstanceHandle, InstanceHandle>> mDynamicPairs; ///< Candidate dynamic pairs of this update (each pair once).
    std::vector<StaticPair> mStaticPairs;                                 ///< Candidate dynamic-static pairs of this update.
    std::vector<InstanceHandle> mCandidates;                              ///< Reused result buffer of hash map queries.
    std::vector<InstanceHandle> mSegmentCandidates;                       ///< Reused result buffer of `FindSegmentHit`.

    Hash2D mStaticColliderHashMap;  ///< Broad phase for static colliders.
    Hash2D mDynamicColliderHashMap; ///< Broad phase for dynamic colliders (without `InTower` units).
};
}
