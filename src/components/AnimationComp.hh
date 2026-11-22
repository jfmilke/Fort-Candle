#pragma once
#include "components/Component.hh"

namespace gamedev
{
/**
 * Pending rotation and scale animations, played by the `AnimationSystem`.
 *
 * Each animation runs while its flag is set; `scaleAnim` resets itself when the target is reached.
 *
 * TODO: The remaining angles `rot_x`/`rot_y`/`rot_z` only decay exponentially and never reach zero, so their flags stay set
 * (harmless: the game sets a new `rot_y` with every turn).
 */
struct Animated : Component
{
    using Component::Component; // Use constructor of component

    bool rotXAnim = false;                  ///< Rotate by the remaining `rot_x` about the local x axis.
    tg::angle rot_x = 0_deg;                ///< Remaining rotation about x; each update applies the fraction 7 * dt of it.
    bool rotYAnim = false;                  ///< Rotate by the remaining `rot_y` about the local y axis.
    tg::angle rot_y = 0_deg;                ///< Remaining rotation about y, see `rot_x`.
    bool rotZAnim = false;                  ///< Rotate by the remaining `rot_z` about the local z axis.
    tg::angle rot_z = 0_deg;                ///< Remaining rotation about z, see `rot_x`.
    bool rotQuatAnim = false;               ///< Turn towards `rot_quat` (never resets itself).
    tg::quat rot_quat = tg::quat::identity; ///< Target orientation for `rotQuatAnim`.
    float rot_speed = 12.0f;                ///< Interpolation rate towards `rot_quat` per second.

    bool scaleAnim = false; ///< Scale uniformly towards `scale` by at most 1 per second.
    float scale = 0.0f;     ///< Target uniform scale (compared with the current z scale).
};
}
