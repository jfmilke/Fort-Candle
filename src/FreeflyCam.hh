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
     * Moves the camera in camera axes.
     *
     * Without `freeFlyEnabled`, the axes are those of the view direction projected onto the ground (x/z moves are parallel to the ground),
     * and the move is only done while the point the camera looks at on the ground stays within ±80 units of the origin in x and z.
     *
     * @param distance Offset in camera axes: x = side (+x is left), y = up, z = view direction (see `forwardToRotation`).
     * @param freeFlyEnabled Use the full view direction and skip the map border check.
     */
    void moveRelative(tg::vec3 distance, bool freeFlyEnabled);

    /**
     * Turns the view; the altitude is clamped to ±89 degrees.
     *
     * @param dx Azimuth change in radians.
     * @param dy Altitude change in radians; positive looks down.
     */
    void mouselook(float dx, float dy);

    /**
     * Places the camera at `focus + global_offset`, looking at `focus`.
     *
     * @param focus World position to look at.
     * @param global_offset Camera offset from `focus` in world space.
     */
    void setFocus(tg::pos3 focus, tg::vec3 global_offset);
};

/**
 * Smoothed camera with a physical (current) and a target state.
 *
 * Input changes the target state, rendering uses the physical state,
 * and `interpolateToTarget` moves the physical state towards the target every frame.
 */
struct FreeflyCam
{
    FreeflyCamState physical; ///< Current state, used for rendering.

    FreeflyCamState target; ///< State towards which `physical` is interpolated.

    float sensitivityRotation = 70.f; ///< Decay rate (1/s) of the view direction towards the target.
    float sensitivityPosition = 25.f; ///< Decay rate (1/s) of the position towards the target.

    /**
     * Moves `physical` towards `target` by exponential decay (see `sensitivityRotation`, `sensitivityPosition`) and normalizes its view direction.
     *
     * Does nothing once the summed squared differences are below `tg::epsilon`.
     *
     * @param dt Time step in seconds.
     * @return True if `physical` changed.
     */
    bool interpolateToTarget(float dt);
};

/**
 * Smoothed lerp alpha, framerate-correct.
 *
 * @param smoothing Fraction of the distance that remains after one second.
 * @param dt Time step in seconds.
 * @return Fraction of the remaining distance to cover in `dt`.
 */
inline float smoothLerpAlpha(float smoothing, float dt) { return 1 - std::pow(smoothing, dt); }

/**
 * Exponential decay alpha, framerate-correct damp / lerp.
 *
 * @param lambda Decay rate in 1/s.
 * @param dt Time step in seconds.
 * @return Fraction of the remaining distance to cover in `dt`.
 */
inline float exponentialDecayAlpha(float lambda, float dt) { return 1 - std::exp(-lambda * dt); }

/**
 * Lerp alpha from a halftime, framerate-correct.
 *
 * @param halftime Time in seconds after which half of the distance remains.
 * @param dt Time step in seconds.
 * @return Fraction of the remaining distance to cover in `dt`.
 */
inline float halftimeLerpAlpha(float halftime, float dt) { return 1 - std::pow(.5f, dt / halftime); }

/**
 * Computes the camera rotation for a view direction.
 *
 * @param fwd View direction (normalized internally).
 * @param up World up direction.
 * @return Rotation whose matrix rows are the camera's side (left), up and view axes (world to camera axes);
 *         its transpose maps camera-axis offsets to world space.
 */
tg::quat forwardToRotation(tg::vec3 fwd, tg::vec3 up = {0, 1, 0});
}
