#pragma once
#include <filesystem>
#include <glow/fwd.hh>
#include <glow/objects/VertexArray.hh>
#include <string>
#include <typed-geometry/tg.hh>
#include <unordered_map>
#include "Mesh3D.hh"


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
    Mesh3D meshCube; ///< `cube.obj`, loaded by `initialize`. Unused.

    // terrain
    Mesh3D meshWorld;        ///< `world_demo.obj`, loaded by `initialize`. Unused.
    Mesh3D meshWorld_ground; ///< `world_ground.obj`, loaded by `initialize`. Unused.
    Mesh3D meshWorld_water;  ///< `world_water.obj`, loaded by `initialize`. Unused.

    // vaos
    // misc
    glow::SharedVertexArray mVaoCube; ///< VAO of `meshCube`. Unused.

    // terrain
    glow::SharedVertexArray mVaoWorld;        ///< VAO of `meshWorld`. Unused.
    glow::SharedVertexArray mVaoWorld_ground; ///< VAO of `meshWorld_ground`. Unused.
    glow::SharedVertexArray mVaoWorld_water;  ///< VAO of `meshWorld_water`. Unused.

    // textures
    glow::SharedTexture2D mTexAlbedoPolyPalette; ///< Shared color palette texture (`data/textures/BigPalette.png`, sRGB).
    glow::SharedTexture2D mTexNormalWater;       ///< Never loaded (null).

public:
    /// Loads the predefined meshes above and the palette texture. Still required for `mTexAlbedoPolyPalette`.
    void initialize();

    /// Registers every `.obj` file under `data/meshes/` (recursively) with `registerObject`.
    void scanMeshes();

    /**
     * Lists the registered meshes in a folder.
     *
     * TODO: Iterates the name map, so a mesh is left out if a file with the same name was scanned before it.
     * Five meshes in `data/meshes/` share their names with meshes in `nature/` (e.g. `bush.obj`);
     * which one is listed depends on the scan order of the file system.
     *
     * @param parent_folder Name of the direct parent folder (e.g. "units"); case-sensitive, paths are lower-case.
     * @return Lower-case paths of the meshes.
     */
    std::vector<std::string> getObjectsByFolder(std::string parent_folder);

    /**
     * Loads a mesh, uploads it and stores its VAO and bounds under its lower-case path and name.
     *
     * If two files share a name, the name keeps referring to the first one.
     *
     * @param mesh_path Path of the `.obj` file.
     * @return False if `Mesh3D::loadFromFile` fails (also for meshes without texture coordinates) or the path is already registered.
     */
    bool registerObject(std::filesystem::path mesh_path);

    /// VAO of a registered mesh by name or path (an unknown path falls back to its file name), or nullptr.
    glow::SharedVertexArray getVAO(std::filesystem::path identifier);

    /// Local-space bounds of a registered mesh by name or path (falls back like `getVAO`), or an empty box.
    tg::aabb3 getAABB(std::filesystem::path identifier);

    /// True if a mesh with this name or path is registered (falls back like `getVAO`).
    bool hasObject(std::filesystem::path mesh_identifier);

    /**
     * Converts a box into a flat box on the ground plane.
     *
     * @param aabb Box to convert.
     * @return Box centered below the center of `aabb` at y = 0, with the x/z extent of `aabb` and no height.
     */
    tg::box3 aabb3_as_box(const tg::aabb3& aabb);

    /**
     * Converts a box into a circle on the ground plane.
     *
     * TODO: The radius is computed as max(x extent, max.z, min.z) / 2, probably meant to be max(x extent, z extent) / 2.
     *
     * @param aabb Box to convert.
     * @return Sphere centered below the center of `aabb` at y = 0.
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
