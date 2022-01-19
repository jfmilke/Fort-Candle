#pragma once

#include <glow-extras/viewer/objects/other/labels.hh>
#include <glow-extras/viewer/traits.hh>
#include <glow-extras/viewer/view.hh>

namespace glow::viewer
{
struct arrow_style
{
    /// start of arrow has a small margin that is world size * this factor
    float margin_arrow_factor = 0.f;

    /// end of arrow has a small margin that is world size * this factor
    float margin_shaft_factor = 0.f;

    /// arrow length is world size * this factor
    float length_factor = 2.5f;

    /// shaft length is at least world size * this factor
    float shaft_min_length_factor = 2.5f;

    /// arrow radius is world size * this factor
    float radius_factor = 2.5f;

    // TODO: shape
    int segments = 16;
};

enum class material_type : uint8_t
{
    default_, ///< default, e.g. white/grayish points
    diffuse,
};

// TODO: move somewhere more general?
struct material
{
    material_type type = material_type::default_;
    tg::color4 color;

    material() = default;
    material(tg::color3 const& c) : type(material_type::diffuse), color(c) {}
    material(tg::color4 const& c) : type(material_type::diffuse), color(c) {}
    material(float r, float g, float b, float a = 1.f) : type(material_type::diffuse), color(r, g, b, a) {}
    material(std::string_view color_str);
    material(char const* color_str);

    bool is_default() const { return type == material_type::default_; }
};

/// stores raw geometric data that can be displayed via gv::canvas()
/// (see canvas_t)
struct canvas_data
{
protected:
    struct point;
    struct splat;
    struct line;
    struct triangle;

    struct point_ref
    {
        /// sets the normal of all added points
        point_ref& color(tg::color4 const& c)
        {
            for (auto& p : _points)
                p.color = c;
            return *this;
        }
        point_ref& color(float r, float g, float b, float a = 1) { return color(tg::color4(r, g, b, a)); }
        point_ref& color(std::string_view color_str);

        /// NOTE: only works reliably if add_points(pos) with a corresponding pos vertex attribute
        template <class ColorT>
        point_ref& colors(pm::vertex_attribute<ColorT> const& colors)
        {
            static_assert(std::is_constructible_v<tg::color4, ColorT>, "tg::color4(color) must work");
            TG_ASSERT(colors.mesh().vertices().size() == int(_points.size()) && "mismatching size");
            auto idx = 0;
            for (auto v : colors.mesh().vertices())
                _points[idx++].color = tg::color4(colors[v]);
            return *this;
        }
        /// NOTE: count must match with the number of points added!
        template <class Colors, std::enable_if_t<tg::is_any_range<Colors>, int> = 0>
        point_ref& colors(Colors const& colors)
        {
            using element_t = std::decay_t<decltype(*std::begin(colors))>;
            static_assert(std::is_constructible_v<tg::color4, element_t>, "tg::color4(element) must work");
            auto idx = 0;
            for (auto&& c : colors)
            {
                TG_ASSERT(idx < int(_points.size()) && "too many colors provided");
                _points[idx++].color = tg::color4(c);
            }
            TG_ASSERT(idx == int(_points.size()) && "not enough colors provided");
            return *this;
        }

        /// sets the size of all added points
        /// NOTE: world size or px size is determined by canvas state and cannot be changed!
        point_ref& size(float s)
        {
            for (auto& p : _points)
                p.size = s;
            return *this;
        }

        /// scales all added points
        point_ref& scale_size(float s)
        {
            for (auto& sp : _points)
                sp.size *= s;
            return *this;
        }

        /// NOTE: only works reliably if add_points(pos) with a corresponding pos vertex attribute
        template <class ScalarT>
        point_ref& sizes(pm::vertex_attribute<ScalarT> const& sizes)
        {
            static_assert(std::is_constructible_v<float, ScalarT>, "float(size) must work");
            TG_ASSERT(sizes.mesh().vertices().size() == int(_points.size()) && "mismatching size");
            auto idx = 0;
            for (auto v : sizes.mesh().vertices())
                _points[idx++].size = float(sizes[v]);
            return *this;
        }
        /// NOTE: count must match with the number of points added!
        template <class Sizes, std::enable_if_t<tg::is_any_range<Sizes>, int> = 0>
        point_ref& sizes(Sizes const& sizes)
        {
            using element_t = std::decay_t<decltype(*std::begin(sizes))>;
            static_assert(std::is_constructible_v<float, element_t>, "float(element) must work");
            auto idx = 0;
            for (auto&& c : sizes)
            {
                TG_ASSERT(idx < int(_points.size()) && "too many sizes provided");
                _points[idx++].size = float(c);
            }
            TG_ASSERT(idx == int(_points.size()) && "not enough sizes provided");
            return *this;
        }

