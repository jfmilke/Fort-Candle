#pragma once
#include <glow/fwd.hh>
#include <filesystem>
#include "ecs/Engine.hh"
#include "GameObjects.hh"
#include "Components.hh"
#include "Terrain.hh"
#include <typed-geometry/tg-lean.hh>
#include <list>

namespace gamedev
{
/**
 * Manages named entity presets and creates instances from them.
 *
 * Base class of the unit, structure, nature and item factories.
 *
 * Presets:
 *  - A preset is a `Prototype` instance (never part of any system) that carries the mesh and all components of an
 *    entity type.
 *  - `Create` clones a preset into a live instance.
 *  - Identifiers are case-insensitive and can be a name or a path:
 *    @code
 *    Register("../data/meshes/units/person.obj"); // registers the name "person" and the path
 *    auto preset = GetPreset("person");          // by name (file name without extension) or by path; modify as needed
 *    auto handle = Create("person", position);   // live instance
 *    @endcode
 *
 * Subclasses attach their components in `PostProcessRegistration` or in their own `Register` overloads.
 * `Game::init()` creates one instance of each subclass and registers all meshes of its folder in `data/meshes/`.
 *
 * World layouts are saved and loaded as `.fac` files in `data/config/`, a sequence of records
 * `[null-terminated preset name][tg::vec3 translation][tg::quat rotation][tg::size3 scaling]`.
 */
class Factory
{
public:
    /// Sets the ECS and the asset manager. Must be called before any other function.
    void Init(SharedEngineECS& ecs, SharedGameObjects& obj);

    /// Sets the terrain; created instances are then placed on its surface.
    void AttachTerrain(std::shared_ptr<Terrain>& terrain);

    /**
     * Adds `presetHandle` as preset under `identifier` (a name, or a path whose file name becomes the name).
     * If the identifier exists, the existing preset's components are replaced by copies of `presetHandle`'s.
     * Prefer `Register` + `GetPreset` to add and then modify a preset.
     */
    void AddPreset(std::string identifier, InstanceHandle presetHandle);

    /// Adds `presetHandle` under both the name `identifier` and the path `identifying_path` (see above).
    void AddPreset(std::string identifier, std::filesystem::path identifying_path, InstanceHandle presetHandle);

    /// Returns the preset instance for a name or path (to modify it), or an invalid handle.
    InstanceHandle GetPreset(std::string identifier);

    /**
     * Creates a `Prototype` instance from the asset `asset_identifier` (mesh name or path known to the asset manager),
     * scaled uniformly by `scaling`, runs `PostProcessRegistration` and adds it as preset under the asset name (and
     * path). Returns the preset, or an invalid handle if the asset is unknown. Preferred way to add presets.
     */
    InstanceHandle Register(std::string asset_identifier, float scaling = 1.0);

    /// Like above, but registers the preset under `factory_identifier`.
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, float scaling = 1.0);

