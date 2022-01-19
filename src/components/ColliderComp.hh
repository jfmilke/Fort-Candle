#pragma once
#include "components/Component.hh"

namespace gamedev
{
struct Collider : public Component
{
    using Component::Component;

    bool dynamic = false;
};
}