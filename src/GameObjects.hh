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

class GameObjects
{
public:
    GameObjects() = default;

public:
    // meshes
    // misc
    Mesh3D meshCube;

    // terrain
    Mesh3D meshWorld;
    Mesh3D meshWorld_ground;
    Mesh3D meshWorld_water;

    // vaos
    // misc
    glow::SharedVertexArray mVaoCube;

    // terrain
    glow::SharedVertexArray mVaoWorld;
    glow::SharedVertexArray mVaoWorld_ground;
    glow::SharedVertexArray mVaoWorld_water;

    // textures
    glow::SharedTexture2D mTexAlbedoPolyPalette;
    glow::SharedTexture2D mTexNormalWater;

public:
    // (deprecated) Initialize predefined meshes
    void initialize();

    // Scans all files and registers them as Asset
    void scanMeshes();

    // Returns a vector with the (string) paths of all obj-files which are registered and have the depicted parent folder.
    std::vector<std::string> getObjectsByFolder(std::string parent_folder);

    // Register an asset
    bool registerObject(std::filesystem::path mesh_path);
    // Get VAO using either the asset-filename or the asset-filepath
    glow::SharedVertexArray getVAO(std::filesystem::path identifier);
    // Get AABB using either the asset-filename or the asset-filepath
    tg::aabb3 getAABB(std::filesystem::path identifier);
    // Check if a certain object identifier (filename or filepath) is registered
    bool hasObject(std::filesystem::path mesh_identifier);

    // Transforms aabb into oriented box
    tg::box3 aabb3_as_box(const tg::aabb3& aabb);
    // Transforms aabb into circle, but constructs radius from side-lengths, not from radius (favors a smaller circle)
    tg::sphere3 aabb3_as_circle(const tg::aabb3& aabb);

    // ToDo: Construct OBB
    //       1. Center of gravity c = 1/n * sum(x_i)
    //       2. Compute covariance matrix of all points
    //       3. The Eigenvectors of this matrix represent the principal directions
    // ToDo: Construct Circles
    //       1. Center of gravity c = 1/n * sum(x_i)
    //       2. Radius r = max(|x_i - c|)

private:
    void initializeMeshes();
    void initializeTextures();

    std::unordered_map<std::string, std::string> mMapNameToID;
    std::unordered_map<std::string, glow::SharedVertexArray> mMapObjectPathToVAO;
    std::unordered_map<std::string, tg::aabb3> mMapObjectPathToAABB;
};
}
