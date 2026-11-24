#pragma once
#include <typed-geometry/tg-lean.hh>
#include "components/Component.hh"

namespace gamedev
{
/**
 * Arrow trajectory computed by `GamelogicSystem::GetArrowArc`.
 *
 * The height over the horizontal distance x is `y(u) = yStart + (yEnd - yStart)·u + 4·height·u·(1-u)` with `u = x / xMax`:
 * a straight line from the start height to the end height plus a parabolic bulge that is `height` at the middle.
 */
struct arcEq
{
    float xMax = 0;                     ///< Horizontal distance to the predicted target position; 0 if no shot is possible.
    float height = 0;                   ///< Height of the arc above the straight line from start to end, at its middle.
    float yStart = 0;                   ///< Start height (the arrow's spawn height).
    float yEnd = 0;                     ///< End height (the aim point above the target, see `Arrow::aimHeight`).
    tg::vec3 velocity = tg::vec3::zero; ///< Horizontal velocity (y = 0) towards the predicted target position.
};

/// Simple kinematics, integrated by the `PhysicsSystem` (positions in world units, velocities per second).
struct Physics : Component
{
    using Component::Component; // Use constructor of component

    tg::vec3 velocity = tg::vec3::zero; ///< Velocity in world units per second.

    /**
     * Position before the last physics update, written once per update before the move.
     *
     * `Factory` placements also set it, so a freshly placed instance does not appear to move.
     * Read to estimate the velocity for leading arrow shots, as the swept segment of an arrow's hit test,
     * and by the `CollisionSystem` as fallback for units squeezed between structures.
     */
    tg::vec3 lastPosition = tg::vec3::zero;

    bool forceGround = true; ///< Snap the height to the terrain every update.
    bool gravity = false;    ///< Accelerate downwards (-y) by 9.81 per second squared.

    bool arrowArc = false; ///< Follow the arc (see `arcEq`) in y instead of `velocity.y` (arrows).
    float arc_x = 0;       ///< Horizontal distance travelled along the arc.
    float arc_xMax = 0;    ///< Horizontal length of the arc; must be positive while `arrowArc` is set.
    float arc_height = 0;  ///< Height of the arc above the straight line from start to end, at its middle.
    float arc_yStart = 0;  ///< Start height of the arc.
    float arc_yEnd = 0;    ///< End height of the arc (at `arc_xMax`).
    float arc_ySlope = 0;  ///< Current slope dy/dx of the arc; `AnimationSystem` aligns arrows to it.
};
}