    /// Like above, but registers the preset under `factory_identifier` and `factory_path`.
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, std::filesystem::path factory_path, float scaling = 1.0);

    /**
     * Creates a live instance from the preset `identifier` (without the `Prototype` component), adds it to the spatial
     * hash map and remembers it for saving. Returns an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier);

    /// Like `Create`, placed at `position` (height taken from the terrain, if attached).
    InstanceHandle Create(std::string identifier, tg::pos3 position);

    /// Like `Create`, placed at `position` with `rotation`.
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation);

    /// Like `Create`, placed at `position` with `rotation` and `scaling` (replaces the preset scale).
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation, tg::size3 scaling);

    /// Like `Create`, placed at `position` with `scaling` (replaces the preset scale).
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::size3 scaling);

    /**
     * Creates a copy of `templateHandle` with all its components (a `Prototype` stays a prototype) and adds it to the
     * spatial hash map. Unlike `Create`, the copy is not remembered for saving.
     */
    InstanceHandle Clone(InstanceHandle templateHandle);

    /// Like `Clone`, placed at `position` (height taken from the terrain, if attached).
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position);

    /// Like `Clone`, placed at `position` with `rotation`.
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation);

    /// Like `Clone`, placed at `position` with `rotation` and `scaling`.
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation, tg::size3 scaling);

    /// Like `Clone`, placed at `position` with `scaling`.
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::size3 scaling);

    /// Marks the instance of the last `Create` call, so `SaveMarkedObjects` saves it.
    void MarkLastCreated();

    /// `Register`s every asset in `asset_identifiers` with the same `scaling`.
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float scaling = 1.0);

    /// All preset names (unordered).
    std::vector<std::string> GetNameIdentifiers();

    /**
     * Meant to return all preset paths.
     *
     * TODO: Returns the names instead (it iterates the name map).
     */
    std::vector<std::string> GetPathIdentifiers();

    /// Handles of all live instances created by this factory.
    std::vector<InstanceHandle> GetGeneratedHandles();

    /// Handles and preset names of all live instances created by this factory.
    std::vector<std::pair<InstanceHandle, std::string>> GetGeneratedIdentifiers();

    /// True if a preset with this name or path exists.
    bool exists(std::string identifier);

    /// Saves all live instances created by this factory to `mSavefilePath`. Returns false on I/O errors.
    bool SaveObjects();

    /// Saves all live marked instances to `filepath` (a full path). Returns false on I/O errors.
    bool SaveMarkedObjects(std::filesystem::path filepath);

    /// Creates the instances saved in `mSavefilePath`. Returns false if none were loaded.
    bool LoadObjects();

    /// Creates the instances saved in `data/config/<filepath>.fac`. Returns false if none were loaded.
    bool LoadObjects(std::filesystem::path filepath);

    /// Like `LoadObjects(filepath)`, but also marks the created instances and returns their handles.
    std::vector<InstanceHandle> LoadObjectsMarked(std::filesystem::path filepath);

    virtual ~Factory() {}

private:
    /// Replaces the components of preset `presetID` by copies of `templateHandle`'s. No sanity checks.
    void OverwritePreset(int presetID, InstanceHandle templateHandle);

    /// Writes all live created instances to `filepath` (`.fac` format).
    bool SaveObjectsTo(std::filesystem::path filepath);

    /// Writes all live marked instances to `filepath` (`.fac` format).
    bool SaveMarkedObjectsTo(std::filesystem::path filepath);

    /**
     * Creates one instance per `.fac` record of `filepath` with the saved transform (including the saved height) and
     * returns their handles; with `marked`, also marks them.
     */
    std::vector<InstanceHandle> LoadObjectsFrom(std::filesystem::path filepath, bool marked = false);

    /// Returns `boundingBox` unchanged (the code after the `return` is dead).
    tg::aabb3 MaxBoundingBox(tg::aabb3& boundingBox);
protected:
    /// Hook for subclasses, called by `Register` before the preset is added, e.g. to attach components.
    virtual void PostProcessRegistration(InstanceHandle presetHandle){};

protected:
    SharedEngineECS mECS;                                                            ///< Shared ECS.
    SharedGameObjects mObj;                                                          ///< Asset manager (meshes, textures).
    std::shared_ptr<Terrain> mT;                                                     ///< Terrain for placement, may be null.
    int mFreeID = 0;                                                                 ///< Next preset ID.
    std::pair<InstanceHandle, std::string> mLastCreated = {{std::uint32_t(-1)}, ""}; ///< Last `Create` result.

    std::filesystem::path mSavefilePath = "../data/config/undefined.fac"; ///< Default save file; its folder is used by `LoadObjects(path)`.

    std::unordered_map<std::string, int> mObjectNameToID; ///< Preset name -> preset ID.
    std::unordered_map<std::string, int> mObjectPathToID; ///< Preset path -> preset ID.
    std::unordered_map<int, InstanceHandle> mIDToPreset;  ///< Preset ID -> preset instance.

    std::list<std::pair<InstanceHandle, std::string>> mGeneratedObjects; ///< Instances created (not cloned) by this factory, with preset name.
    std::list<std::pair<InstanceHandle, std::string>> mMarkedObjects;    ///< Instances marked for `SaveMarkedObjects`.
};
}
