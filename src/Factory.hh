#pragma once
#include <filesystem>
#include <glow/fwd.hh>
#include <list>
#include <typed-geometry/tg-lean.hh>
#include "Components.hh"
#include "GameObjects.hh"
#include "Terrain.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
/**
 * Manages named entity presets and creates instances from them.
 *
 * Base class of the unit, structure, nature and item factories.
 *
 * Presets:
 *  - A preset is a `Prototype` instance (never part of any system) that carries the mesh and all components of an entity type.
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
    void Init(EngineECS& ecs, SharedGameObjects& obj);

    /// Sets the terrain; created instances are then placed on its surface.
    void AttachTerrain(std::shared_ptr<Terrain>& terrain);

    /**
     * Adds `presetHandle` as preset under `identifier`.
     *
     * If the identifier exists, the existing preset's components are replaced by copies of `presetHandle`'s
     * (and `presetHandle` itself is not stored).
     * Prefer `Register` + `GetPreset` to add and then modify a preset.
     *
     * @param identifier Name, or path whose file name without extension becomes the name (both are then added).
     * @param presetHandle `Prototype` instance to use as preset.
     */
    void AddPreset(std::string identifier, InstanceHandle presetHandle);

    /**
     * Adds `presetHandle` as preset under a name and a path.
     *
     * If the name or the path exists, that preset is overwritten as in `AddPreset(identifier, presetHandle)` and gets the other identifier as well.
     *
     * @param identifier Preset name.
     * @param identifying_path Preset path.
     * @param presetHandle `Prototype` instance to use as preset.
     */
    void AddPreset(std::string identifier, std::filesystem::path identifying_path, InstanceHandle presetHandle);

    /// Returns the preset instance for a name or path (to modify it), or an invalid handle.
    InstanceHandle GetPreset(std::string identifier);

    /**
     * Creates a preset from an asset and adds it under the asset's name (and path, if `asset_identifier` is one).
     *
     * Preferred way to add presets.
     *
     * The preset is an instance with the asset's VAO and the palette texture (no normal or ARM texture),
     * the `Prototype` component and a uniform scale.
     * Its `max_bounds` is the asset's AABB scaled by `scaling * 1.15`.
     * `PostProcessRegistration` runs after the `Prototype` component is added and before the preset is added.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, float scaling = 1.0);

    /**
     * Like `Register(asset_identifier, scaling)`, but adds the preset under `factory_identifier`, and `max_bounds` is scaled by `scaling * 1.25`.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param factory_identifier Preset name, or path whose file name becomes the name.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, float scaling = 1.0);

    /**
     * Like `Register(asset_identifier, factory_identifier, scaling)`, but adds the preset under `factory_identifier` and `factory_path`.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param factory_identifier Preset name.
     * @param factory_path Preset path.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string factory_identifier, std::filesystem::path factory_path, float scaling = 1.0);

    /**
     * Creates a live instance from a preset, at the preset's transform.
     *
     * The instance gets copies of all components except `Prototype`, so it joins the systems.
     * It is added to the spatial hash map, published (`EngineECS::PublishPosition`) and remembered for saving and for `MarkLastCreated`.
     *
     * @param identifier Preset name or path.
     * @return The new instance, or an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier);

    /**
     * Like `Create(identifier)`, placed at `position`.
     *
     * @param identifier Preset name or path.
     * @param position World position; the height is taken from the terrain, if attached.
     * @return The new instance, or an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier, tg::pos3 position);

    /**
     * Like `Create(identifier, position)`, with `rotation`.
     *
     * @param identifier Preset name or path.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param rotation Rotation (replaces the preset rotation).
     * @return The new instance, or an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation);

    /**
     * Like `Create(identifier, position)`, with `rotation` and `scaling`.
     *
     * @param identifier Preset name or path.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param rotation Rotation (replaces the preset rotation).
     * @param scaling Scale (replaces the preset scale).
     * @return The new instance, or an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::quat rotation, tg::size3 scaling);

    /**
     * Like `Create(identifier, position)`, with `scaling`.
     *
     * @param identifier Preset name or path.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param scaling Scale (replaces the preset scale).
     * @return The new instance, or an invalid handle if the preset does not exist.
     */
    InstanceHandle Create(std::string identifier, tg::pos3 position, tg::size3 scaling);

    /**
     * Creates a copy of an instance with all its components (a `Prototype` stays a prototype) and adds it to the spatial hash map.
     *
     * Unlike `Create`, the copy is not remembered for saving, and it is not published right away.
     *
     * @param templateHandle Live instance to copy.
     * @return The copy.
     */
    InstanceHandle Clone(InstanceHandle templateHandle);

    /**
     * Like `Clone(templateHandle)`, placed at `position` and published (`EngineECS::PublishPosition`).
     *
     * @param templateHandle Live instance to copy.
     * @param position World position; the height is taken from the terrain, if attached.
     * @return The copy.
     */
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position);

    /**
     * Like `Clone(templateHandle, position)`, with `rotation`.
     *
     * @param templateHandle Live instance to copy.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param rotation Rotation (replaces the template's rotation).
     * @return The copy.
     */
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation);

    /**
     * Like `Clone(templateHandle, position)`, with `rotation` and `scaling`.
     *
     * @param templateHandle Live instance to copy.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param rotation Rotation (replaces the template's rotation).
     * @param scaling Scale (replaces the template's scale).
     * @return The copy.
     */
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::quat rotation, tg::size3 scaling);

    /**
     * Like `Clone(templateHandle, position)`, with `scaling`.
     *
     * @param templateHandle Live instance to copy.
     * @param position World position; the height is taken from the terrain, if attached.
     * @param scaling Scale (replaces the template's scale).
     * @return The copy.
     */
    InstanceHandle Clone(InstanceHandle templateHandle, tg::pos3 position, tg::size3 scaling);

    /// Marks the instance of the last `Create` call, so `SaveMarkedObjects` saves it (once per call).
    void MarkLastCreated();

    /**
     * `Register`s every asset with the same scale.
     *
     * @param asset_identifiers Mesh names or paths known to the asset manager.
     * @param scaling Uniform scale of the presets.
     */
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float scaling = 1.0);

    /// All preset names (unordered).
    std::vector<std::string> GetNameIdentifiers();

    /**
     * All preset paths (unordered). Unused.
     */
    std::vector<std::string> GetPathIdentifiers();

    /// Handles of all live instances created by this factory (drops destroyed ones from the list first).
    std::vector<InstanceHandle> GetGeneratedHandles();

    /// Handles and preset names of all live instances created by this factory (drops destroyed ones first).
    std::vector<std::pair<InstanceHandle, std::string>> GetGeneratedIdentifiers();

    /// True if a preset with this name or path exists.
    bool exists(std::string identifier);

    /// Saves all live instances created by this factory to `mSavefilePath`. Returns false on I/O errors.
    bool SaveObjects();

    /**
     * Saves all live marked instances (see `MarkLastCreated`).
     *
     * @param filepath Full path of the `.fac` file.
     * @return False on I/O errors.
     */
    bool SaveMarkedObjects(std::filesystem::path filepath);

    /// Creates the instances saved in `mSavefilePath`. Returns false if none were loaded.
    bool LoadObjects();

    /**
     * Creates the instances saved in a `.fac` file in the folder of `mSavefilePath` (`data/config/`).
     *
     * @param filepath File name without extension, relative to that folder.
     * @return False if none were loaded.
     */
    bool LoadObjects(std::filesystem::path filepath);

    /**
     * Like `LoadObjects(filepath)`, but also marks the created instances.
     *
     * @param filepath File name without extension, relative to `data/config/`.
     * @return The created instances.
     */
    std::vector<InstanceHandle> LoadObjectsMarked(std::filesystem::path filepath);

    virtual ~Factory() {}

