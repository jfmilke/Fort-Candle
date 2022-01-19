#pragma once
#include "components/Component.hh"

namespace gamedev
{
struct arcEq
{
    float xMax;
    float yMax;
    float yBase;
    tg::vec3 velocity;
};

struct Physics : Component
{
    using Component::Component; // Use constructor of component

    tg::vec3 velocity = tg::vec3::zero;
    tg::vec3 lastPosition = tg::vec3::zero;

    bool forceGround = true;
    bool gravity = false;

    bool arrowArc = false;
    float arc_x = 0;
    float arc_xMax = 0;
    float arc_yMax = 0;
    float arc_yBase = 0;
    float arc_ySlope = 0;
};
}