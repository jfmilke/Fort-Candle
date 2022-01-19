#pragma once

#include <cmath>

#include <typed-geometry/types/pos.hh>
#include <typed-geometry/types/quat.hh>
#include <typed-geometry/types/vec.hh>

namespace gamedev
{
// simple camera state
struct FreeflyCamState
{
    tg::pos3 position = tg::pos3(0);
    tg::vec3 forward = tg::vec3(0, 0, 1);

    void moveRelative(tg::vec3 distance, bool freeFlyEnabled);
    void mouselook(float dx, float dy);
    void setFocus(tg::pos3 focus, tg::vec3 global_offset);
};

// smoothed camera, has a physical (current) and target state
struct FreeflyCam
{
    // current state
    FreeflyCamState physical;

    // state toward which this interpolates
    FreeflyCamState target;

    float sensitivityRotation = 70.f;
    float sensitivityPosition = 25.f;

    // interpolates physical state towards target based on delta time and sensitivities
    // returns true if changes to physical were made
    bool interpolateToTarget(float dt);
};

// Smoothed lerp alpha, framerate-correct
inline float smoothLerpAlpha(float smoothing, float dt) { return 1 - std::pow(smoothing, dt); }

// Exponential decay alpha, framerate-correct damp / lerp
inline float exponentialDecayAlpha(float lambda, float dt) { return 1 - std::exp(-lambda * dt); }

// alpha based on the halftime between current and target state
inline float halftimeLerpAlpha(float halftime, float dt) { return 1 - std::pow(.5f, dt / halftime); }

tg::quat forwardToRotation(tg::vec3 fwd, tg::vec3 up = {0, 1, 0});
}
