#include "canvas.hh"

#include <string>

#include <glow-extras/colors/color.hh>

// TODO: more performant from_hex

void glow::viewer::canvas_data::set_color(std::string_view hex)
{
    auto c = glow::colors::color::from_hex(std::string(hex));
    set_color(c.r, c.g, c.b, c.a);
}

glow::viewer::material::material(std::string_view color_str) : type(material_type::diffuse)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    color = {c.r, c.g, c.b, c.a};
}

glow::viewer::material::material(const char* color_str) : type(material_type::diffuse)
{
    auto c = glow::colors::color::from_hex(color_str);
    color = {c.r, c.g, c.b, c.a};
}

glow::viewer::canvas_t::point_ref& glow::viewer::canvas_t::point_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::splat_ref& glow::viewer::canvas_t::splat_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::line_ref& glow::viewer::canvas_t::line_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::triangle_ref& glow::viewer::canvas_t::triangle_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

void glow::viewer::canvas_data::add_label(const glow::viewer::label& label) { _labels.push_back(label); }

void glow::viewer::canvas_data::add_labels(glow::array_view<const glow::viewer::label> labels)
{
    for (auto l : labels)
        _labels.push_back(l);
}

void glow::viewer::canvas_data::add_arrow(tg::pos3 from, tg::pos3 to, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    auto mat = material(color);

    auto const length = distance(to, from);
    auto const dir = normalize_safe(to - from);
    tg::pos3 center;

    // compute 3 pos
    {
        auto const length_arrow = world_size * style.length_factor;
        auto const length_shaft = world_size * style.shaft_min_length_factor;
        auto const margin_arrow = world_size * style.margin_arrow_factor;
        auto const margin_shaft = world_size * style.margin_shaft_factor;

        if (length_arrow + length_shaft + margin_arrow + margin_shaft <= length) // enough space
        {
            to -= dir * margin_arrow;
            center = to - dir * length_arrow;
            from += dir * margin_shaft;
        }
        else // not enough space: start from to and go backwards
        {
            to -= dir * margin_arrow;
            center = to - dir * length_arrow;
            from = center - dir * length_shaft;
        }
    }

    auto const TA = tg::any_normal(dir);
    auto const TB = normalize_safe(cross(TA, dir));

    auto const r_shaft = world_size / 2;
    auto const r_arrow = world_size * style.radius_factor / 2;

    for (auto i = 0; i < style.segments; ++i)
    {
        auto a0 = 360_deg * i / style.segments;
        auto a1 = 360_deg * (i == style.segments - 1 ? 0 : i + 1) / style.segments;

        auto [s0, c0] = tg::sin_cos(a0);
        auto [s1, c1] = tg::sin_cos(a1);

        auto n0 = TA * s0 + TB * c0;
        auto n1 = TA * s1 + TB * c1;

        auto const from_shaft_0 = from + n0 * r_shaft;
        auto const from_shaft_1 = from + n1 * r_shaft;

        auto const center_shaft_0 = center + n0 * r_shaft;
        auto const center_shaft_1 = center + n1 * r_shaft;

        auto const center_arrow_0 = center + n0 * r_arrow;
        auto const center_arrow_1 = center + n1 * r_arrow;

        // shaft end
        _add_triangle(from, from_shaft_0, from_shaft_1, -dir, -dir, -dir, mat);

        // shaft mantle
        _add_triangle(from_shaft_0, center_shaft_0, center_shaft_1, n0, n0, n1, mat);
        _add_triangle(from_shaft_0, center_shaft_1, from_shaft_1, n0, n1, n1, mat);

        // arrow end
        _add_triangle(center, center_arrow_0, center_arrow_1, -dir, -dir, -dir, mat);

        // arrow mantle
        // TODO: normal?
        _add_triangle(center_arrow_0, to, center_arrow_1, n0, n0, n1, mat);
    }
}

void glow::viewer::canvas_data::add_arrow(tg::pos3 from_pos, tg::vec3 extent, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    add_arrow(from_pos, from_pos + extent, world_size, color, style);
}

void glow::viewer::canvas_data::add_arrow(tg::vec3 extent, tg::pos3 to_pos, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    add_arrow(to_pos - extent, to_pos, world_size, color, style);
}

