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

struct EditorState
{
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;
};

void manipulateTransform(EditorState& state, tg::mat4 const& view, tg::mat4 const& proj, transform& inoutTransform);
void manipulatePhysics(EditorState& state, Physics* body);
void showCollisionShape(EditorState& state, BoxShape* bshape, transform& xform);
}