        /// moves all points by += offset
        template <class T>
        point_ref& translate(tg::vec<3, T> const& offset)
        {
            for (auto& p : _points)
                p.pos += tg::vec3(offset);
            return *this;
        }
        point_ref& translate(float x, float y, float z) { return translate(tg::vec3(x, y, z)); }

        /// replaces each pos by M * pos
        point_ref& transform(tg::mat4 const& M)
        {
            for (auto& p : _points)
                p.pos = M * p.pos;
            return *this;
        }

        /// adds a label to each point
        point_ref& label(std::string_view text, label_style const& style = {})
        {
            for (auto& p : _points)
                _canvas.add_label(p.pos, text, style);
            return *this;
        }

    public:
        point_ref(array_view<point> points, canvas_data& canvas) : _points(points), _canvas(canvas) {}

        point_ref(point_ref const&) = delete;
        point_ref(point_ref&&) = delete;
        point_ref& operator=(point_ref const&) = delete;
        point_ref& operator=(point_ref&&) = delete;

    private:
        array_view<point> _points;
        canvas_data& _canvas;
    };

    struct splat_ref
    {
        /// sets the normal of all added splats
        splat_ref& color(tg::color4 const& c)
        {
            for (auto& p : _splats)
                p.color = c;
            return *this;
        }
        splat_ref& color(float r, float g, float b, float a = 1) { return color(tg::color4(r, g, b, a)); }
        splat_ref& color(std::string_view color_str);

        /// NOTE: only works reliably if add_splats(pos) with a corresponding pos vertex attribute
        template <class ColorT>
        splat_ref& colors(pm::vertex_attribute<ColorT> const& colors)
        {
            static_assert(std::is_constructible_v<tg::color4, ColorT>, "tg::color4(color) must work");
            TG_ASSERT(colors.mesh().vertices().size() == int(_splats.size()) && "mismatching size");
            auto idx = 0;
            for (auto v : colors.mesh().vertices())
                _splats[idx++].color = tg::color4(colors[v]);
            return *this;
        }
        /// NOTE: count must match with the number of splats added!
        template <class Colors, std::enable_if_t<tg::is_any_range<Colors>, int> = 0>
        splat_ref& colors(Colors const& colors)
        {
            using element_t = std::decay_t<decltype(*std::begin(colors))>;
            static_assert(std::is_constructible_v<tg::color4, element_t>, "tg::color4(element) must work");
            auto idx = 0;
            for (auto&& c : colors)
            {
                TG_ASSERT(idx < int(_splats.size()) && "too many colors provided");
                _splats[idx++].color = tg::color4(c);
            }
            TG_ASSERT(idx == int(_splats.size()) && "not enough colors provided");
            return *this;
        }

        /// sets the size of all added splats
        splat_ref& size(float s)
        {
            for (auto& sp : _splats)
                sp.size = s;
            return *this;
        }

        /// scales all added splats
        splat_ref& scale_size(float s)
        {
            for (auto& sp : _splats)
                sp.size *= s;
            return *this;
        }

        /// NOTE: only works reliably if add_splats(pos) with a corresponding pos vertex attribute
        template <class ScalarT>
        splat_ref& sizes(pm::vertex_attribute<ScalarT> const& sizes)
        {
            static_assert(std::is_constructible_v<float, ScalarT>, "float(size) must work");
            TG_ASSERT(sizes.mesh().vertices().size() == int(_splats.size()) && "mismatching size");
            auto idx = 0;
            for (auto v : sizes.mesh().vertices())
                _splats[idx++].size = float(sizes[v]);
            return *this;
        }
        /// NOTE: count must match with the number of splats added!
        template <class Sizes, std::enable_if_t<tg::is_any_range<Sizes>, int> = 0>
        splat_ref& sizes(Sizes const& sizes)
        {
            using element_t = std::decay_t<decltype(*std::begin(sizes))>;
            static_assert(std::is_constructible_v<float, element_t>, "float(element) must work");
            auto idx = 0;
            for (auto&& c : sizes)
            {
                TG_ASSERT(idx < int(_splats.size()) && "too many sizes provided");
                _splats[idx++].size = float(c);
            }
            TG_ASSERT(idx == int(_splats.size()) && "not enough sizes provided");
            return *this;
        }

