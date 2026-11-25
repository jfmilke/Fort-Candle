#pragma once
#include <typed-geometry/types/objects/box.hh>
#include <typed-geometry/types/objects/sphere.hh>
#include "components/Component.hh"

namespace gamedev
{
/// Oriented box collision shape in the instance's local space; the `CollisionSystem` uses its ground-plane (x/z) part.
struct BoxShape : public Component
{
    using Component::Component;

    tg::box3 box; ///< Box in local space (before `transform::transform_mat2D()`).

    float minY = 0.f; ///< Bottom of the mesh in local space (set for structures; arrows aim past static colliders).
    float maxY = 0.f; ///< Top of the mesh in local space (set for structures).
};

/// Circle collision shape on the ground plane, in the instance's local space.
struct CircleShape : public Component
{
    using Component::Component;

    tg::sphere3 circle; ///< Circle in local space; only x/z of the center and the radius matter.

    float minY = 0.f; ///< Bottom of the mesh in local space (set for structures; arrows aim past static colliders).
    float maxY = 0.f; ///< Top of the mesh in local space (set for structures).
};
}
