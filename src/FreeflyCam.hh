#pragma once

#include <cmath>

#include <typed-geometry/types/pos.hh>
#include <typed-geometry/types/quat.hh>
#include <typed-geometry/types/vec.hh>

namespace gamedev
{
/// Simple camera state: world position and view direction.
struct FreeflyCamState
{
    tg::pos3 position = tg::pos3(0);      ///< World position.
    tg::vec3 forward = tg::vec3(0, 0, 1); ///< View direction (not necessarily normalized).

    /**
     * Moves by `distance` given in camera axes (z = view direction, see `forwardToRotation`). Without
     * `freeFlyEnabled`, the move is parallel to the ground and only done while the point the camera looks at on the
     * ground stays within ±80 units of the origin.
     */
    void moveRelative(tg::vec3 distance, bool freeFlyEnabled);

    /// Turns the view by `dx` (azimuth) and `dy` (altitude) radians; the altitude is clamped to ±89 degrees.
    void mouselook(float dx, float dy);

    /// Places the camera at `focus + global_offset`, looking at `focus`.
    void setFocus(tg::pos3 focus, tg::vec3 global_offset);
};

/**
 * Smoothed camera with a physical (current) and a target state.
 *
 * Input changes the target state, rendering uses the physical state, and `interpolateToTarget` moves the physical
 * state towards the target every frame.
 */
struct FreeflyCam
{
    FreeflyCamState physical; ///< Current state, used for rendering.

    FreeflyCamState target; ///< State towards which `physical` is interpolated.

    float sensitivityRotation = 70.f; ///< Decay rate (1/s) of the view direction towards the target.
    float sensitivityPosition = 25.f; ///< Decay rate (1/s) of the position towards the target.

    /**
     * Moves `physical` towards `target` (exponential decay over `dt` seconds).
     * Returns true if `physical` changed.
     */
    bool interpolateToTarget(float dt);
};

/**
 * Smoothed lerp alpha, framerate-correct: the fraction of the remaining distance covered in `dt` if `smoothing` of it
 * remains after one second.
 */
inline float smoothLerpAlpha(float smoothing, float dt) { return 1 - std::pow(smoothing, dt); }

/// Exponential decay alpha, framerate-correct damp / lerp with rate `lambda` (1/s).
inline float exponentialDecayAlpha(float lambda, float dt) { return 1 - std::exp(-lambda * dt); }

/// Alpha based on the halftime (seconds) between current and target state.
inline float halftimeLerpAlpha(float halftime, float dt) { return 1 - std::pow(.5f, dt / halftime); }

/**
 * Rotation whose matrix rows are the camera's side, up and view axes for view direction `fwd` (world to camera axes);
 * its transpose maps camera-axis offsets to world space.
 */
tg::quat forwardToRotation(tg::vec3 fwd, tg::vec3 up = {0, 1, 0});
}