        /// moves all splats by += offset
        template <class T>
        splat_ref& translate(tg::vec<3, T> const& offset)
        {
            for (auto& p : _splats)
                p.pos += tg::vec3(offset);
            return *this;
        }
        splat_ref& translate(float x, float y, float z) { return translate(tg::vec3(x, y, z)); }

        /// moves all pos by += d * normal
        splat_ref& normal_translate(float d)
        {
            for (auto& p : _splats)
                p.pos += p.normal * d;
            return *this;
        }

        /// replaces each pos by M * pos (and also normal)
        splat_ref& transform(tg::mat4 const& M)
        {
            for (auto& p : _splats)
            {
                p.pos = M * p.pos;
                p.normal = M * p.normal;
            }
            return *this;
        }

    public:
        splat_ref(array_view<splat> splats) : _splats(splats) {}

        splat_ref(splat_ref const&) = delete;
        splat_ref(splat_ref&&) = delete;
        splat_ref& operator=(splat_ref const&) = delete;
        splat_ref& operator=(splat_ref&&) = delete;

    private:
        array_view<splat> _splats;
    };

    struct line_ref
    {
        /// sets the normal of all added lines
        line_ref& color(tg::color4 const& c)
        {
            for (auto& l : _lines)
            {
                l.p0.color = c;
                l.p1.color = c;
            }
            return *this;
        }
        line_ref& color(float r, float g, float b, float a = 1) { return color(tg::color4(r, g, b, a)); }
        line_ref& color(std::string_view color_str);

        /// sets the size of all added lines
        /// NOTE: world size or px size is determined by canvas state and cannot be changed!
        line_ref& size(float s)
        {
            TG_ASSERT(s >= 0);
            for (auto& l : _lines)
            {
                l.p0.size = s;
                l.p1.size = s;
            }
            return *this;
        }

        /// scales all added lines
        line_ref& scale_size(float s)
        {
            for (auto& l : _lines)
            {
                l.p0.size *= s;
                l.p1.size *= s;
            }
            return *this;
        }

        /// sets a dash size for all lines
        /// NOTE: only works with world size currently
        line_ref& dash_size_world(float s)
        {
            TG_ASSERT(s >= 0);
            for (auto& l : _lines)
                l.dash_size = s;
            return *this;
        }

        /// moves all lines by += offset
        template <class T>
        line_ref& translate(tg::vec<3, T> const& offset)
        {
            for (auto& p : _lines)
            {
                p.p0.pos += tg::vec3(offset);
                p.p1.pos += tg::vec3(offset);
            }
            return *this;
        }
        line_ref& translate(float x, float y, float z) { return translate(tg::vec3(x, y, z)); }

        /// replaces each pos by M * pos
        line_ref& transform(tg::mat4 const& M)
        {
            for (auto& p : _lines)
            {
                p.p0.pos = M * p.p0.pos;
                p.p1.pos = M * p.p1.pos;
            }
            return *this;
        }

    public:
        line_ref(array_view<line> lines) : _lines(lines) {}

        line_ref(line_ref const&) = delete;
        line_ref(line_ref&&) = delete;
        line_ref& operator=(line_ref const&) = delete;
        line_ref& operator=(line_ref&&) = delete;

    private:
        array_view<line> _lines;
    };

    struct triangle_ref
    {
        /// sets the color of all added triangles
        triangle_ref& color(tg::color4 const& c)
        {
            for (auto& l : _triangles)
            {
                l.color[0] = c;
                l.color[1] = c;
                l.color[2] = c;
            }
            return *this;
        }
        triangle_ref& color(float r, float g, float b, float a = 1) { return color(tg::color4(r, g, b, a)); }
        triangle_ref& color(std::string_view color_str);

        /// sets the triangle normal of all triangles
        triangle_ref& normal(tg::vec3 const& n)
        {
            for (auto& t : _triangles)
            {
                t.normal[0] = n;
                t.normal[1] = n;
                t.normal[2] = n;
            }
            return *this;
        }
        triangle_ref& normal(float x, float y, float z) { return normal(tg::vec3(x, y, z)); }

