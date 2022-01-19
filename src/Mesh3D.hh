#pragma once

#include <glow/fwd.hh>
#include <string>
#include <polymesh/Mesh.hh>

#include <typed-geometry/tg-lean.hh> // leans means only types, no functions

/// mesh derives from pm::Mesh, thus it has .vertices(), .faces(), etc.
struct Mesh3D : public pm::Mesh
{
    // attributes are stored as members
    pm::vertex_attribute<tg::pos3> position{*this};

    // normal, tangent, texCoord are stored per halfedge
    // (e.g. to support different normals on the same vertex for different faces)
    // (see image in https://www.graphics.rwth-aachen.de/media/openmesh_static/Documentations/OpenMesh-6.3-Documentation/a00010.html
    pm::halfedge_attribute<tg::vec3> normal{*this};
    pm::halfedge_attribute<tg::vec3> tangent{*this};
    pm::halfedge_attribute<tg::pos2> texCoord{*this};

    /// loads mesh from file
    ///
    /// by default, computed vertex tangents are interpolated from face tangents,
    /// for flat shaded objects (e.g. cube) this flag should be set to false
    ///
    /// returns true if file was loaded without errors
    bool loadFromFile(std::string const& filename, bool interpolate_tangents = true);

    /// uploads the mesh to the GPU (with a GL_TRIANGLES vertex array)
    ///
    /// vertex layout:
    ///     in vec3 aPosition;
    ///     in vec3 aNormal;
    ///     in vec3 aTangent;
    ///     in vec2 aTexCoord;
    glow::SharedVertexArray createVertexArray() const;

    glow::SharedVertexArray createBasicVertexArray() const;
};
