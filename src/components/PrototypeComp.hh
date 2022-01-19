#pragma once
#include "components/Component.hh"

namespace gamedev
{
// Just a flag to prevent any system from using this instance
struct Prototype : public Component
{
    using Component::Component;
};
}