        /// sets the three vertex normals (for all triangles, probably only useful for add_face(tri))
        triangle_ref& normals(tg::vec3 const& n0, tg::vec3 const& n1, tg::vec3 const& n2)
        {
            for (auto& t : _triangles)
            {
                t.normal[0] = n0;
                t.normal[1] = n1;
                t.normal[2] = n2;
            }
            return *this;
        }

        /// moves all triangles by += offset
        template <class T>
        triangle_ref& translate(tg::vec<3, T> const& offset)
        {
            for (auto& t : _triangles)
            {
                t.pos[0] += tg::vec3(offset);
                t.pos[1] += tg::vec3(offset);
                t.pos[2] += tg::vec3(offset);
            }
            return *this;
        }
        triangle_ref& translate(float x, float y, float z) { return translate(tg::vec3(x, y, z)); }

        /// moves all pos by += d * normal
        triangle_ref& normal_translate(float d)
        {
            for (auto& t : _triangles)
            {
                t.pos[0] += t.normal[0] * d;
                t.pos[1] += t.normal[1] * d;
                t.pos[2] += t.normal[2] * d;
            }
            return *this;
        }

        /// replaces each pos by M * pos (and also normals)
        triangle_ref& transform(tg::mat4 const& M)
        {
            for (auto& t : _triangles)
            {
                t.pos[0] = M * t.pos[0];
                t.pos[1] = M * t.pos[1];
                t.pos[2] = M * t.pos[2];

                t.normal[0] = M * t.normal[0];
                t.normal[1] = M * t.normal[1];
                t.normal[2] = M * t.normal[2];
            }
            return *this;
        }

    public:
        triangle_ref(array_view<triangle> triangles) : _triangles(triangles) {}

        triangle_ref(triangle_ref const&) = delete;
        triangle_ref(triangle_ref&&) = delete;
        triangle_ref& operator=(triangle_ref const&) = delete;
        triangle_ref& operator=(triangle_ref&&) = delete;

    private:
        array_view<triangle> _triangles;
    };

    // config
public:
    void set_color(tg::color3 const& c, float a = 1) { _mat = tg::color4(c, a); }
    void set_color(tg::color4 const& c) { _mat = c; }
    void set_color(float r, float g, float b, float a = 1) { _mat = tg::color4(r, g, b, a); }
    void set_color(std::string_view hex);
    void clear_color() { _mat = {}; }
    void set_material(material const& m) { _mat = m; }

    void set_point_size_px(float s = 7.f)
    {
        TG_ASSERT(s > 0);
        _points_size = s;
        _points_curr = &_points_px;
    }
    void set_point_size_world(float s)
    {
        TG_ASSERT(s > 0);
        _points_size = s;
        _points_curr = &_points_world;
    }

    void set_line_width_px(float w = 5.f)
    {
        TG_ASSERT(w > 0);
        _lines_width = w;
        _lines_curr = &_lines_px;
    }
    void set_line_width_world(float w)
    {
        TG_ASSERT(w > 0);
        _lines_width = w;
        _lines_curr = &_lines_world;
    }

    void set_splat_size(float s)
    {
        TG_ASSERT(s > 0);
        _splats_size = s;
    }

    /// number of segments used to approximate round geometry
    void set_resolution(int res)
    {
        TG_ASSERT(res >= 4);
        _resolution = res;
    }

