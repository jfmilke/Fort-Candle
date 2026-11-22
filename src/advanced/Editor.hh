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
 * Edits a transform with the "Editor" window and an ImGuizmo gizmo.
 *
 * The window selects the operation and mode and shows position, rotation (Euler angles in degrees) and scale as numbers.
 * Any change from the window or the gizmo is written back.
 * Scaling always uses local mode.
 *
 * @param state Gizmo settings; updated by the window.
 * @param view Camera view matrix.
 * @param proj Camera projection matrix.
 * @param[in,out] inoutTransform Transform to edit, usually the selected instance's.
 */
void manipulateTransform(EditorState& state, tg::mat4 const& view, tg::mat4 const& proj, transform& inoutTransform);

/**
 * Shows the "Physics" window to edit a body's velocity.
 *
 * @param state Unused.
 * @param body Physics component to edit.
 */
void manipulatePhysics(EditorState& state, Physics* body);

/**
 * Debug: shows the "Collision Shape" window with a box before and after its 2D transformation.
 *
 * The box is transformed by `transform::transform_mat2D`. `Game::onGui` has the call disabled (`if (false)`).
 *
 * @param state Unused.
 * @param bshape Box shape to show.
 * @param xform Transform of the shape's instance.
 */
void showCollisionShape(EditorState& state, BoxShape* bshape, transform& xform);
}
