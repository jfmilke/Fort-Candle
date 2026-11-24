#pragma once

#include <typed-geometry/tg.hh>

namespace gamedev
{
/**
 * Returns the twist of a rotation about the x axis (swing-twist decomposition).
 *
 * @param q Rotation to decompose.
 * @return Rotation about the x axis only.
 */
inline tg::quat xRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.x, q.w);

    // quaternion representing rotation about the x axis
    return tg::quat::from_axis_angle(tg::dir3::pos_x, 2.f * theta);
}

/**
 * Returns the twist of a rotation about the y (up) axis, i.e. its heading on the ground plane.
 *
 * @param q Rotation to decompose.
 * @return Rotation about the y axis only.
 */
inline tg::quat yRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.y, q.w);

    // quaternion representing rotation about the y axis
    return tg::quat::from_axis_angle(tg::dir3::pos_y, 2.f * theta);
}

/**
 * Returns the twist of a rotation about the z axis (swing-twist decomposition).
 *
 * @param q Rotation to decompose.
 * @return Rotation about the z axis only.
 */
inline tg::quat zRotation(const tg::quat& q)
{
    tg::angle_t<tg::f32> theta = tg::atan2(q.z, q.w);

    // quaternion representing rotation about the z axis
    return tg::quat::from_axis_angle(tg::dir3::pos_z, 2.f * theta);
}

/**
 * Placement of an instance in world space: translation, rotation and per-axis scale, applied as T * R * S.
 *
 * The `*2D` functions describe the instance on the ground plane (x/z, y is up) for 2D collision.
 * They only use the rotation about the y axis; `linear_mat2D` and `scale_mat2D` also ignore the y scale.
 */
struct transform
{
    tg::vec3 translation = {0, 0, 0}; ///< World position.
    tg::size3 scaling = {1, 1, 1};    ///< Scale along the local x, y and z axes.
    tg::quat rotation = {0, 0, 0, 1}; ///< Orientation (x, y, z, w), identity by default.

public:
    /**
     * Moves along the rotated axes.
     *
     * @param dir Offset in local (rotated, unscaled) coordinates.
     */
    void move_relative(tg::vec3 dir)
    {
        auto const m = tg::mat3(rotation);
        translation.x += dot(m.row(0), dir);
        translation.y += dot(m.row(1), dir);
        translation.z += dot(m.row(2), dir);
    }

    /**
     * Moves along the world axes.
     *
     * @param dir Offset in world coordinates.
     */
    void move_absolute(tg::vec3 dir)
    {
        translation.x += dir.x;
        translation.y += dir.y;
        translation.z += dir.z;
    }

    /**
     * Rotates in local space: `rotation = rotation * q1`.
     *
     * @param q1 Rotation to apply, about the local axes.
     */
    void rotate(tg::quat q1)
    {
        rotation = {
            rotation.w * q1.x + rotation.x * q1.w + rotation.y * q1.z - rotation.z * q1.y, //
            rotation.w * q1.y - rotation.x * q1.z + rotation.y * q1.w + rotation.z * q1.x, //
            rotation.w * q1.z + rotation.x * q1.y - rotation.y * q1.x + rotation.z * q1.w, //
            rotation.w * q1.w - rotation.x * q1.x - rotation.y * q1.y - rotation.z * q1.z  //
        };
    }

    /**
     * Multiplies the scale uniformly.
     *
     * @param s Factor for all three axes.
     */
    void scale(float s)
    {
        scaling.width *= s;
        scaling.height *= s;
        scaling.depth *= s;
    }

    /**
     * Multiplies the scale per axis.
     *
     * @param s Factors for the local x, y and z axes.
     */
    void scale(tg::size3 s)
    {
        scaling.width *= s.width;
        scaling.height *= s.height;
        scaling.depth *= s.depth;
    }

    /**
     * Moves the translation linearly towards a target.
     *
     * @param p Target translation.
     * @param alpha Fraction of the way to go, 0..1.
     */
    void lerp_to_translation(tg::vec3 p, float alpha) { translation = tg::lerp(translation, p, alpha); }

    /**
     * Rotates towards a target by spherical interpolation.
     *
     * @param q Target rotation.
     * @param alpha Fraction of the way to go, 0..1.
     */
    void slerp_to_rotation(tg::quat q, float alpha) { rotation = tg::slerp(rotation, q, alpha); }

    /**
     * Like `slerp_to_rotation`, but interpolates the quaternion components linearly without renormalizing.
     *
     * @param q Target rotation.
     * @param alpha Fraction of the way to go, 0..1.
     */
    void inaccurate_lerp_to_rotation(tg::quat q, float alpha) { rotation = tg::lerp(rotation, q, alpha); }

    /**
     * Moves the scale linearly towards a target.
     *
     * @param s Target scale.
     * @param alpha Fraction of the way to go, 0..1.
     */
    void lerp_to_scale(tg::size3 s, float alpha) { scaling = tg::lerp(scaling, s, alpha); }

    /**
     * Interpolates translation, rotation (spherically) and scale towards a target.
     *
     * @param target Target transform.
     * @param alpha Fraction of the way to go, 0..1.
     */
    void lerp_to(transform const& target, float alpha)
    {
        lerp_to_translation(target.translation, alpha);
        slerp_to_rotation(target.rotation, alpha);
        lerp_to_scale(target.scaling, alpha);
    }

