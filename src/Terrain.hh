#pragma once
#include "advanced/World.hh"
#include "advanced/transform.hh"
#include "ecs/Engine.hh"
#include <filesystem>
#include "Mesh3D.hh"


namespace gamedev
{
class Terrain
{
public:
    void Init(SharedEngineECS& ecs);

    void setGroundMesh(std::filesystem::path mesh_path);
    void setWaterMesh(std::filesystem::path mesh_path);
    void setGroundTexture(std::filesystem::path tex_path);
    void setWaterTexture(std::filesystem::path tex_path);

    void setGroundMesh(glow::SharedVertexArray& mesh);
    void setWaterMesh(glow::SharedVertexArray& mesh);
    void setGroundTexture(glow::SharedTexture2D& tex);
    void setWaterTexture(glow::SharedTexture2D& tex);

    InstanceHandle CreateGround(tg::pos3 pos, tg::size3 scale);
    InstanceHandle CreateWater(tg::pos3 pos, tg::size3 scale);

    float heightAt(const tg::pos2& p);
    float heightAt(const tg::pos3& p);

    tg::aabb3 GetSize();

private:
    void createHeightmap(const Mesh3D& mesh, const transform& xform);
    pm::minmax_t<tg::pos3> getMapGroundExtents();
    pm::minmax_t<tg::pos3> getMapWaterExtents();

private:
    SharedEngineECS mECS;

    InstanceHandle mGroundHandle;
    InstanceHandle mWaterHandle;

    Mesh3D mMeshGround;
    Mesh3D mMeshWater;

    glow::SharedVertexArray mVaoGround;
    glow::SharedVertexArray mVaoWater;

    glow::SharedTexture2D mTexGround;
    glow::SharedTexture2D mTexWater;

private:
    std::vector<std::vector<float>> mHeightMap;
    polymesh::minmax_t<tg::pos3> mMapExtents;
};
}
