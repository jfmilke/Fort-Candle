#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

namespace gamedev
{
/**
 * Plays the non-skeletal `Animated` animations and drives flying arrows.
 *
 * Entities: all instances with an `Animated` component.
 *
 * Instances that also have an `Arrow` are kept separately and are not animated otherwise:
 *  - their lifetime runs down,
 *  - they are aligned to their trajectory,
 *  - they are removed once they are 1 unit past the end of their arc (a miss),
 *  - `EventType::ArrowHit` is sent when the segment they travelled in this update (from `Physics::lastPosition`)
 *    passes within `Arrow::hitRadius` of the target's aim point (`Arrow::aimHeight` above its position).
 */
class AnimationSystem : public System
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    /// Sorts the entity into the arrow set or the animated set.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;

    /**
     * Advances arrows and animations. Expired arrows and arrows past their arc are marked for destruction.
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

    /**
     * Pitches an arrow so it points along the slope of its arc.
     *
     * @param instance Arrow instance to rotate about its local x axis.
     * @param physics Its `Physics` component (`arc_ySlope`).
     */
    void AlignTrajectory(Instance& instance, Physics* physics);

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).

    std::set<InstanceHandle> mArrows; ///< Entities with an `Arrow` component.
    std::set<InstanceHandle> mLiving; ///< Unused.
};
}