private:
    /**
     * Replaces the components of a preset by copies of another instance's. No sanity checks.
     *
     * @param presetID ID of an existing preset.
     * @param templateHandle Live instance to copy the components from.
     */
    void OverwritePreset(int presetID, InstanceHandle templateHandle);

    /**
     * Writes all live created instances in `.fac` format.
     *
     * @param filepath File to write (overwritten).
     * @return False on I/O errors.
     */
    bool SaveObjectsTo(std::filesystem::path filepath);

    /**
     * Writes all live marked instances in `.fac` format.
     *
     * @param filepath File to write (overwritten).
     * @return False on I/O errors.
     */
    bool SaveMarkedObjectsTo(std::filesystem::path filepath);

    /**
     * Creates one instance per `.fac` record with the saved transform (including the saved height).
     *
     * Records with an unknown preset name are skipped (and logged).
     *
     * @param filepath `.fac` file to read.
     * @param marked Also mark the created instances (see `MarkLastCreated`).
     * @return The created instances; empty if the file cannot be opened.
     */
    std::vector<InstanceHandle> LoadObjectsFrom(std::filesystem::path filepath, bool marked = false);

    /**
     * Sets `Physics::lastPosition` to the current position of a freshly placed instance,
     * so that it does not appear to have moved from the preset position (arrow lead).
     *
     * @param handle Live instance; nothing happens without a `Physics` component.
     */
    void SettlePhysics(InstanceHandle handle);

protected:
    /**
     * Hook for subclasses, called by `Register` after the `Prototype` component is added and before the preset is added, e.g. to attach components.
     *
     * @param presetHandle The new preset instance.
     */
    virtual void PostProcessRegistration(InstanceHandle presetHandle) {};

protected:
    EngineECS* mECS = nullptr;                                                       ///< ECS (not owned).
    SharedGameObjects mObj;                                                          ///< Asset manager (meshes, textures).
    std::shared_ptr<Terrain> mT;                                                     ///< Terrain for placement, may be null.
    int mFreeID = 0;                                                                 ///< Next preset ID.
    std::pair<InstanceHandle, std::string> mLastCreated = {{std::uint32_t(-1)}, ""}; ///< Last `Create` result.

    /// Default save file (each subclass sets its own); its folder is used by `LoadObjects(path)` and `LoadObjectsMarked`.
    std::filesystem::path mSavefilePath = "../data/config/undefined.fac";

    std::unordered_map<std::string, int> mObjectNameToID; ///< Preset name -> preset ID.
    std::unordered_map<std::string, int> mObjectPathToID; ///< Preset path -> preset ID.
    std::unordered_map<int, InstanceHandle> mIDToPreset;  ///< Preset ID -> preset instance.

    std::list<std::pair<InstanceHandle, std::string>> mGeneratedObjects; ///< Instances created (not cloned) by this factory, with preset name.
    std::list<std::pair<InstanceHandle, std::string>> mMarkedObjects;    ///< Instances marked for `SaveMarkedObjects`.
};
}