    // face
public:
    template <int D, class T>
    triangle_ref add_face(tg::pos<D, T> const& p0, tg::pos<D, T> const& p1, tg::pos<D, T> const& p2, material const& mat = {})
    {
        return add_face(tg::triangle<D, T>(p0, p1, p2), mat);
    }
    template <int D, class T>
    triangle_ref add_face(tg::pos<D, T> const& p0, tg::pos<D, T> const& p1, tg::pos<D, T> const& p2, tg::pos<D, T> const& p3, material const& mat = {})
    {
        return add_face(tg::quad<D, T>(p0, p1, p2, p3), mat);
    }
    template <int D, class T>
    triangle_ref add_face(tg::triangle<D, T> const& t, material const& mat = {})
    {
        static_assert(D == 3, "only supports 3D currently");
        auto const n = normalize_safe(cross(tg::vec3(t.pos1 - t.pos0), tg::vec3(t.pos2 - t.pos0)));
        return _add_triangle(tg::pos3(t.pos0), tg::pos3(t.pos1), tg::pos3(t.pos2), n, n, n, mat);
    }
    template <class T>
    triangle_ref add_face(tg::box<2, T, 3> const& t, material const& mat = {})
    {
        auto start_cnt = _triangles.size();
        auto n = tg::normal_of(tg::box2in3(t));
        auto v00 = tg::pos3(t[{-1, -1}]);
        auto v01 = tg::pos3(t[{-1, +1}]);
        auto v10 = tg::pos3(t[{+1, -1}]);
        auto v11 = tg::pos3(t[{+1, +1}]);
        _add_triangle(v00, v10, v11, n, n, n, mat);
        _add_triangle(v00, v11, v01, n, n, n, mat);
        return array_view<triangle>(_triangles.data() + start_cnt, _triangles.size() - start_cnt);
    }
    template <int D, class T>
    triangle_ref add_face(tg::quad<D, T> const& t, material const& mat = {})
    {
        auto start_cnt = _triangles.size();
        static_assert(D == 3, "only supports 3D currently");
        auto vc = tg::centroid_of(tg::quad3(t));
        auto v00 = tg::pos3(t.pos00);
        auto v01 = tg::pos3(t.pos01);
        auto v10 = tg::pos3(t.pos10);
        auto v11 = tg::pos3(t.pos11);
        auto n00 = normalize_safe(cross(v01 - v00, v10 - v00));
        auto n01 = normalize_safe(cross(v11 - v01, v00 - v01));
        auto n10 = normalize_safe(cross(v00 - v10, v11 - v10));
        auto n11 = normalize_safe(cross(v10 - v11, v01 - v11));
        auto nc = normalize_safe(n00 + n01 + n10 + n11);
        _add_triangle(v00, v10, vc, n00, n10, nc, mat);
        _add_triangle(v10, v11, vc, n10, n11, nc, mat);
        _add_triangle(v11, v01, vc, n11, n01, nc, mat);
        _add_triangle(v01, v00, vc, n01, n00, nc, mat);
        return array_view<triangle>(_triangles.data() + start_cnt, _triangles.size() - start_cnt);
    }
    template <class PosT>
    triangle_ref add_face(pm::face_handle face, pm::vertex_attribute<PosT> const& pos, material const& mat = {})
    {
        auto start_cnt = _triangles.size();
        // compute normal
        // start with the centroid
        decltype(tg::pos3() + tg::pos3()) centroid;
        {
            auto area = 0.0f;
            auto h = face.any_halfedge();
            auto const v0 = h.vertex_from();
            auto const p0 = tg::pos3(v0[pos]);

            auto p_prev = tg::pos3(h.vertex_to()[pos]);
            h = h.next();
            do
            {
                auto p_curr = tg::pos3(h.vertex_to()[pos]);
                auto a = length(cross(p_prev - p0, p_curr - p0));
                area += a;
                centroid += (p_prev + p_curr + p0) * a;

                // circulate
                h = h.next();
                p_prev = p_curr;
            } while (h.vertex_to() != v0);
            centroid /= 3.0f * area;
        }

        tg::vec3 normal;
        {
            auto e = face.any_halfedge();
            auto const v0 = tg::pos3(e.vertex_from()[pos]);
            auto const v1 = tg::pos3(e.vertex_to()[pos]);
            normal = cross(v0 - centroid, v1 - centroid);
            auto l = length(normal);
            normal = l == 0 ? tg::vec3::zero : normal / l;
        }
        auto h0 = face.any_halfedge();
        auto he = h0.prev();
        auto h = h0.next();
        auto v0 = tg::pos3(pos[h0.vertex_from()]);
        while (h != he)
        {
            auto v1 = tg::pos3(pos[h.vertex_from()]);
            auto v2 = tg::pos3(pos[h.vertex_to()]);
            _add_triangle(v0, v1, v2, normal, normal, normal, mat);
            h = h.next();
        }
        return array_view<triangle>(_triangles.data() + start_cnt, _triangles.size() - start_cnt);
    }

