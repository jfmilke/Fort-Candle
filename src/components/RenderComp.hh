#pragma once
#include "advanced/transform.hh"
#include "components/Component.hh"

namespace gamedev
{
/// Marks an instance to be drawn by the `RenderSystem` and controls mouse interaction with it.
struct Render : Component
{
    using Component::Component;

    transform* inheritTransform = nullptr; ///< Unused.
    bool selectable = false;               ///< Can be selected with a left click.
    bool targetable = false;               ///< Can be targeted with a right click (e.g. as attack target).
};
}
