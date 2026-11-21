#pragma once
#include "components/Component.hh"

namespace gamedev
{
/**
 * Flag for factory presets: instances with this component are never added to any system (see
 * `EngineECS::UpdateSystems`) and only serve as templates to clone.
 */
struct Prototype : public Component
{
    using Component::Component;
};
}
