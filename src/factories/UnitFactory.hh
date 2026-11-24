#pragma once
#include <filesystem>
#include <glow/fwd.hh>
#include <typed-geometry/tg-lean.hh>
#include "Components.hh"
#include "Factory.hh"
#include "GameObjects.hh"
#include "Types.hh"
#include "ecs/Engine.hh"


namespace gamedev
{
/**
 * Factory for units (meshes in `data/meshes/units/`). Saves to `data/config/units.fac`.
 *
 * Unit presets are rendered, animated, dynamic colliders with physics.
 * `InitResources` turns these presets into game units (box shape, scale 0.25):
 *  - `creature`: monster,
 *  - `person`: pioneer without a job,
 *  - `person_artisan`: produces gold,
 *  - `person_soldier`: archer.
 */
class UnitFactory : public Factory
{
public:
    UnitFactory() { mSavefilePath = "../data/config/units.fac"; }

    /**
     * Registers a unit preset: `Factory::Register` plus a shape from the mesh bounds, a dynamic `Collider` and `Physics`.
     *
     * @param asset_identifier Mesh name or path known to the asset manager; also the preset name.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);

    /**
     * Like above, but registers the preset under another name. Unused.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param unit_identifier Preset name.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, bool boxshape, float scaling = 1.0);

    /**
     * Like above, but registers the preset under a name and a path. Unused.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param unit_identifier Preset name.
     * @param unit_path Preset path.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, std::filesystem::path unit_path, bool boxshape, float scaling = 1.0);

    /**
     * Registers a unit preset for every asset, all with the same scale and shape type.
     *
     * @param asset_identifiers Mesh names or paths known to the asset manager.
     * @param global_scaling Uniform scale of all presets.
     * @param global_boxshape Use `BoxShape`s if true, else `CircleShape`s.
     */
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    /// Configures the unit presets used by the game (replacing their default components); call after registering the unit meshes.
    void InitResources();

private:
    /**
     * Gives every unit preset `Render` (selectable) and `Animated`.
     *
     * @param presetHandle Preset that `Factory::Register` is creating.
     */
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    /// `creature`: hostile melee monster (claws), targetable but not selectable.
    void Init_Monster();

    /// `person`: friendly pioneer without a job.
    void Init_Person();

    /// `person_artisan`: friendly pioneer that produces gold.
    void Init_Artisan();

    /// `person_soldier`: friendly archer that can man towers.
    void Init_Soldier();
};
}
