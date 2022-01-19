#pragma once
#include <glow/fwd.hh>
#include <cstdint>

namespace gamedev
{
GLOW_SHARED(struct, Component);
GLOW_SHARED(struct, Collider);
GLOW_SHARED(struct, BoxShape);
GLOW_SHARED(struct, CircleShape);

typedef struct Instance Instance;

// Base Component Struct
// Components are POD
// ... Credits go to RTG Assignment 2 ;)
struct Component
{
    // Backreference from Component to Entity
    Instance* instance = nullptr;
    uint32_t handle_value = uint32_t(-1);

    Component() = default;
    // A component can only be created for a given entity
    Component(Instance* i) : instance(i) {}

    // Forbid copying of components to prevent usage errors
    // Component(Component const&) = delete;
    // Component& operator=(Component const&) = delete;

    // Destructor must be virtual in order to safely delete subclasses
    virtual ~Component() {}
};
}