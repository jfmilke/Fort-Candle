#pragma once
#include "components/Component.hh"
#include <typed-geometry/types/objects/box.hh>
#include <typed-geometry/types/objects/sphere.hh>

namespace gamedev
{
// Oriented Bounding Box or Bounding Circle
struct BoxShape : public Component
{
    using Component::Component;

    tg::box3 box;
};

struct CircleShape : public Component
{
    using Component::Component;

    tg::sphere3 circle;
};
}