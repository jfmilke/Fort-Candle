#pragma once

#include <vector>

#include <glow-extras/colors/color.hh>

#include "CameraController.hh"
#include "detail/MeshAttribute.hh"
#include "detail/config_structs_impl.hh"
#include "fwd.hh"
#include "renderables/GeometricRenderable.hh"
#include "traits.hh"

// view configuration
// is called from the variadic view(obj, ...) function

namespace glow::viewer
{
// Config settings
struct background_color
{
    tg::color3 inner;
    tg::color3 outer;
    constexpr explicit background_color(tg::color3 in, tg::color3 out) : inner(std::move(in)), outer(std::move(out)) {}
    constexpr explicit background_color(tg::color3 c) : inner(c), outer(std::move(c)) {}
};
struct ssao_power
{
    float power;
    constexpr explicit ssao_power(float s) : power(s) {}
};
struct ssao_radius
{
    float radius;
    constexpr explicit ssao_radius(float r) : radius(r) {}
};
struct grid_size
{
    float size;
    constexpr explicit grid_size(float s) : size(s) {}
};
struct grid_center
{
    tg::pos3 center;
    constexpr explicit grid_center(tg::pos3 c) : center(c) {}
};
struct camera_orientation
{
    tg::angle azimuth;
    tg::angle altitude;
    float distance;

    constexpr explicit camera_orientation(tg::angle azimuth, tg::angle altitude, float distance = -1.f)
      : azimuth(azimuth), altitude(altitude), distance(distance)
    {
    }
};
struct orthogonal_projection
{
    tg::aabb3 bounds;

    constexpr explicit orthogonal_projection(float left, float right, float bottom, float top, float near, float far)
      : bounds{{left, bottom, near}, {right, top, far}}
    {
    }

    constexpr explicit orthogonal_projection(tg::aabb3 const& bounds) : bounds(bounds) {}
};
struct camera_transform
{
    tg::pos3 pos;
    tg::pos3 target;
    constexpr explicit camera_transform(tg::pos3 pos, tg::pos3 target) : pos(pos), target(target) {}
};
struct clip_plane
{
    tg::pos3 pos;
    tg::vec3 normal;
    clip_plane(tg::pos3 p, tg::vec3 n) : pos(p), normal(n) {}
};

/// additional keys for closing the viewer
/// works with any GLFW_KEY_xyz
/// Some ascii chars work as well, e.g. capital letter 'A', ...
struct close_keys
{
    std::vector<int> keys;

    template <class... Args>
    explicit close_keys(Args... args)
    {
        (keys.push_back(args), ...);
    }
    explicit close_keys(tg::span<int> keys)
    {
        for (auto k : keys)
            this->keys.push_back(k);
    }
};

struct headless_screenshot
{
    std::string filename;
    tg::ivec2 resolution;
    int accumulationCount;
    GLenum format;

    explicit headless_screenshot(tg::ivec2 resolution = tg::ivec2(3840, 2160), int accumulationCount = 64, std::string_view filename = "viewer_screen.png", GLenum format = GL_RGB8)
      : filename(filename), resolution(resolution), accumulationCount(accumulationCount), format(format)
    {
    }
};

// Config tags
inline auto constexpr no_grid = no_grid_t{};
inline auto constexpr no_left_mouse_control = no_left_mouse_control_t{};
inline auto constexpr no_right_mouse_control = no_right_mouse_control_t{};
inline auto constexpr no_shadow = no_shadow_t{};
inline auto constexpr no_shading = no_shading_t{};
inline auto constexpr maybe_empty = maybe_empty_t{};
inline auto constexpr print_mode = print_mode_t{};
inline auto constexpr no_outline = no_outline_t{};
inline auto constexpr no_ssao = ssao_power(0.f);
inline auto constexpr controls_2d = controls_2d_t{};
inline auto constexpr transparent = transparent_t{};
inline auto constexpr opaque = opaque_t{};
inline auto constexpr no_fresnel = no_fresnel_t{};
inline auto constexpr clear_accumulation = clear_accumulation_t{};
inline auto constexpr dark_ui = dark_ui_t{};
inline auto constexpr subview_margin = subview_margin_t{};
inline auto constexpr infinite_accumulation = infinite_accumulation_t{};
inline auto constexpr tonemap_exposure = tonemap_exposure_t{};
inline auto constexpr backface_culling = backface_culling_t{};
inline auto constexpr preserve_camera = preserve_camera_t{};
inline auto constexpr reuse_camera = reuse_camera_t{};

// passthrough configures for scene modification commands
void configure(Renderable&, no_grid_t b);
void configure(Renderable&, no_shadow_t);
void configure(Renderable&, no_left_mouse_control_t b);
void configure(Renderable&, no_right_mouse_control_t b);
void configure(Renderable&, print_mode_t b);
void configure(Renderable&, maybe_empty_t);
void configure(Renderable&, no_outline_t b);
void configure(Renderable&, infinite_accumulation_t b);
void configure(Renderable&, controls_2d_t b);
void configure(Renderable&, dark_ui_t b);
void configure(Renderable&, subview_margin_t b);
void configure(Renderable&, clear_accumulation_t b);
void configure(Renderable&, tonemap_exposure_t const& b);
void configure(Renderable&, background_color const& b);
void configure(Renderable&, ssao_power const& b);
void configure(Renderable&, ssao_radius const& b);
void configure(Renderable&, grid_size const& v);
void configure(Renderable&, grid_center const& v);
void configure(Renderable&, camera_orientation const& b);
void configure(Renderable&, orthogonal_projection const& p);
void configure(Renderable&, camera_transform const& b);
void configure(Renderable&, headless_screenshot const& b);
void configure(Renderable&, close_keys const& k);
void configure(Renderable&, preserve_camera_t b);
void configure(Renderable&, reuse_camera_t b);

/// custom scene configuration
void configure(Renderable&, std::function<void(SceneConfig&)> f);

/// give the view its own camera
void configure(Renderable&, SharedCameraController c);

/// overrides the transform of the given renderable
void configure(Renderable& r, tg::mat4 const& transform);
template <class ScalarT>
void configure(Renderable& r, tg::mat<4, 4, ScalarT> const& transform)
{
    configure(r, tg::mat4(transform));
}
#if GLOW_HAS_GLM
void configure(Renderable& r, glm::mat4 const& transform);
#endif

/// custom bounding box
void configure(Renderable&, tg::aabb3 const& bounds);

/// names the renderable
void configure(Renderable& r, char const* name);
void configure(Renderable& r, std::string_view name);
void configure(Renderable& r, std::string const& name);

/// add renderjob
void configure(Renderable&, SharedRenderable const& rj);

/// global color of the renderable
void configure(GeometricRenderable& r, glow::colors::color const& c);
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, Color const& c);

