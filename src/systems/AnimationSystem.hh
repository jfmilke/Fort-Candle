#pragma once

#include <glow/fwd.hh>
#include "Terrain.hh"
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

#include <deque>
#include <memory>

namespace gamedev
{
class CollisionSystem;

/**
 * Plays the non-skeletal `Animated` animations and handles arrows.
 *
 * Entities: all instances with an `Animated` component.
 *
 * Instances that also have an `Arrow` are kept separately and are not animated otherwise.
 * The `PhysicsSystem` moves flying arrows; this system, after the animations:
 *  - points them along their velocity,
 *  - sends `EventType::ArrowHit` when the path of the tip in this update (from `Physics::lastPosition`)
 *    passes within `Arrow::hitRadius` of the target's aim point (`Arrow::aimHeight` above its position);
 *    the arrow then sticks in the target, follows it and is removed with it,
 *  - lets straight shots (`Arrow::hitsBlockers`) stick in the first structure the tip enters (`CollisionSystem::FindSegmentHit`),
 *    until the structure is destroyed,
 *  - lets them stick in the terrain where the tip goes below it, for good.
 *    Of the arrows in the ground or in structures, only the newest `kMaxStuckArrows` stay,
 *  - removes arrows whose flight lasts longer than `Arrow::lifetime`.
 *
 * A stuck arrow loses its `Physics` and `ParticleEmitter` and leaves the ECS spatial hash map.
 */
class AnimationSystem : public System
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    /// Sets the terrain that arrows land on.
    void SetTerrain(std::shared_ptr<Terrain> terrain);

    /// Sets the collision system, which finds the structures straight shots stick in.
    void SetCollision(std::shared_ptr<CollisionSystem> collision);

    /// Sorts the entity into the arrow set or the animated set.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;

    /**
     * Advances the animations, then the arrows (see class description).
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

    /**
     * Applies the pending rotations (quaternion target and eased x/y/z angles).
     *
     * @param instance Instance to rotate.
     * @param subject Its `Animated` component; the remaining angles are reduced.
     * @param dt Time step in seconds.
     */
    void Rotate(Instance& instance, Animated* subject, float dt);

    /**
     * Moves the scale towards `Animated::scale` by at most 1 per second.
     *
     * The same amount is added to all three axes; the z scale decides when the target is reached.
     *
     * @param instance Instance to scale.
     * @param subject Its `Animated` component; `scaleAnim` is reset at the target.
     * @param dt Time step in seconds.
     */
    void Scale(Instance& instance, Animated* subject, float dt);

    /**
     * Does nothing. Unused.
     *
     * @param subject Unused.
     * @param dt Unused.
     */
    void Wiggle(Living* subject, float dt);

    /// Arrows stuck in the ground or in structures that are kept; older ones are removed (the instance pool holds only `MAX_INSTANCES`).
    static constexpr size_t kMaxStuckArrows = 200;

private:
    /**
     * Aligns a flying arrow to its velocity and tests it for a hit or a landing in this update.
     *
     * @param handle Arrow instance.
     * @param[in,out] arrow Its `Arrow` component; the lifetime runs down, the state changes on a hit or landing.
     * @param dt Time step in seconds.
     */
    void UpdateFlyingArrow(InstanceHandle handle, Arrow& arrow, float dt);

    /**
     * Makes an arrow stick in a unit or structure: stores its transform relative to it.
     *
     * @param handle Arrow instance, already placed where it sticks.
     * @param[in,out] arrow Its `Arrow` component; becomes `ArrowState::InObject`.
     * @param object Live unit or structure it hit.
     */
    void StickIn(InstanceHandle handle, Arrow& arrow, InstanceHandle object);

    /**
     * Moves an arrow stuck in a unit or structure along with it.
     *
     * Removes the arrow once the object is gone, marked for destruction, or a `Prototype` (a destroyed wall).
     *
     * @param handle Arrow instance.
     * @param arrow Its `Arrow` component (`ArrowState::InObject`).
     */
    void FollowObject(InstanceHandle handle, Arrow& arrow);

    /**
     * Ends the flight of an arrow: removes its `Physics` and `ParticleEmitter` and takes it out of the ECS spatial hash map.
     *
     * @param handle Arrow instance.
     */
    void StopArrow(InstanceHandle handle);

private:
    EngineECS* mECS = nullptr;                   ///< ECS (not owned).
    std::shared_ptr<Terrain> mTerrain;           ///< Terrain for arrow landings; without it, arrows fly until their lifetime ends.
    std::shared_ptr<CollisionSystem> mCollision; ///< Structures that straight shots stick in; without it, they fly through.

    std::set<InstanceHandle> mArrows;        ///< Entities with an `Arrow` component.
    std::deque<InstanceHandle> mStuckArrows; ///< Arrows stuck in the ground or in structures, oldest first.
    std::set<InstanceHandle> mLiving;        ///< Unused.
};
}
