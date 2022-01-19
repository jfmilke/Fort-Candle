#include "Editor.hh"
#include "components/PhysicsComp.hh"
#include <typed-geometry/tg.hh>
#include "components/ShapeComp.hh"
#include "transform.hh"

void gamedev::manipulateTransform(EditorState& state, tg::mat4 const& view, tg::mat4 const& proj, transform& inoutTransform)
{
    tg::mat4 mat = affine_to_mat4(inoutTransform.transform_mat());

    // run imguizmo on mat
    {
        if (ImGui::Begin("Editor"))
        {
            if (ImGui::RadioButton("Move", state.operation == ImGuizmo::OPERATION::TRANSLATE))
                state.operation = ImGuizmo::OPERATION::TRANSLATE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", state.operation == ImGuizmo::OPERATION::ROTATE))
                state.operation = ImGuizmo::OPERATION::ROTATE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", state.operation == ImGuizmo::OPERATION::SCALE))
                state.operation = ImGuizmo::OPERATION::SCALE;

            bool is_local = state.mode == ImGuizmo::MODE::LOCAL || state.operation == ImGuizmo::OPERATION::SCALE;
            if (ImGui::RadioButton("Local", is_local))
                state.mode = ImGuizmo::MODE::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", !is_local))
                state.mode = state.operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;

            ImGui::Separator();

            float pos[3];
            float rot[3];
            float scl[3];
            ImGuizmo::DecomposeMatrixToComponents(tg::data_ptr(mat), pos, rot, scl);

            bool changes = false;

            if (ImGui::InputFloat3("Position", pos))
                changes = true;
            if (ImGui::InputFloat3("Rotation", rot))
                changes = true;
            if (ImGui::InputFloat3("Scale", scl))
                changes = true;

            if (changes)
                ImGuizmo::RecomposeMatrixFromComponents(pos, rot, scl, tg::data_ptr(mat));
        }
        ImGui::End();

        state.mode = state.operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : state.mode;
        ImGuizmo::Manipulate(tg::data_ptr(view), tg::data_ptr(proj), state.operation, state.mode, tg::data_ptr(mat));
    }

    // re-assemble transform
    tg::mat3 rotation;
    tg::decompose_transformation(mat, inoutTransform.translation, rotation, inoutTransform.scaling);
    inoutTransform.rotation = tg::quat::from_rotation_matrix(rotation);
}

void gamedev::manipulatePhysics(EditorState& state, Physics* body)
{
    // run imguizmo on mat
    {
        if (ImGui::Begin("Physics"))
        {
            float vel[3];
            vel[0] = body->velocity.x;
            vel[1] = body->velocity.y;
            vel[2] = body->velocity.z; 

            bool changes = false;

            if (ImGui::InputFloat3("Velocity", vel))
            {
                changes = true;
            }

            if (changes)
            {
                body->velocity.x = vel[0];
                body->velocity.y = vel[1];
                body->velocity.z = vel[2];
            }
        }
        ImGui::End();
    }
}

void gamedev::showCollisionShape(EditorState& state, BoxShape* bshape, transform& xform)
{
    // run imguizmo on mat
    {
        if (ImGui::Begin("Collision Shape"))
        {
            ImGui::Text("Before Transformation");
            ImGui::Text("Box Center: %.2f %.2f %.2f", bshape->box.center.x, bshape->box.center.y, bshape->box.center.z);
            ImGui::Text("Box Half X: %.2f %.2f %.2f", bshape->box.half_extents[0].x, bshape->box.half_extents[0].y, bshape->box.half_extents[0].z);
            ImGui::Text("Box Half Y: %.2f %.2f %.2f", bshape->box.half_extents[1].x, bshape->box.half_extents[1].y, bshape->box.half_extents[1].z);
            ImGui::Text("Box Half Z: %.2f %.2f %.2f", bshape->box.half_extents[2].x, bshape->box.half_extents[2].y, bshape->box.half_extents[2].z);

            auto t = tg::mat4(xform.transform_mat2D());
            t[3][3] = 1.0f;
            auto t_box = bshape->box;
            t_box.center = t * t_box.center;
            t_box.half_extents[0] = t * t_box.half_extents[0];
            t_box.half_extents[1] = t * t_box.half_extents[1];
            t_box.half_extents[2] = t * t_box.half_extents[2];

            ImGui::Text("After Transformation");
            ImGui::Text("Box Center: %.2f %.2f %.2f", t_box.center.x, t_box.center.y, t_box.center.z);
            ImGui::Text("Box Half X: %.2f %.2f %.2f", t_box.half_extents[0].x, t_box.half_extents[0].y, t_box.half_extents[0].z);
            ImGui::Text("Box Half Y: %.2f %.2f %.2f", t_box.half_extents[1].x, t_box.half_extents[1].y, t_box.half_extents[1].z);
            ImGui::Text("Box Half Z: %.2f %.2f %.2f", t_box.half_extents[2].x, t_box.half_extents[2].y, t_box.half_extents[2].z);
        }
        ImGui::End();
    }
}
