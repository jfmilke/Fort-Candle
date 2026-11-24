#pragma once
#include <typed-geometry/tg-lean.hh>
#include "components/Component.hh"

namespace gamedev
{
/// Parabolic arrow trajectory computed by `GamelogicSystem::GetArrowArc`.
struct arcEq
{
    float xMax;        ///< Horizontal distance to the predicted target position; 0 if no shot is possible.
    float yMax;        ///< Apex height above the start height.
    float yBase;       ///< Start height.
    tg::vec3 velocity; ///< Horizontal velocity (y = 0) towards the predicted target position.
};

/// Simple kinematics, integrated by the `PhysicsSystem` (positions in world units, velocities per second).
struct Physics : Component
{
    using Component::Component; // Use constructor of component

    tg::vec3 velocity = tg::vec3::zero;     ///< Velocity in world units per second.
    tg::vec3 lastPosition = tg::vec3::zero; ///< Position before the last physics update; used to undo moves into colliders and to aim arrows ahead.

    bool forceGround = true; ///< Snap the height to the terrain every update.
    bool gravity = false;    ///< Accelerate downwards (-y) by 9.81 per second squared.

    bool arrowArc = false; ///< Follow a parabola in y instead of `velocity.y` (arrows).
    float arc_x = 0;       ///< Horizontal distance travelled along the arc.
    float arc_xMax = 0;    ///< Horizontal length of the arc.
    float arc_yMax = 0;    ///< Apex height of the arc above `arc_yBase`.
    float arc_yBase = 0;   ///< Start height of the arc.
    float arc_ySlope = 0;  ///< Current slope dy/dx of the arc; `AnimationSystem` aligns arrows to it.
};
}
