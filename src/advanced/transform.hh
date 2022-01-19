#pragma once

#include <typed-geometry/tg.hh>

namespace gamedev
{
inline tg::quat xRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.x, q.w);

    // quaternion representing rotation about the x axis
    return tg::quat::from_axis_angle(tg::dir3::pos_x, 2.f * theta);
}

inline tg::quat yRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.y, q.w);

    // quaternion representing rotation about the y axis
    return tg::quat::from_axis_angle(tg::dir3::pos_y, 2.f * theta);
}

inline tg::quat zRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.z, q.w);

    // quaternion representing rotation about the z axis
    return tg::quat::from_axis_angle(tg::dir3::pos_z, 2.f * theta);
}

struct transform
{
    tg::vec3 translation = {0, 0, 0};
    tg::size3 scaling = {1, 1, 1};
    tg::quat rotation = {0, 0, 0, 1};

public:
    void move_relative(tg::vec3 dir)
    {
        auto const m = tg::mat3(rotation);
        translation.x += dot(m.row(0), dir);
        translation.y += dot(m.row(1), dir);
        translation.z += dot(m.row(2), dir);
    }

    void move_absolute(tg::vec3 dir)
    {
        translation.x += dir.x;
        translation.y += dir.y;
        translation.z += dir.z;
    }

    void rotate(tg::quat q1)
    {
        rotation = {
            rotation.w * q1.x + rotation.x * q1.w + rotation.y * q1.z - rotation.z * q1.y, //
            rotation.w * q1.y - rotation.x * q1.z + rotation.y * q1.w + rotation.z * q1.x, //
            rotation.w * q1.z + rotation.x * q1.y - rotation.y * q1.x + rotation.z * q1.w, //
            rotation.w * q1.w - rotation.x * q1.x - rotation.y * q1.y - rotation.z * q1.z  //
        };
    }

    void scale(float s)
    {
        scaling.width *= s;
        scaling.height *= s;
        scaling.depth *= s;
    }

    void scale(tg::size3 s)
    {
        scaling.width *= s.width;
        scaling.height *= s.height;
        scaling.depth *= s.depth;
    }

    void lerp_to_translation(tg::vec3 p, float alpha) { translation = tg::lerp(translation, p, alpha); }
    void slerp_to_rotation(tg::quat q, float alpha) { rotation = tg::slerp(rotation, q, alpha); }

    void inaccurate_lerp_to_rotation(tg::quat q, float alpha) { rotation = tg::lerp(rotation, q, alpha); }

    void lerp_to_scale(tg::size3 s, float alpha) { scaling = tg::lerp(scaling, s, alpha); }
    void lerp_to(transform const& target, float alpha)
    {
        lerp_to_translation(target.translation, alpha);
        slerp_to_rotation(target.rotation, alpha);
        lerp_to_scale(target.scaling, alpha);
    }

    transform() = default;
    transform(tg::vec3 const& translation) : translation(translation) {}
    transform(tg::vec3 const& translation, float scaling) : translation(translation), scaling(tg::isize3(scaling, scaling, scaling)) {}
    transform(tg::vec3 const& translation, tg::isize3 const& scaling) : translation(translation), scaling(scaling) {}

public:
    tg::mat4x3 transform_mat() const
    {
        // note: this is significantly faster than three naive matrix muls!
        auto const rot_mat = rotation_mat();
        tg::mat4x3 M;
        M[0] = rot_mat[0] * scaling[0];
        M[1] = rot_mat[1] * scaling[1];
        M[2] = rot_mat[2] * scaling[2];
        M[3] = translation;

        return M;
    }

    tg::mat4x3 linear_transform_mat() const
    {
        // note: this is significantly faster than three naive matrix muls!
        auto const rot_mat = rotation_mat();
        tg::mat4x3 M;
        M[0] = rot_mat[0] * scaling[0];
        M[1] = rot_mat[1] * scaling[1];
        M[2] = rot_mat[2] * scaling[2];
        return M;
    }

    tg::mat3 rotation_mat() const { return tg::mat3(rotation); }
   
    tg::mat4 scale_mat() const
    {
        tg::mat4 M{tg::mat4::identity};
        M[0] *= scaling[0];
        M[1] *= scaling[1];
        M[2] *= scaling[2];
        return M;
    }

    tg::mat4 transform_mat2D() const
    {
        auto const rot_mat = tg::mat4(yRotation(rotation));
        
        tg::mat4 M;
        M[0] = rot_mat[0] * scaling[0];
        M[1] = rot_mat[1] * scaling[1];
        M[2] = rot_mat[2] * scaling[2];
        M[3][0] = translation.x;
        M[3][1] = translation.y;
        M[3][2] = translation.z;
        M[3][3] = 1.f;
        return M;
    }

    // No translation
    tg::mat3 linear_mat2D() const
    {
        auto const rot_mat = tg::mat3(yRotation(rotation));
        tg::mat3 M;
        M[0] = rot_mat[0] * scaling[0];
        M[1] = rot_mat[1];
        M[2] = rot_mat[2] * scaling[2];

        return M;
    }

    tg::mat3 rotation_mat2D() const
    {
        return tg::mat3(yRotation(rotation));
    }

    tg::quat rotation_quat_x() const { return xRotation(rotation); }
    tg::quat rotation_quat_y() const { return yRotation(rotation); }
    tg::quat rotation_quat_z() const { return zRotation(rotation); }

    // 2D: Scale in x & z direction
    tg::mat3 scale_mat2D() const
    {
        tg::mat3 M{tg::mat3::identity};
        M[0] *= scaling[0];
        M[2] *= scaling[2];
        return M;
    }

    // 2D: Scale in x & z direction
    tg::vec3 translation_2D() const
    {
        tg::vec3 V;
        V.x = translation.x;
        V.z = translation.z;
        return V;
    }

    tg::vec3 right_vec() const { return right_vec_from_rotation(rotation_mat()); }
    tg::vec3 up_vec() const { return up_vec_from_rotation(rotation_mat()); }
    tg::vec3 forward_vec() const { return forward_vec_from_rotation(rotation_mat()); }

    tg::vec3 get_moved_relative(tg::vec3 dir) const
    {
        auto const m = tg::mat3(rotation);
        auto res = translation;
        res.x += dot(m.row(0), dir);
        res.y += dot(m.row(1), dir);
        res.z += dot(m.row(2), dir);
        return res;
    }

    constexpr static tg::vec3 global_right() { return {1, 0, 0}; }
    constexpr static tg::vec3 global_up() { return {0, 1, 0}; }
    constexpr static tg::vec3 global_forward() { return {0, 0, 1}; }

    constexpr static tg::vec3 right_vec_from_rotation(tg::mat3 const& m) { return m.col(0); }
    constexpr static tg::vec3 up_vec_from_rotation(tg::mat3 const& m) { return m.col(1); }
    constexpr static tg::vec3 forward_vec_from_rotation(tg::mat3 const& m) { return m.col(2); }
};

inline transform compute_global_transform(transform const& parent, transform const& child)
{
    transform res;
    res.translation = parent.translation + (parent.rotation * child.translation) * parent.scaling;
    res.rotation = tg::normalize(parent.rotation * child.rotation);
    res.scaling = parent.scaling * child.scaling;
    return res;
}

inline tg::mat4 affine_to_mat4(tg::mat4x3 const& m)
{
    tg::mat4 res;
    res[0] = tg::vec4(m[0], 0.f);
    res[1] = tg::vec4(m[1], 0.f);
    res[2] = tg::vec4(m[2], 0.f);
    res[3] = tg::vec4(m[3], 1.f);
    return res;
}
}
