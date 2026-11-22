#pragma once
#include "components/Component.hh"

namespace gamedev
{
/**
 * Makes an instance take part in 2D collision detection (`CollisionSystem`); the shape comes from a `BoxShape` or
 * `CircleShape` component.
 */
struct Collider : public Component
{
    using Component::Component;

    bool dynamic = false; ///< Moves and is pushed out of other colliders; static colliders never move.
};
}
