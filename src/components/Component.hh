#pragma once
#include <glow/fwd.hh>
#include <concepts>
#include <cstdint>

namespace gamedev
{
GLOW_SHARED(struct, Component);
GLOW_SHARED(struct, Collider);
GLOW_SHARED(struct, BoxShape);
GLOW_SHARED(struct, CircleShape);

/**
 * Base of all components: plain data structs attached to an instance through the ECS.
 *
 * Credits go to RTG Assignment 2.
 *
 *  - Components are stored by value in a `ComponentArray` (copied on insert, moved around on removal), so they must stay copyable.
 *  - The base is not polymorphic (no virtual destructor): components are never used or deleted through a `Component*`.
 *    Type-erased operations on all components of an entity belong on `IComponentArray`, which knows the concrete type.
 *  - A new component type must be registered with `EngineECS::RegisterComponent` in `Game::init()` before use.
 *  - Derived types inherit the constructors (`using Component::Component;`).
 */
struct Component
{
    uint32_t handle_value = uint32_t(-1); ///< Handle value of the owning instance (set by the ECS on insert and clone).

    /// Creates a component that is not attached to an instance yet.
    Component() = default;
};

/// What the ECS requires of a component type: derived from `Component`, default-constructible and copyable.
template <class T>
concept EcsComponent = std::derived_from<T, Component> && std::default_initializable<T> && std::copyable<T>;
}