    // faces
    // TODO: pyramids
    // TODO: per vertex normals
public:
    template <int D, class T>
    triangle_ref add_faces(tg::triangle<D, T> const& t, material const& mat = {})
    {
        return add_face(t, mat);
    }
    template <class T>
    triangle_ref add_faces(tg::box<2, T, 3> const& b, material const& mat = {})
    {
        return add_face(b, mat);
    }
    template <int D, class T>
    triangle_ref add_faces(tg::quad<D, T> const& q, material const& mat = {})
    {
        return add_face(q, mat);
    }
    template <class T>
    triangle_ref add_faces(T const& value, material const& mat = {})
    {
        auto start_cnt = _triangles.size();
        if constexpr (tg::has_faces_of<T const&>)
            for (auto const& f : faces_of(value))
                add_faces(f, mat);
        else if constexpr (gv::detail::is_vertex_attr<T>)
            for (auto f : value.mesh().faces())
                add_face(f, value, mat);
        else if constexpr (tg::is_any_range<T const&>)
            for (auto const& e : value)
                add_faces(e, mat);
        else
            static_assert(tg::always_false<T>, "type not supported");
        return array_view<triangle>(_triangles.data() + start_cnt, _triangles.size() - start_cnt);
    }

    // line
public:
    template <int D, class T>
    line_ref add_line(tg::segment<D, T> const& s, material const& mat = {})
    {
        static_assert(D == 3, "only supports 3D currently");
        return _add_line(tg::pos3(s.pos0), tg::pos3(s.pos1), mat);
    }
    template <int D, class T>
    line_ref add_line(tg::pos<D, T> const& p0, tg::pos<D, T> const& p1, material const& mat = {})
    {
        return add_lines(tg::segment(p0, p1), mat);
    }
    template <int D, class T>
    line_ref add_line(tg::pos<D, T> const& p, tg::vec<D, T> const& dir, material const& mat = {})
    {
        return add_lines(tg::segment(p, p + dir), mat);
    }

    // lines
public:
    template <int D, class T>
    line_ref add_lines(tg::segment<D, T> const& s, material const& mat = {})
    {
        static_assert(D == 3, "only supports 3D currently");
        return _add_line(tg::pos3(s.pos0), tg::pos3(s.pos1), mat);
    }
    template <class T>
    line_ref add_lines(tg::sphere<2, T, 3> const& s, material const& mat = {})
    {
        auto start_cnt = _lines_curr->size();
        auto d0 = tg::any_normal(s.normal) * s.radius;
        auto d1 = cross(d0, s.normal);
        auto prev_p = s.center + d1;
        for (auto i = 1; i <= _resolution; ++i)
        {
            auto [si, co] = tg::sin_cos(360_deg * i / _resolution);
            auto curr_p = s.center + d0 * si + d1 * co;
            _add_line(prev_p, curr_p, mat);
            prev_p = curr_p;
        }
        return array_view<line>(_lines_curr->data() + start_cnt, _lines_curr->size() - start_cnt);
    }
    template <class T>
    line_ref add_lines(T const& value, material const& mat = {})
    {
        auto start_cnt = _lines_curr->size();
        if constexpr (tg::has_edges_of<T const&>)
            for (auto const& e : edges_of(value))
                add_lines(e, mat);
        else if constexpr (gv::detail::is_vertex_attr<T>)
            for (auto e : value.mesh().edges())
                _add_line(tg::pos3(value[e.vertexA()]), tg::pos3(value[e.vertexB()]), mat);
        else if constexpr (tg::is_any_range<T const&>)
            for (auto const& e : value)
                add_lines(e, mat);
        else
            static_assert(tg::always_false<T>, "type not supported");
        return array_view<line>(_lines_curr->data() + start_cnt, _lines_curr->size() - start_cnt);
    }

    // point
public:
    template <class PosT>
    point_ref add_point(PosT const& pos, material const& mat = {})
    {
        static_assert(tg::is_comp_like<PosT, 3, float>, "pos must be a pos3 like type");
        return _add_point(tg::pos3(pos), mat);
    }
    point_ref add_point(float x, float y, float z, material const& mat = {}) { return add_point(tg::pos3(x, y, z), mat); }

