#pragma once
#include "components/Component.hh"

namespace gamedev
{
/// Movement orders for the `InputSystem`. Unused: neither is registered by `Game`.
struct Input : public Component
{
    using Component::Component;

    tg::pos3 targetMove = tg::pos3(0.f, 0.f, 0.f); ///< Position to move to.
    tg::vec3 targetLook = tg::vec3(1.f, 0.f, 0.f); ///< Direction to look in.
    tg::vec3 currentLook = targetLook;             ///< Current look direction.
    float rotationSpeed = 1.f;                     ///< Turn speed.
    float runningSpeed = 10.f;                     ///< Movement speed in world units per second.
    bool refresh = false;                          ///< Set when a target changed.

    InstanceHandle focusInstance; ///< Instance to follow and/or look at.
    bool focusMove = false;       ///< Follow `focusInstance`.
    bool focusLook = false;       ///< Look at `focusInstance`.
};
}
