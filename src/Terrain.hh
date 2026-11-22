#pragma once
#include "advanced/World.hh"
#include "advanced/transform.hh"
#include "ecs/Engine.hh"
#include <filesystem>
#include "Mesh3D.hh"


namespace gamedev
{
/**
 * The map: ground and water instances plus a height map of the ground for placing objects on the surface.
 *
 * Created by `Game::init()`.
 *
 *  - `CreateGround` rasterizes the transformed ground mesh into a height map with one sample per world unit on the
 *    x/z plane, and sets up the ECS spatial hash map to cover the map.
 *  - Factories and the `PhysicsSystem` query `heightAt` to keep instances on the ground.
 */
class Terrain
{
public:
    /// Stores the ECS.
    void Init(SharedEngineECS& ecs);

    /// Loads the ground mesh (also needed for the height map) and uploads it.
    void setGroundMesh(std::filesystem::path mesh_path);

    /// Loads the water mesh and uploads it.
    void setWaterMesh(std::filesystem::path mesh_path);

    /// Loads the ground texture (sRGB).
    void setGroundTexture(std::filesystem::path tex_path);

    /// Loads the water texture (sRGB).
    void setWaterTexture(std::filesystem::path tex_path);

    /// Uses an existing VAO for the ground (no height map data, see `setGroundMesh(path)`).
    void setGroundMesh(glow::SharedVertexArray& mesh);

    /// Uses an existing VAO for the water.
    void setWaterMesh(glow::SharedVertexArray& mesh);

    /// Uses an existing ground texture.
    void setGroundTexture(glow::SharedTexture2D& tex);

    /// Uses an existing water texture.
    void setWaterTexture(glow::SharedTexture2D& tex);

    /**
     * Creates the rendered ground instance at `pos` with `scale`, builds the height map and initializes the ECS spatial
     * hash map with one cell per world unit over the map. Returns an invalid handle if no ground mesh is set.
     */
    InstanceHandle CreateGround(tg::pos3 pos, tg::size3 scale);

    /// Creates the rendered water instance at `pos` with `scale`. Returns an invalid handle if no water mesh is set.
    InstanceHandle CreateWater(tg::pos3 pos, tg::size3 scale);

    /**
     * Ground height at world position (x, z) = `p`, interpolated from the height map; 0 outside the map.
     *
     * TODO: The second row of the interpolation reuses a sample of the first row (`[p0.y][p0.x + 1]`).
     */
    float heightAt(const tg::pos2& p);

    /// Ground height below the world position `p` (x/z are used).
    float heightAt(const tg::pos3& p);

    /// World-space bounds of the ground.
    tg::aabb3 GetSize();

private:
    /**
     * Rasterizes `mesh` transformed by `xform` into `mHeightMap` (highest surface per cell, -20000 where there is
     * none) and stores the bounds in `mMapExtents`.
     */
    void createHeightmap(const Mesh3D& mesh, const transform& xform);

    /// World-space bounds of the ground mesh.
    pm::minmax_t<tg::pos3> getMapGroundExtents();

    /// World-space bounds of the water mesh.
    pm::minmax_t<tg::pos3> getMapWaterExtents();

private:
    SharedEngineECS mECS; ///< Shared ECS.

    InstanceHandle mGroundHandle; ///< Ground instance.
    InstanceHandle mWaterHandle;  ///< Water instance.

    Mesh3D mMeshGround; ///< Ground mesh (CPU side, for the height map).
    Mesh3D mMeshWater;  ///< Water mesh (CPU side).

    glow::SharedVertexArray mVaoGround; ///< Ground VAO.
    glow::SharedVertexArray mVaoWater;  ///< Water VAO.

    glow::SharedTexture2D mTexGround; ///< Ground texture.
    glow::SharedTexture2D mTexWater;  ///< Water texture.

private:
    std::vector<std::vector<float>> mHeightMap; ///< Heights indexed [z - min.z][x - min.x], one sample per world unit.
    polymesh::minmax_t<tg::pos3> mMapExtents;   ///< World-space bounds of the ground.
};
}
