#pragma once
#include "components/Component.hh"
#include "types/Light.hh"
#include <queue>

namespace gamedev
{
struct PointLightEmitter: Component
{
    using Component::Component;

    PointLight pl = {{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, 10.f};

    bool flicker = true;
    float flicker_min = 0.0;
    float flicker_max = 1.0;
    int flicker_smoothing = 30; // Range (1, 50)
    float flicker_sum = 0.0;
    std::queue<float> flicker_smoothingQueue;

    bool shadowing = false;
};
}