std::vector<glow::viewer::SharedRenderable> glow::viewer::canvas_data::create_renderables() const
{
    std::vector<glow::viewer::SharedRenderable> r;

    pm::Mesh m;
    auto pos = m.vertices().make_attribute<tg::pos3>();
    auto normals = m.vertices().make_attribute<tg::vec3>();
    auto size = m.vertices().make_attribute<float>();
    auto color3 = m.vertices().make_attribute<tg::color3>();
    auto color4 = m.vertices().make_attribute<tg::color4>();
    auto dash_size = m.edges().make_attribute<float>();

    //
    // points
    //
    if (!_points_px.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_points_px.size());
        for (auto const& p : _points_px)
        {
            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
        }

        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::points(pos).point_size_px(size), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _points_px)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
            }

            r.push_back(gv::make_and_configure_renderable(gv::points(pos).point_size_px(size), color4, gv::transparent));
        }
    }
    if (!_points_world.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_points_world.size());
        for (auto const& p : _points_world)
        {
            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
        }

        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::points(pos).point_size_world(size), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _points_world)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
            }

            r.push_back(gv::make_and_configure_renderable(gv::points(pos).point_size_world(size), color4, gv::transparent));
        }
    }

    //
    // splats
    //
    if (!_splats.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_splats.size());
        for (auto const& p : _splats)
        {
            TG_ASSERT(p.size >= 0 && "no splat size set (forgot to call .set_splat_size() or .add_splats(...).size(...)?)");

            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
            normals[v] = p.normal;
        }

        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::points(pos).round().normals(normals).point_size_world(size), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _splats)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
                normals[v] = p.normal;
            }

            r.push_back(gv::make_and_configure_renderable(gv::points(pos).round().normals(normals).point_size_world(size), color4, gv::transparent));
        }
    }

    //
    // lines
    //
    if (!_lines_px.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_lines_px.size());
        for (auto const& l : _lines_px)
        {
            if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                continue;

            if (l.p0.color.a < 1 || l.p1.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v0 = m.vertices().add();
            auto v1 = m.vertices().add();
            pos[v0] = l.p0.pos;
            pos[v1] = l.p1.pos;
            color3[v0] = tg::color3(l.p0.color);
            color3[v1] = tg::color3(l.p1.color);
            size[v0] = l.p0.size;
            size[v1] = l.p1.size;
            auto e = m.edges().add_or_get(v0, v1);
            dash_size[e] = l.dash_size;
        }

        // TODO: screen space dash size
        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::lines(pos).line_width_px(size), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& l : _lines_px)
            {
                if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                    continue;

                if (l.p0.color.a >= 1 && l.p1.color.a >= 1)
                    continue;

                auto v0 = m.vertices().add();
                auto v1 = m.vertices().add();
                pos[v0] = l.p0.pos;
                pos[v1] = l.p1.pos;
                color4[v0] = tg::color4(l.p0.color);
                color4[v1] = tg::color4(l.p1.color);
                size[v0] = l.p0.size;
                size[v1] = l.p1.size;
                auto e = m.edges().add_or_get(v0, v1);
                dash_size[e] = l.dash_size;
            }

            // TODO: screen space dash size
            r.push_back(gv::make_and_configure_renderable(gv::lines(pos).line_width_px(size), color4, gv::transparent));
        }
    }
    if (!_lines_world.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_lines_world.size());
        for (auto const& l : _lines_world)
        {
            if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                continue;

            if (l.p0.color.a < 1 || l.p1.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v0 = m.vertices().add();
            auto v1 = m.vertices().add();
            pos[v0] = l.p0.pos;
            pos[v1] = l.p1.pos;
            color3[v0] = tg::color3(l.p0.color);
            color3[v1] = tg::color3(l.p1.color);
            size[v0] = l.p0.size;
            size[v1] = l.p1.size;
            auto e = m.edges().add_or_get(v0, v1);
            dash_size[e] = l.dash_size;
        }

        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::lines(pos).line_width_world(size).dash_size_world(dash_size), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& l : _lines_world)
            {
                if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                    continue;

                if (l.p0.color.a >= 1 && l.p1.color.a >= 1)
                    continue;

                auto v0 = m.vertices().add();
                auto v1 = m.vertices().add();
                pos[v0] = l.p0.pos;
                pos[v1] = l.p1.pos;
                color4[v0] = tg::color4(l.p0.color);
                color4[v1] = tg::color4(l.p1.color);
                size[v0] = l.p0.size;
                size[v1] = l.p1.size;
                auto e = m.edges().add_or_get(v0, v1);
                dash_size[e] = l.dash_size;
            }

            r.push_back(gv::make_and_configure_renderable(gv::lines(pos).line_width_world(size).dash_size_world(dash_size), color4, gv::transparent));
        }
    }

    //
    // faces
    //
    if (!_triangles.empty())
    {
        auto has_transparent = false;
        m.clear();
        m.vertices().reserve(_triangles.size() * 3);
        for (auto const& t : _triangles)
        {
            if (t.color[0].a <= 0 && t.color[1].a <= 0 && t.color[2].a <= 0)
                continue;

            if (t.color[0].a < 1 || t.color[1].a < 1 || t.color[2].a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v0 = m.vertices().add();
            auto v1 = m.vertices().add();
            auto v2 = m.vertices().add();
            pos[v0] = t.pos[0];
            pos[v1] = t.pos[1];
            pos[v2] = t.pos[2];
            normals[v0] = t.normal[0];
            normals[v1] = t.normal[1];
            normals[v2] = t.normal[2];
            color3[v0] = tg::color3(t.color[0]);
            color3[v1] = tg::color3(t.color[1]);
            color3[v2] = tg::color3(t.color[2]);
            m.faces().add(v0, v1, v2);
        }

        if (!m.vertices().empty())
            r.push_back(gv::make_and_configure_renderable(gv::polygons(pos).normals(normals), color3));

        if (has_transparent)
        {
            m.clear();
            for (auto const& t : _triangles)
            {
                if (t.color[0].a <= 0 && t.color[1].a <= 0 && t.color[2].a <= 0)
                    continue;

                if (t.color[0].a >= 1 && t.color[1].a >= 1 && t.color[2].a >= 1)
                    continue;

                auto v0 = m.vertices().add();
                auto v1 = m.vertices().add();
                auto v2 = m.vertices().add();
                pos[v0] = t.pos[0];
                pos[v1] = t.pos[1];
                pos[v2] = t.pos[2];
                normals[v0] = t.normal[0];
                normals[v1] = t.normal[1];
                normals[v2] = t.normal[2];
                color4[v0] = t.color[0];
                color4[v1] = t.color[1];
                color4[v2] = t.color[2];
                m.faces().add(v0, v1, v2);
            }

            r.push_back(gv::make_and_configure_renderable(gv::polygons(pos).normals(normals), color4, gv::transparent));
        }
    }

    //
    // labels
    //
    if (!_labels.empty())
        r.push_back(gv::make_and_configure_renderable(_labels));

    return r;
}

glow::viewer::canvas_t::~canvas_t()
{
    auto v = gv::view();
    for (auto const& r : create_renderables())
        gv::view(r);
}