    transform() = default;

    /// Unrotated, unscaled transform at `translation`.
    transform(tg::vec3 const& translation) : translation(translation) {}

    /**
     * Unrotated transform with uniform scale.
     *
     * TODO: `scaling` is truncated to an integer (it goes through `tg::isize3`).
     *
     * @param translation World position.
     * @param scaling Scale for all three axes.
     */
    transform(tg::vec3 const& translation, float scaling) : translation(translation), scaling(tg::isize3(scaling, scaling, scaling)) {}

    /**
     * Unrotated transform with integer per-axis scale.
     *
     * @param translation World position.
     * @param scaling Scale along the x, y and z axes.
     */
    transform(tg::vec3 const& translation, tg::isize3 const& scaling) : translation(translation), scaling(scaling) {}

public:
    /// Model matrix T * R * S (local to world) as an affine 4x3 matrix.
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

    /// R * S without translation (last column zero).
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

    /// Rotation as 3x3 matrix.
    tg::mat3 rotation_mat() const { return tg::mat3(rotation); }

    /// Scale as 4x4 matrix.
    tg::mat4 scale_mat() const
    {
        tg::mat4 M{tg::mat4::identity};
        M[0] *= scaling[0];
        M[1] *= scaling[1];
        M[2] *= scaling[2];
        return M;
    }

    /// Model matrix that only keeps the rotation about the y axis (no tilt), with translation and full scale.
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

    /// Rotation about the y axis times the x/z scale (y is not scaled), without translation.
    tg::mat3 linear_mat2D() const
    {
        auto const rot_mat = tg::mat3(yRotation(rotation));
        tg::mat3 M;
        M[0] = rot_mat[0] * scaling[0];
        M[1] = rot_mat[1];
        M[2] = rot_mat[2] * scaling[2];

        return M;
    }

    /// Rotation about the y axis only, as 3x3 matrix.
    tg::mat3 rotation_mat2D() const
    {
        return tg::mat3(yRotation(rotation));
    }

    /// Twist of the rotation about the x axis.
    tg::quat rotation_quat_x() const { return xRotation(rotation); }

    /// Twist of the rotation about the y axis.
    tg::quat rotation_quat_y() const { return yRotation(rotation); }

    /// Twist of the rotation about the z axis.
    tg::quat rotation_quat_z() const { return zRotation(rotation); }

    /// Scale matrix with only the x and z scale (y stays 1).
    tg::mat3 scale_mat2D() const
    {
        tg::mat3 M{tg::mat3::identity};
        M[0] *= scaling[0];
        M[2] *= scaling[2];
        return M;
    }

    /// Translation projected onto the ground plane (y = 0).
    tg::vec3 translation_2D() const
    {
        tg::vec3 V;
        V.x = translation.x;
        V.z = translation.z;
        return V;
    }

    /// Local x axis in world space.
    tg::vec3 right_vec() const { return right_vec_from_rotation(rotation_mat()); }

    /// Local y axis in world space.
    tg::vec3 up_vec() const { return up_vec_from_rotation(rotation_mat()); }

    /// Local z axis in world space.
    tg::vec3 forward_vec() const { return forward_vec_from_rotation(rotation_mat()); }

    /**
     * Like `move_relative`, but returns the result instead of changing this transform.
     *
     * @param dir Offset in local (rotated, unscaled) coordinates.
     * @return The moved translation.
     */
    tg::vec3 get_moved_relative(tg::vec3 dir) const
    {
        auto const m = tg::mat3(rotation);
        auto res = translation;
        res.x += dot(m.row(0), dir);
        res.y += dot(m.row(1), dir);
        res.z += dot(m.row(2), dir);
        return res;
    }

    /// World x axis.
    constexpr static tg::vec3 global_right() { return {1, 0, 0}; }

    /// World y axis (up).
    constexpr static tg::vec3 global_up() { return {0, 1, 0}; }

    /// World z axis.
    constexpr static tg::vec3 global_forward() { return {0, 0, 1}; }

    /// First column of a rotation matrix (local x axis).
    constexpr static tg::vec3 right_vec_from_rotation(tg::mat3 const& m) { return m.col(0); }

    /// Second column of a rotation matrix (local y axis).
    constexpr static tg::vec3 up_vec_from_rotation(tg::mat3 const& m) { return m.col(1); }

    /// Third column of a rotation matrix (local z axis).
    constexpr static tg::vec3 forward_vec_from_rotation(tg::mat3 const& m) { return m.col(2); }
};

/**
 * Combines a child transform, given relative to its parent, into a world transform.
 *
 * TODO: The parent scale is applied after the parent rotation, so this is only exact for a uniform parent scale.
 *
 * @param parent World transform of the parent.
 * @param child Transform relative to `parent`.
 * @return World transform of the child.
 */
inline transform compute_global_transform(transform const& parent, transform const& child)
{
    transform res;
    res.translation = parent.translation + (parent.rotation * child.translation) * parent.scaling;
    res.rotation = tg::normalize(parent.rotation * child.rotation);
    res.scaling = parent.scaling * child.scaling;
    return res;
}

/**
 * Extends an affine matrix to a 4x4 matrix.
 *
 * @param m Affine 4x3 matrix (4 columns, 3 rows), e.g. from `transform::transform_mat`.
 * @return `m` with the last row (0, 0, 0, 1).
 */
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
