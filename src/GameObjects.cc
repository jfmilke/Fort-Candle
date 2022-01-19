#include "GameObjects.hh"
#include <glow/objects/Texture2D.hh>
#include <algorithm>
#include <functional>

void gamedev::GameObjects::initialize()
{
    initializeMeshes();
    initializeTextures();
}

void gamedev::GameObjects::initializeMeshes()
{
    // misc
    meshCube.loadFromFile("../data/meshes/cube.obj", false);

    // terrain
    meshWorld.loadFromFile("../data/meshes/world_demo.obj", false);
    meshWorld_ground.loadFromFile("../data/meshes/world_ground.obj", false);
    meshWorld_water.loadFromFile("../data/meshes/world_water.obj", true);

    // prepare upload to gpu
    // misc
    mVaoCube = meshCube.createVertexArray();

    // terrain
    mVaoWorld = meshWorld.createVertexArray();
    mVaoWorld_ground = meshWorld_ground.createVertexArray();
    mVaoWorld_water = meshWorld_water.createVertexArray();
}

void gamedev::GameObjects::initializeTextures()
{
    mTexAlbedoPolyPalette = glow::Texture2D::createFromFile("../data/textures/BigPalette.png", glow::ColorSpace::sRGB);
    //mTexNormalWater = glow::Texture2D::createFromFile("../data/textures/water.normal.png", glow::ColorSpace::Linear);
}

void gamedev::GameObjects::scanMeshes()
{
    for (const auto& obj : std::filesystem::recursive_directory_iterator("../data/meshes/"))
    {
        // Check that obj is no directory
        if (!obj.is_regular_file())
        {
            continue;
        }

        // Only accept obj files
        if (!(obj.path().extension().compare(".obj") == 0))
        {
            continue;
        }
        
        // Correct path to forward slash
        auto path = obj.path().string();
        std::replace(path.begin(), path.end(), '\\', '/');
        
        registerObject(path);
    }
}

std::vector<std::string> gamedev::GameObjects::getObjectsByFolder(std::string parent_folder)
{
    std::vector<std::string> objects;

    for (const auto& obj : mMapNameToID)
    {
        auto path = std::filesystem::path(obj.second);

        if (path.parent_path().filename().compare(parent_folder) == 0)
        {
            objects.push_back(obj.second);
        }
    }

    return objects;
}

bool gamedev::GameObjects::hasObject(std::filesystem::path mesh_identifier)
{
    std::string id = mesh_identifier.string();
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    if (mesh_identifier.has_parent_path())
    {
        if (mMapObjectPathToVAO.find(id) != mMapObjectPathToVAO.end())
            return true;

        id = mesh_identifier.filename().string();
        id = id.substr(0, id.find_last_of("."));
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    }

    if (mMapNameToID.find(id) != mMapNameToID.end())
    {
        return true;
    }

    return false;
}

bool gamedev::GameObjects::registerObject(std::filesystem::path mesh_path)
{
    Mesh3D mesh;

    if(!mesh.loadFromFile(mesh_path.string()))
    {
        glow::error() << "Registering empty mesh: " << mesh_path.string();
        return false;
    }

    auto aabb = mesh.position.aabb();
    auto vao = mesh.createVertexArray();

    // Make objPath relative to a canonical location
    std::string objPath = mesh_path.string();
    std::transform(objPath.begin(), objPath.end(), objPath.begin(), ::tolower);

    // Extract filename without extension
    std::string objName = mesh_path.filename().string();
    objName = objName.substr(0, objName.find_last_of('.'));
    std::transform(objName.begin(), objName.end(), objName.begin(), ::tolower);

    // Check if objPath is already mapped
    if (mMapObjectPathToVAO.find(objPath) != mMapObjectPathToVAO.end())
    {
        glow::info() << "Object exists already: " << objPath;
        return false;
    }

    mMapNameToID.insert({objName, objPath});
    mMapObjectPathToVAO.insert({objPath, vao});
    mMapObjectPathToAABB.insert({objPath, tg::aabb3(aabb.min, aabb.max)});

    return true;
}

glow::SharedVertexArray gamedev::GameObjects::getVAO(std::filesystem::path identifier)
{
    auto id = identifier.string();
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    if (identifier.has_parent_path())
    {
        if (mMapObjectPathToVAO.find(id) != mMapObjectPathToVAO.end())
            return mMapObjectPathToVAO[id];

        glow::error() << "Could not find GameObject path.";
        glow::info() << "Trying object name.";


        id = identifier.filename().string();
        id = id.substr(0, id.find_last_of("."));
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    }


    if (mMapNameToID.find(id) != mMapNameToID.end())
    {
        return mMapObjectPathToVAO[mMapNameToID[id]];
    }

    return nullptr;
}

tg::aabb3 gamedev::GameObjects::getAABB(std::filesystem::path identifier)
{
    auto id = identifier.string();
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    if (identifier.has_parent_path())
    {
        if (mMapObjectPathToVAO.find(id) != mMapObjectPathToVAO.end())
            return mMapObjectPathToAABB[id];

        glow::error() << "Could not find GameObject path. Trying object name.";

        id = identifier.filename().string();
        id = id.substr(0, id.find_last_of("."));
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    }

    if (mMapNameToID.find(id) != mMapNameToID.end())
    {
        return mMapObjectPathToAABB[mMapNameToID[id]];
    }
    
    return {};
}

tg::box3 gamedev::GameObjects::aabb3_as_box(const tg::aabb3& aabb)
{
    tg::box3 b;
    b.center = aabb.min + (aabb.max - aabb.min) / 2.0;
    b.center.y = 0.0;
    b.half_extents[0] = tg::vec3((aabb.max.x - aabb.min.x) / 2.0, 0, 0);
    b.half_extents[1] = tg::vec3::zero;
    b.half_extents[2] = tg::vec3(0, 0, (aabb.max.z - aabb.min.z) / 2.0);
    return b;
}

tg::sphere3 gamedev::GameObjects::aabb3_as_circle(const tg::aabb3& aabb)
{
    tg::sphere3 s;
    const float a = (aabb.max.x - aabb.min.x);
    const float b = (aabb.max.z - aabb.min.z);
    s.center = aabb.min + (aabb.max - aabb.min) / 2.0;
    s.center.y = 0.0;
    s.radius = tg::max(aabb.max.x - aabb.min.x, aabb.max.z, aabb.min.z) / 2.0;

    return s;
}