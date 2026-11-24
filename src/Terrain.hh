#pragma once
#include <filesystem>
#include "Mesh3D.hh"
#include "advanced/World.hh"
#include "advanced/transform.hh"
#include "ecs/Engine.hh"


namespace gamedev
{
/**
 * The map: ground and water instances plus a height map of the ground for placing objects on the surface.
 *
 * Created by `Game::init()`.
 *
 *  - `CreateGround` rasterizes the transformed ground mesh into a height map with one sample per world unit on the x/z plane,
 *    and sets up the ECS spatial hash map to cover the map.
 *  - Factories, the `PhysicsSystem`, the `GamelogicSystem` and `Game` query `heightAt` to keep instances (and clicked positions) on the ground.
 */
class Terrain
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    /**
     * Loads the ground mesh (also needed for the height map) and uploads it.
     *
     * @param mesh_path Path of the OBJ file.
     */
    void setGroundMesh(std::filesystem::path mesh_path);

    /**
     * Loads the water mesh and uploads it.
     *
     * @param mesh_path Path of the OBJ file.
     */
    void setWaterMesh(std::filesystem::path mesh_path);

    /**
     * Loads the ground texture.
     *
     * @param tex_path Path of the image file (sRGB).
     */
    void setGroundTexture(std::filesystem::path tex_path);

    /**
     * Loads the water texture.
     *
     * @param tex_path Path of the image file (sRGB).
     */
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
     * Creates the rendered ground instance, builds the height map and initializes the ECS spatial hash map with one cell per world unit over the map.
     *
     * @param pos World position of the ground.
     * @param scale Scale of the ground mesh.
     * @return The ground instance, or an invalid handle if no ground mesh is set.
     */
    InstanceHandle CreateGround(tg::pos3 pos, tg::size3 scale);

    /**
     * Creates the rendered water instance.
     *
     * @param pos World position of the water.
     * @param scale Scale of the water mesh.
     * @return The water instance, or an invalid handle if no water mesh is set.
     */
    InstanceHandle CreateWater(tg::pos3 pos, tg::size3 scale);

    /**
     * Ground height, bilinearly interpolated from the height map.
     *
     * TODO: The second row of the interpolation reuses a sample of the first row (`[p0.y][p0.x + 1]`).
     *
     * @param p World position (x, z).
     * @return Height in world units; 0 outside the map and in its last row and column.
     */
    float heightAt(const tg::pos2& p);

    /// Ground height below the world position `p` (x/z are used).
    float heightAt(const tg::pos3& p);

    /// World-space bounds of the ground.
    tg::aabb3 GetSize();

private:
    /**
     * Rasterizes a mesh into `mHeightMap` (highest surface per cell, -20000 where there is none) and stores its bounds in `mMapExtents`.
     *
     * Assumes a triangle mesh.
     *
     * @param mesh Ground mesh (CPU side).
     * @param xform Transform of the ground instance.
     */
    void createHeightmap(const Mesh3D& mesh, const transform& xform);

    /// World-space bounds of the ground mesh.
    pm::minmax_t<tg::pos3> getMapGroundExtents();

    /// World-space bounds of the water mesh.
    pm::minmax_t<tg::pos3> getMapWaterExtents();

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).

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
