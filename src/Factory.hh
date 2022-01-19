#pragma once
#include <glow/fwd.hh>
#include <filesystem>
#include "ecs/Engine.hh"
#include "GameObjects.hh"
#include "Components.hh"
#include "Terrain.hh"
#include <typed-geometry/tg-lean.hh>
#include <list>

/* Factory Main System
 *
 * A factory manages a set of presets of entities which can be added and accessed by readable string identifiers.
 * It can spawn new entities with a certain preset, but also clone them.
 * 
 * If a new preset is to be added, it is advised to let the factory create the entity itself using the asset manager:
 *    Register("../data/meshes/units/person.obj");    // Automatically checks if identifier is path and extracts filename as well
 *    auto handle = GetPreset("person");              // Can be accessed either by the path above or by its filename alone (without extension)
 *    // Do something with handle to modify preset
 * 
 * Creating an entity is as simple as:
 *    auto handle = Create("person");
 * 
 * To customize the registration of new entities, subclasses of factory can overwrite the Register() methods.
 * 
 */

namespace gamedev
{
class Factory
{
public:
    // Factory needs access to ECS and the asset manager
    void Init(SharedEngineECS& ecs, SharedGameObjects& obj);
    void AttachTerrain(std::shared_ptr<Terrain>& terrain);

    // Add a new object along with its configuration to the factories preset container.
    // Info: Better call Register() in conjunction with GetPreset() to add a new preset and alter it afterwards.
    void AddPreset(std::string identifier, InstanceHandle presetHandle);
    void AddPreset(std::string identifier, std::filesystem::path identifying_path, InstanceHandle presetHandle);

    // Returns the handle of the actual preset (i.e. to modify the preset)
    InstanceHandle GetPreset(std::string identifier);

    // Create a new object with the asset manager using asset_identifier
    // and add this object as a preset with the "prototype" component (prevents system usage of this instance).
    // Will either use the asset_identifier name as an identifier for the factory system and, if applicable, also add as a path identifier.
    // Or will use the explicitly given identifier within the factory system (and, if applicable, the explicitly given path).
    // Info: Preferred way to initally add presets.
    InstanceHandle Register(std::string asset_identifier, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, std::filesystem::path factory_path, float scaling = 1.0);

    // Creates a new instance and clone the components of an existing preset (along with some transformation)
    InstanceHandle Create(std::string identifier);
    InstanceHandle Create(std::string identifier, tg::pos3 position);
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation);
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation, tg::size3 scaling);
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::size3 scaling);

    // Creates a new instance and clones the components of the given handle (along with some transformation)
    InstanceHandle Clone(InstanceHandle templateHandle);
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position);
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation);
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation, tg::size3 scaling);
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::size3 scaling);

    void MarkLastCreated();

    // Registers each object in the vector for this system
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float scaling = 1.0);

    // Returns a list with all identifiers in lexicographic order (either name or path)
    std::vector<std::string> GetNameIdentifiers();
    std::vector<std::string> GetPathIdentifiers();
    std::vector<InstanceHandle> GetGeneratedHandles();
    std::vector<std::pair<InstanceHandle, std::string>> GetGeneratedIdentifiers();

    // Returns whether the given identifier is registered (checks name and path)
    bool exists(std::string identifier);

    // Load and save objects
    bool SaveObjects();
    bool SaveMarkedObjects(std::filesystem::path filepath);
    bool LoadObjects();
    bool LoadObjects(std::filesystem::path filepath);
    std::vector<InstanceHandle> LoadObjectsMarked(std::filesystem::path filepath);

    virtual ~Factory() {}

private:
    // Overwrites the given ID with the given handle. Won't do any sanity check.
    void OverwritePreset(int presetID, InstanceHandle templateHandle);
    bool SaveObjectsTo(std::filesystem::path filepath);
    bool SaveMarkedObjectsTo(std::filesystem::path filepath);
    std::vector<InstanceHandle> LoadObjectsFrom(std::filesystem::path filepath, bool marked = false);

    tg::aabb3 MaxBoundingBox(tg::aabb3& boundingBox);
protected:
    // Can be modified by subclasses to i.e. add some additional components while using the main class' registration method
    virtual void PostProcessRegistration(InstanceHandle presetHandle){};

protected:
    SharedEngineECS mECS;
    SharedGameObjects mObj;
    std::shared_ptr<Terrain> mT;
    int mFreeID = 0;
    std::pair<InstanceHandle, std::string> mLastCreated = {{std::uint32_t(-1)}, ""};

    // Save location
    std::filesystem::path mSavefilePath = "../data/config/undefined.fac";

    // Mappings to get from the object identifier (name or path) to the internal ID which uniquely identifies the presets handle
    std::unordered_map<std::string, int> mObjectNameToID;
    std::unordered_map<std::string, int> mObjectPathToID;
    // A unique mapping the the instance handle of the preset
    std::unordered_map<int, InstanceHandle> mIDToPreset;

    // Keeps track of all objects that were created (not cloned) by this factory (handle & object identifier)
    std::list<std::pair<InstanceHandle, std::string>> mGeneratedObjects;
    std::list<std::pair<InstanceHandle, std::string>> mMarkedObjects;
};
}
