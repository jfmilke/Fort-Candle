#pragma once
#include "components/Component.hh"

namespace gamedev
{
struct Animated : Component
{
    using Component::Component; // Use constructor of component

    bool rotXAnim = false;
    tg::angle rot_x = 0_deg;
    bool rotYAnim = false;
    tg::angle rot_y = 0_deg;
    bool rotZAnim = false;
    tg::angle rot_z = 0_deg;
    bool rotQuatAnim = false;
    tg::quat rot_quat = tg::quat::identity;
    float rot_speed = 12.0f;

    bool scaleAnim = false;
    float scale = 0.0f;
};
}