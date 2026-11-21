#pragma once
#include <glow/fwd.hh>
#include "Mesh3D.hh"
#include <unordered_map>
#include <filesystem>
#include <string>
#include <glow/objects/VertexArray.hh>
#include <typed-geometry/tg.hh>


namespace gamedev
{
GLOW_SHARED(class, GameObjects);

/**
 * Asset manager: loads every `.obj` mesh under `data/meshes/` once and hands out its VAO and bounds.
 *
 * Created by `Game::init()` and shared (as `SharedGameObjects`) with the factories. Needs an OpenGL context.
 *
 * Assets are identified case-insensitively, either
 *  - by their file name without extension (e.g. `person`), or
 *  - by their path as scanned (e.g. `../data/meshes/units/person.obj`).
 *
 * All game meshes are textured with the shared palette texture `mTexAlbedoPolyPalette`.
 */
class GameObjects
{
public:
    GameObjects() = default;

public:
    // meshes
    // misc
    Mesh3D meshCube; ///< `cube.obj` (loaded by `initialize`, unused).

    // terrain
    Mesh3D meshWorld;        ///< `world_demo.obj` (loaded by `initialize`, unused).
    Mesh3D meshWorld_ground; ///< `world_ground.obj` (loaded by `initialize`, unused).
    Mesh3D meshWorld_water;  ///< `world_water.obj` (loaded by `initialize`, unused).

    // vaos
    // misc
    glow::SharedVertexArray mVaoCube; ///< VAO of `meshCube` (unused).

    // terrain
    glow::SharedVertexArray mVaoWorld;        ///< VAO of `meshWorld` (unused).
    glow::SharedVertexArray mVaoWorld_ground; ///< VAO of `meshWorld_ground` (unused).
    glow::SharedVertexArray mVaoWorld_water;  ///< VAO of `meshWorld_water` (unused).

    // textures
    glow::SharedTexture2D mTexAlbedoPolyPalette; ///< Shared color palette texture (`data/textures/BigPalette.png`, sRGB).
    glow::SharedTexture2D mTexNormalWater;       ///< Never loaded (null).

public:
    /// Loads the predefined meshes above and the palette texture. Still required for `mTexAlbedoPolyPalette`.
    void initialize();

    /// Registers every `.obj` file under `data/meshes/` (recursively) with `registerObject`.
    void scanMeshes();

    /// Returns the paths of all registered meshes whose direct parent folder is named `parent_folder` (e.g. "units").
    std::vector<std::string> getObjectsByFolder(std::string parent_folder);

    /**
     * Loads the mesh at `mesh_path`, uploads it and stores its VAO and bounds. Returns false if the mesh is empty or
     * the path is already registered. If two files share a name, the name keeps referring to the first one.
     */
    bool registerObject(std::filesystem::path mesh_path);

    /// VAO of a registered mesh by name or path, or nullptr.
    glow::SharedVertexArray getVAO(std::filesystem::path identifier);

    /// Local-space bounds of a registered mesh by name or path, or an empty box.
    tg::aabb3 getAABB(std::filesystem::path identifier);

    /// True if a mesh with this name or path is registered.
    bool hasObject(std::filesystem::path mesh_identifier);

    /// Converts `aabb` into a flat box on the ground plane (y = 0) with the x/z extent of `aabb`.
    tg::box3 aabb3_as_box(const tg::aabb3& aabb);

    /**
     * Converts `aabb` into a circle on the ground plane (y = 0) around its center.
     *
     * TODO: The radius is computed as max(x extent, max.z, min.z) / 2, probably meant to be
     * max(x extent, z extent) / 2.
     */
    tg::sphere3 aabb3_as_circle(const tg::aabb3& aabb);

    // ToDo: Construct OBB
    //       1. Center of gravity c = 1/n * sum(x_i)
    //       2. Compute covariance matrix of all points
    //       3. The Eigenvectors of this matrix represent the principal directions
    // ToDo: Construct Circles
    //       1. Center of gravity c = 1/n * sum(x_i)
    //       2. Radius r = max(|x_i - c|)

private:
    /// Loads and uploads the predefined meshes.
    void initializeMeshes();

    /// Loads the palette texture.
    void initializeTextures();

    std::unordered_map<std::string, std::string> mMapNameToID;                    ///< Lower-case name -> path.
    std::unordered_map<std::string, glow::SharedVertexArray> mMapObjectPathToVAO; ///< Lower-case path -> VAO.
    std::unordered_map<std::string, tg::aabb3> mMapObjectPathToAABB;              ///< Lower-case path -> local bounds.
};
}