    // points
public:
    template <class T>
    point_ref add_points(T const& value, material const& mat = {})
    {
        auto start_cnt = _points_curr->size();
        if constexpr (tg::is_comp_like<T, 3, float>)
            _add_point(tg::pos3(value), mat);
        else if constexpr (tg::has_vertices_of<T const&>)
            for (auto const& v : vertices_of(value))
                _add_point(tg::pos3(v), mat);
        else if constexpr (gv::detail::is_vertex_attr<T>)
            for (auto v : value.mesh().vertices())
                add_points(value[v], mat);
        else if constexpr (tg::is_any_range<T const&>)
            for (auto const& v : value)
                add_points(v, mat);
        else
            static_assert(tg::always_false<T>, "type not supported");
        return {array_view<point>(_points_curr->data() + start_cnt, _points_curr->size() - start_cnt), *this};
    }

    // splat
public:
    template <class PosT, class NormalT>
    splat_ref add_splat(PosT const& pos, NormalT const& normal, material const& mat = {})
    {
        static_assert(tg::is_comp_like<PosT, 3, float>, "value must be a pos3 like type");
        static_assert(tg::is_comp_like<NormalT, 3, float>, "value must be a pos3 like type");
        return _add_splat(tg::pos3(pos), tg::vec3(normal), mat);
    }

    // splats
public:
    template <class PosT, class NormalT>
    splat_ref add_splats(PosT const& pos, NormalT const& normal, material const& mat = {})
    {
        auto start_cnt = _splats.size();
        if constexpr (tg::is_comp_like<PosT, 3, float>)
            _add_splat(tg::pos3(pos), tg::vec3(normal), mat);
        else if constexpr (gv::detail::is_vertex_attr<PosT>)
            for (auto v : pos.mesh().vertices())
            {
                if constexpr (gv::detail::is_vertex_attr<NormalT>)
                    add_splats(pos[v], normal[v], mat);
                else
                    add_splats(pos[v], tg::vec3(normal), mat);
            }
        else if constexpr (tg::is_any_range<PosT const&>)
        {
            if constexpr (tg::is_any_range<NormalT const&>)
            {
                auto p_it = std::begin(pos);
                auto n_it = std::begin(normal);
                auto p_end = std::end(pos);
                auto n_end = std::end(normal);

                while (p_it != p_end && n_it != n_end)
                {
                    add_splats(pos, normal, mat);

                    ++p_it;
                    ++n_end;
                }

                TG_ASSERT(!(p_it != p_end) && !(n_it != n_end) && "range size mismatch");
            }
            else
                for (auto const& p : pos)
                    add_splats(p, tg::vec3(normal), mat);
        }
        else
            static_assert(tg::always_false<PosT>, "type not supported");
        return array_view<splat>(_splats.data() + start_cnt, _splats.size() - start_cnt);
    }

    // labels
public:
    void add_label(label const& label);
    template <class T>
    label& add_label(tg::pos<3, T> pos, std::string_view text, label_style const& style = {})
    {
        auto& l = _labels.emplace_back();
        l.text = text;
        l.pos = tg::pos3(pos);
        l.style = style;
        return l;
    }
    template <class Pos3>
    label& add_label(pm::vertex_attribute<Pos3> const& pos, pm::vertex_index v, std::string_view text, label_style const& style = {})
    {
        return add_label(pos[v], text, style);
    }
    template <class Pos3, class Normal3>
    label& add_label(pm::vertex_attribute<Pos3> const& pos, pm::vertex_attribute<Normal3> const& normal, pm::vertex_index v, std::string_view text, label_style const& style = {})
    {
        auto& l = add_label(pos[v], text, style);
        l.normal = tg::vec3(normal[v]);
        return l;
    }

    void add_labels(glow::array_view<label const> labels);

    // extras
public:
    void add_arrow(tg::pos3 from, tg::pos3 to, float world_size, tg::color3 color, arrow_style const& style = {});
    void add_arrow(tg::pos3 from_pos, tg::vec3 extent, float world_size, tg::color3 color, arrow_style const& style = {});
    void add_arrow(tg::vec3 extent, tg::pos3 to_pos, float world_size, tg::color3 color, arrow_style const& style = {});

    void add_file(std::string_view filename, tg::mat4 const& transform = tg::mat4::identity);

    void add_data(canvas_data const& data)
    {
        auto add_range = [](auto& dest, auto const& src) {
            for (auto&& d : src)
                dest.push_back(d);
        };

        add_range(_points_px, data._points_px);
        add_range(_points_world, data._points_world);

        add_range(_lines_px, data._lines_px);
        add_range(_lines_world, data._lines_world);

        add_range(_splats, data._splats);

        add_range(_triangles, data._triangles);

        add_range(_labels, data._labels);
    }

