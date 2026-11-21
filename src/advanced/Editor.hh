#pragma once

#include <imgui/imgui.h>
// order relevant
#include <imgui/imguizmo.h>
#include <typed-geometry/types/mat.hh>

namespace gamedev
{
struct transform;
struct Physics;
struct BoxShape;

/// Gizmo settings of the in-game editor, kept across frames.
struct EditorState
{
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE; ///< Move, rotate or scale.
    ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;                    ///< Gizmo axes in world or local space (scale is always local).
};

/**
 * Shows the "Editor" window (operation, mode, numeric position/rotation/scale) and an ImGuizmo gizmo for
 * `inoutTransform`, and writes any change back to it. `view`/`proj` are the camera matrices.
 */
void manipulateTransform(EditorState& state, tg::mat4 const& view, tg::mat4 const& proj, transform& inoutTransform);

/// Shows the "Physics" window to edit `body`'s velocity.
void manipulatePhysics(EditorState& state, Physics* body);

/// Shows the "Collision Shape" window with `bshape`'s box in local space and transformed to the ground plane by `xform`.
void showCollisionShape(EditorState& state, BoxShape* bshape, transform& xform);
}
