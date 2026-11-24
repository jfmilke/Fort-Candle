#pragma once

#include <glow/fwd.hh>
#include <polymesh/Mesh.hh>
#include <string>

#include <typed-geometry/tg-lean.hh> // leans means only types, no functions

/**
 * Polygon mesh with per-halfedge normals, tangents and texture coordinates, loaded from OBJ files.
 *
 * Course framework code.
 * Derives from `pm::Mesh`, so it has `.vertices()`, `.faces()`, etc.
 * Loading relies on the patched polymesh OBJ reader (`has_valid_texcoords()`/`has_valid_normals()`, see CLAUDE.md).
 */
struct Mesh3D : public pm::Mesh
{
    /// Vertex positions (attributes are stored as members).
    pm::vertex_attribute<tg::pos3> position{*this};

    // normal, tangent, texCoord are stored per halfedge
    // (e.g. to support different normals on the same vertex for different faces)
    // (see image in https://www.graphics.rwth-aachen.de/media/openmesh_static/Documentations/OpenMesh-6.3-Documentation/a00010.html
    pm::halfedge_attribute<tg::vec3> normal{*this};   ///< Normal per halfedge (corner).
    pm::halfedge_attribute<tg::vec3> tangent{*this};  ///< Tangent per halfedge (corner).
    pm::halfedge_attribute<tg::pos2> texCoord{*this}; ///< Texture coordinate per halfedge (corner).

    /**
     * Loads an OBJ file, computes area-weighted vertex normals if the file has none, and computes tangents.
     *
     * Tangents are computed per face from its first three corners.
     * Returns false (and logs) if the file cannot be opened, has faces that cannot be added, or has no texture coordinates;
     * in the last case positions and normals are still loaded, but the tangents stay zero.
     *
     * @param filename Path of the OBJ file.
     * @param interpolate_tangents Average the tangents per vertex (smooth); false keeps one tangent per face, for flat shaded objects (e.g. a cube).
     * @return True if the mesh was loaded completely.
     */
    bool loadFromFile(std::string const& filename, bool interpolate_tangents = true);

    /**
     * Uploads the mesh to the GPU as a GL_TRIANGLES vertex array with one vertex per halfedge;
     * for non-triangle meshes an index buffer fans each polygon into triangles.
     *
     * Needs an OpenGL context.
     *
     * vertex layout:
     *     in vec3 aPosition;
     *     in vec3 aNormal;
     *     in vec3 aTangent;
     *     in vec2 aTexCoord;
     *
     * @return The new vertex array.
     */
    glow::SharedVertexArray createVertexArray() const;

    /// Like `createVertexArray`, but only with `aPosition` and `aNormal` (used for the particle cube).
    glow::SharedVertexArray createBasicVertexArray() const;
};
