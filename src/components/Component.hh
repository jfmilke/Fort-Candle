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

/**
 * Base of all components: plain data structs attached to an instance through the ECS.
 *
 * Credits go to RTG Assignment 2.
 *
 *  - Components are stored by value in a `ComponentArray` (copied on insert, moved around on removal), so they must stay copyable.
 *  - A new component type must be registered with `EngineECS::RegisterComponent` in `Game::init()` before use.
 *  - Derived types inherit the constructors (`using Component::Component;`).
 */
struct Component
{
    /**
     * Back-pointer to the owning instance. Prefer `EngineECS::GetInstance({handle_value})`.
     *
     * TODO: Never read, and can be stale: the `World` moves instances when another one is destroyed.
     * Remove it or keep it updated.
     */
    Instance* instance = nullptr;
    uint32_t handle_value = uint32_t(-1); ///< Handle value of the owning instance.

    /// Creates a component that is not attached to an instance yet.
    Component() = default;

    /**
     * Creates a component for an instance.
     *
     * @param i Owning instance. `handle_value` is set separately (by `ComponentManager::CreateComponent`).
     */
    Component(Instance* i) : instance(i) {}

    // Destructor must be virtual in order to safely delete subclasses
    virtual ~Component() {}
};
}