    std::vector<SharedRenderable> create_renderables() const;

    // state
private:
    material _mat;
    int _resolution = 64;

    // data
    // NOTE: if adding something here, it might need to be added to add_data as well
protected:
    struct point
    {
        tg::pos3 pos;
        tg::color4 color;
        float size = 0;
    };
    struct splat
    {
        tg::pos3 pos;
        tg::color4 color;
        tg::vec3 normal;
        float size = 0;
    };
    struct line
    {
        point p0;
        point p1;
        float dash_size = 0.f;
    };
    struct triangle
    {
        tg::pos3 pos[3];
        tg::vec3 normal[3];
        tg::color4 color[3];
    };

    float _points_size = 7.f;
    std::vector<point> _points_px;
    std::vector<point> _points_world;
    std::vector<point>* _points_curr = &_points_px;

    float _splats_size = -1;
    std::vector<splat> _splats;

    float _lines_width = 5.f;
    std::vector<line> _lines_px;
    std::vector<line> _lines_world;
    std::vector<line>* _lines_curr = &_lines_px;

    std::vector<triangle> _triangles;

    std::vector<label> _labels;

    point_ref _add_point(tg::pos3 p, material const& m)
    {
        auto& pt = _points_curr->emplace_back();
        pt.pos = p;
        pt.color = m.is_default() ? _mat.is_default() ? tg::color4(.7f, .7f, .7f, 1.f) : _mat.color : m.color;
        pt.size = _points_size;
        return {array_view<point>(&pt, 1), *this};
    }
    splat_ref _add_splat(tg::pos3 p, tg::vec3 normal, material const& m)
    {
        auto& s = _splats.emplace_back();
        s.pos = p;
        s.normal = normal;
        s.color = m.is_default() ? _mat.is_default() ? tg::color4(.7f, .7f, .7f, 1.f) : _mat.color : m.color;
        s.size = _splats_size;
        return array_view<splat>(&s, 1);
    }
    line_ref _add_line(tg::pos3 p0, tg::pos3 p1, material const& m)
    {
        auto& l = _lines_curr->emplace_back();
        l.p0.pos = p0;
        l.p1.pos = p1;
        l.p0.color = l.p1.color = m.is_default() ? _mat.is_default() ? tg::color4(.25f, .25f, .25f, 1.f) : _mat.color : m.color;
        l.p0.size = l.p1.size = _lines_width;
        return array_view<line>(&l, 1);
    }
    triangle_ref _add_triangle(tg::pos3 p0, tg::pos3 p1, tg::pos3 p2, tg::vec3 n0, tg::vec3 n1, tg::vec3 n2, material const& m)
    {
        auto& t = _triangles.emplace_back();
        t.pos[0] = p0;
        t.pos[1] = p1;
        t.pos[2] = p2;
        t.normal[0] = n0;
        t.normal[1] = n1;
        t.normal[2] = n2;
        t.color[0] = t.color[1] = t.color[2] = m.is_default() ? _mat.is_default() ? tg::color4(1.f) : _mat.color : m.color;
        return array_view<triangle>(&t, 1);
    }

    // ctors
    // (copy and move is deleted because e.g. _line_curr need stable pointers)
public:
    canvas_data() = default;
    canvas_data(canvas_data const&) = delete;
    canvas_data(canvas_data&&) = delete;
    canvas_data& operator=(canvas_data const&) = delete;
    canvas_data& operator=(canvas_data&&) = delete;
};

/// A canvas is a helper to draw large amounts of heterogeneous primitives efficiently
///
/// Usage:
///
///   auto c = gv::canvas();
///   c.add_lines(...)
///   c.add_points(...)
///   c.add_splats(...)
///
/// Note: settings like colors or sizes are sticky / stateful but can be overridden per-call
///
/// gv::canvas_data can be used to build canvas data without showing it:
///
/// Usage:
///
///   gv::canvas_data d;
///   d.add_xyz(...);
///
///   auto c = gv::canvas();
///   c.add_data(d);
///
/// TODO: lines with normals
struct canvas_t : canvas_data
{
    // on close, shows the view
    ~canvas_t();
};

inline canvas_t canvas() { return {}; }
}