/// per-primitive color of the renderable
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, polymesh::vertex_attribute<Color> const& c);
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, polymesh::face_attribute<Color> const& c);
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, polymesh::edge_attribute<Color> const& c);
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, polymesh::halfedge_attribute<Color> const& c);
template <class Color, class = std::enable_if_t<detail::is_color3_like<Color> || detail::is_color4_like<Color>>>
void configure(GeometricRenderable& r, std::vector<Color> const& c);

/// mapping from data to color
void configure(GeometricRenderable& r, ColorMapping const& cm);

/// textured mesh
void configure(GeometricRenderable& r, Texturing const& t);

/// visibility mask
void configure(GeometricRenderable& r, Masking const& m);

/// render modes
void configure(GeometricRenderable& r, transparent_t);
void configure(GeometricRenderable& r, opaque_t);
void configure(GeometricRenderable& r, no_fresnel_t);

/// culling
void configure(GeometricRenderable& r, backface_culling_t b);

/// clipping
void configure(GeometricRenderable& r, clip_plane const& p);

/// shading
void configure(GeometricRenderable& r, no_shading_t s);
void configure(GeometricRenderable& r, EnvMap const& em);

// ================== Implementation ==================
template <class Color, class>
void configure(GeometricRenderable& r, Color const& c)
{
    if constexpr (detail::is_color4_like<Color>)
    {
        configure(r, glow::colors::color(c.r, c.g, c.b, c.a));
        if (c.a < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);
    }
    else
        configure(r, glow::colors::color(c.r, c.g, c.b));
}

template <class Color, class>
void configure(GeometricRenderable& r, polymesh::vertex_attribute<Color> const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));

    if constexpr (detail::is_color4_like<Color>)
        if (!c.empty() && c.min([](auto&& v) { return v.a; }) < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);

    r.clearHash();
}

template <class Color, class>
void configure(GeometricRenderable& r, polymesh::face_attribute<Color> const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));

    if constexpr (detail::is_color4_like<Color>)
        if (!c.empty() && c.min([](auto&& v) { return v.a; }) < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);

    r.clearHash();
}

template <class Color, class>
void configure(GeometricRenderable& r, polymesh::halfedge_attribute<Color> const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));

    if constexpr (detail::is_color4_like<Color>)
        if (!c.empty() && c.min([](auto&& v) { return v.a; }) < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);

    r.clearHash();
}

template <class Color, class>
void configure(GeometricRenderable& r, polymesh::edge_attribute<Color> const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));

    if constexpr (detail::is_color4_like<Color>)
        if (!c.empty() && c.min([](auto&& v) { return v.a; }) < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);

    r.clearHash();
}

template <class Color, class>
void configure(GeometricRenderable& r, std::vector<Color> const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));

    if constexpr (detail::is_color4_like<Color>)
        if (!c.empty() && tg::min_element(c, [](auto&& v) { return v.a; }) < 1)
            r.setRenderMode(GeometricRenderable::RenderMode::Transparent);

    r.clearHash();
}

// feature test
template <class RenderableT, class ConfigT>
auto detail_can_be_configured(RenderableT& r, ConfigT&& cfg) -> decltype(configure(r, cfg), std::true_type{});
template <class RenderableT, class ConfigT>
auto detail_can_be_configured(...) -> std::false_type;
template <class RenderableT, class ConfigT>
static constexpr bool can_be_configured
    = decltype(glow::viewer::detail_can_be_configured<RenderableT, ConfigT>(std::declval<RenderableT&>(), std::declval<ConfigT>()))::value;

// perform configure
template <class RenderableT, class ConfigT>
void execute_configure(RenderableT& r, ConfigT&& cfg)
{
    static_assert(can_be_configured<RenderableT, ConfigT>, "no configure(...) found for this Renderable + Config. see \"configure.hh\" for available "
                                                           "options");
    configure(r, cfg);
}
}
