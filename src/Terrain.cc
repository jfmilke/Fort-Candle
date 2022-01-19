#include "Terrain.hh"
#include <glow/objects/Texture2D.hh>

void gamedev::Terrain::Init(SharedEngineECS& ecs) { mECS = ecs; }

void gamedev::Terrain::setGroundMesh(std::filesystem::path mesh_path)
{
    mMeshGround.loadFromFile(mesh_path.string());
    mVaoGround = mMeshGround.createVertexArray();
}
void gamedev::Terrain::setWaterMesh(std::filesystem::path mesh_path)
{
    mMeshWater.loadFromFile(mesh_path.string());
    mVaoWater = mMeshWater.createVertexArray();
}
void gamedev::Terrain::setGroundTexture(std::filesystem::path tex_path)
{
    mTexGround = glow::Texture2D::createFromFile(tex_path.string(), glow::ColorSpace::sRGB);
}

void gamedev::Terrain::setWaterTexture(std::filesystem::path tex_path)
{
    mTexWater = glow::Texture2D::createFromFile(tex_path.string(), glow::ColorSpace::sRGB);
}

void gamedev::Terrain::setGroundMesh(glow::SharedVertexArray& mesh) { mVaoGround = mesh; }
void gamedev::Terrain::setWaterMesh(glow::SharedVertexArray& mesh) { mVaoWater = mesh; }
void gamedev::Terrain::setGroundTexture(glow::SharedTexture2D& tex) { mTexGround = tex; }
void gamedev::Terrain::setWaterTexture(glow::SharedTexture2D& tex) { mTexWater = tex; }

gamedev::InstanceHandle gamedev::Terrain::CreateGround(tg::pos3 pos, tg::size3 scale)
{
    if (!mVaoGround)
        return { unsigned(-1) };


    mGroundHandle = mECS->CreateInstance(mVaoGround, mTexGround, nullptr, nullptr);

    auto& groundTransform = mECS->GetInstance(mGroundHandle).xform;
    groundTransform.translation = pos - tg::pos3::zero;
    groundTransform.scale(scale);

    createHeightmap(mMeshGround, groundTransform);

    tg::aabb3 mapSize = tg::aabb3(mMapExtents.min, mMapExtents.max);
    tg::size3 cells = tg::size_of(mapSize);
    mECS->GetHashMap().Init({cells.width, cells.depth}, mapSize);
    
    auto& instance = mECS->GetInstance(mGroundHandle);
    instance.max_bounds = {mMapExtents.min, mMapExtents.max};

    auto render = mECS->CreateComponent<Render>(mGroundHandle);

    // mECS->GetHashMap().AddInstance(mGroundHandle, instance.xform.translation, instance.max_bounds);
    // mECS->GetHashMap2().Insert(mGroundHandle, mapSize, instance.xform.translation);

    return mGroundHandle;
}

gamedev::InstanceHandle gamedev::Terrain::CreateWater(tg::pos3 pos, tg::size3 scale)
{
    if (!mVaoWater)
        return { unsigned(-1) };

    mWaterHandle = mECS->CreateInstance(mVaoWater, mTexWater, nullptr, nullptr);
    auto& waterTransform = mECS->GetInstance(mWaterHandle).xform;
    waterTransform.translation = pos - tg::pos3::zero;
    waterTransform.scale(scale);
    // todo: maybe fix this and have the water have its real extents
    auto instance = mECS->GetInstance(mWaterHandle);
    instance.max_bounds = {mMapExtents.min, mMapExtents.max};

    // mECS->GetHashMap().AddInstance(mGroundHandle, instance.xform.translation, instance.max_bounds);

    auto render = mECS->CreateComponent<Render>(mWaterHandle);

    return mWaterHandle;
}

