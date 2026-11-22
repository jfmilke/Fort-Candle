#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

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
 *  - `EventType::ArrowHit` is sent when they reach their target.
 */
class AnimationSystem : public System
{
public:
    /// Stores the ECS.
    void Init(std::shared_ptr<EngineECS>& ecs);

    /// Sorts the entity into the arrow set or the animated set.
    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /**
     * Advances arrows and animations by `dt` seconds. Expired arrows are marked for destruction.
     * Returns the elapsed time in microseconds.
     */
    int Update(float dt);

    /// Applies the pending rotations of `subject` (quaternion target and eased x/y/z angles) to `instance`.
    void Rotate(Instance& instance, Animated* subject, float dt);

    /// Moves the uniform scale of `instance` towards `subject->scale` by at most `dt`.
    void Scale(Instance& instance, Animated* subject, float dt);

    /// Empty.
    void Wiggle(Living* subject, float dt);

    /// Pitches an arrow so it points along the slope of its arc (`Physics::arc_ySlope`).
    void AlignTrajectory(Instance& instance, Physics* physics);

private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.

    std::set<InstanceHandle> mArrows; ///< Entities with an `Arrow` component.
    std::set<InstanceHandle> mLiving; ///< Unused.
};
}
