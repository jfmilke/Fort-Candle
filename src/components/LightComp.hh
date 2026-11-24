#pragma once
#include "components/Component.hh"
#include "types/Light.hh"
#include <queue>

namespace gamedev
{
/// Point light that moves with its instance; collected each update by the `LightSystem`.
struct PointLightEmitter: Component
{
    using Component::Component;

    PointLight pl = {{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, 10.f}; ///< Light with its position in the instance's local space.

    bool flicker = true;                      ///< Scale the intensity by a smoothed random factor every update.
    float flicker_min = 0.0;                  ///< Lower bound of the random flicker factor.
    float flicker_max = 1.0;                  ///< Upper bound of the random flicker factor.
    int flicker_smoothing = 30;               ///< Number of recent random samples averaged (meant to be 1 to 50).
    float flicker_sum = 0.0;                  ///< Sum of the samples in `flicker_smoothingQueue`.
    std::queue<float> flicker_smoothingQueue; ///< Most recent random samples.

    bool shadowing = false; ///< This is the shadow-casting point light (only one is supported; the last one wins).
};
}