void gamedev::Terrain::createHeightmap(const Mesh3D& mesh, const transform& xform)
{
    // Extract the total size of the terrains XZ plane
    tg::ivec2 size;

    auto transform = tg::mat4(xform.transform_mat());

    // Scale matters
    auto mesh_minmax = mesh.position.minmax();
    mesh_minmax.max = transform * mesh_minmax.max;
    mesh_minmax.min = transform * mesh_minmax.min;

    size[0] = mesh_minmax.max.x - mesh_minmax.min.x;
    size[1] = mesh_minmax.max.z - mesh_minmax.min.z;

    // Generate height map upon its extents
    std::vector<std::vector<float>> height(size[1]);
    for (auto& row : height)
        row = std::move(std::vector<float>(size[0], -20000.f));
        //row.resize(size[0]);

    // Fill height map:
    // Iterate over all triangles..
    for (const auto& face : mesh.faces())
    {
        // ..extract all vertices of a triangle & construct new triangle objects from it..
        auto vertices = face.vertices().to_vector();
        tg::triangle2 triangle_XZ; // holds positional information
        tg::triangle3 triangle_XYZ(transform * mesh.position[vertices[0]], transform * mesh.position[vertices[1]], transform * mesh.position[vertices[2]]); // holds also height information
        // ..variable which controls where inside a single pixel the triangle is sampled..
        tg::vec2 subpixel_offs(0.5f);

        // ..fill the 2D triangle on XZ plane..
        triangle_XZ.pos0.x = triangle_XYZ.pos0.x;
        triangle_XZ.pos0.y = triangle_XYZ.pos0.z;

        triangle_XZ.pos1.x = triangle_XYZ.pos1.x;
        triangle_XZ.pos1.y = triangle_XYZ.pos1.z;

        triangle_XZ.pos2.x = triangle_XYZ.pos2.x;
        triangle_XZ.pos2.y = triangle_XYZ.pos2.z;

        // ..create function which fills the heightmap according to the resultant barycentric coordinates..
        auto func = [&triangle_XYZ, &height, &mesh_minmax](tg::ipos2 p, float a, float b) {
            auto interpolated = tg::interpolate(triangle_XYZ, a, b);
            int id_z = p[1] - mesh_minmax.min.z;
            int id_x = p[0] - mesh_minmax.min.x;
            height[id_z][id_x] = tg::max(height[id_z][id_x], interpolated.y);
        };

        // ..call the rasterization and let it fill the heightmap for each triangle.
        tg::rasterize(triangle_XZ, func, subpixel_offs);
    }

    mHeightMap = std::move(height);
    mMapExtents = mesh_minmax;
}



float gamedev::Terrain::heightAt(const tg::pos2& p)
{
    const tg::ipos2 p0 = tg::ifloor(p) - tg::ivec2(mMapExtents.min.x, mMapExtents.min.z);
    const float s = tg::fract(p.x);
    const float t = tg::fract(p.y);

    if (p0.x < 0)
        return 0.f;

    if (p0.x + 1 >= mHeightMap[0].size())
        return 0.f;

    if (p0.y < 0)
        return 0.f;

    if (p0.y + 1 >= mHeightMap.size())
        return 0.f;

    // Bilinear interpolation
    float h_y0 = tg::interpolate(mHeightMap[p0.y    ][p0.x], mHeightMap[p0.y][p0.x + 1], s);
    float h_y1 = tg::interpolate(mHeightMap[p0.y + 1][p0.x], mHeightMap[p0.y][p0.x + 1], s);
    
    float h = tg::interpolate(h_y0, h_y1, t);
    return h;
}

float gamedev::Terrain::heightAt(const tg::pos3& p)
{
    tg::pos2 p2D{p.x, p.z};
    return heightAt(p2D);
}

pm::minmax_t<tg::pos3> gamedev::Terrain::getMapGroundExtents()
{
    const auto transform = tg::mat4(mECS->GetInstance(mGroundHandle).xform.transform_mat());
    auto extents = mMeshGround.position.minmax();
    return {transform * extents.min, transform * extents.max};
}

pm::minmax_t<tg::pos3> gamedev::Terrain::getMapWaterExtents()
{
    const auto transform = tg::mat4(mECS->GetInstance(mWaterHandle).xform.transform_mat());
    auto extents = mMeshWater.position.minmax();
    return {transform * extents.min, transform * extents.max};
}

tg::aabb3 gamedev::Terrain::GetSize() { return {mMapExtents.min, mMapExtents.max}; }