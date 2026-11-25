#pragma once
#include <typed-geometry/tg-lean.hh>
#include "components/Component.hh"

namespace gamedev
{
/// Simple kinematics, integrated by the `PhysicsSystem` (positions in world units, velocities per second).
struct Physics : Component
{
    using Component::Component; // Use constructor of component

    tg::vec3 velocity = tg::vec3::zero; ///< Velocity in world units per second.

    /**
     * Position before the last physics update, written once per update before the move.
     *
     * `Factory` placements also set it, so a freshly placed instance does not appear to move.
     * Read as the swept segment of an arrow's hit and ground tests,
     * and by the `CollisionSystem` as fallback for units squeezed between structures.
     */
    tg::vec3 lastPosition = tg::vec3::zero;

    /**
     * Horizontal velocity (y = 0) actually observed over the last updates, in world units per second.
     *
     * Unlike `velocity`, it includes collision pushes and sliding.
     * `PhysicsSystem` smooths it over about `PhysicsSystem::kObservedVelocityTime`, which filters out the jitter of crowd pushes.
     * Archers lead their shots with it.
     */
    tg::vec3 observedVelocity = tg::vec3::zero;

    bool forceGround = true; ///< Snap the height to the terrain every update.
    bool gravity = false;    ///< Accelerate downwards (-y) by `PhysicsSystem::kGravity` (projectiles fly ballistic arcs).
};
}
