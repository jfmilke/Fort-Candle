#pragma once
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
 * Factory for buildings and other structures (meshes in `data/meshes/structures/`).
 *
 * Saves to `data/config/structures.fac`; the start layout and the supply route upgrades are other `.fac` files in that folder.
 *
 * Structure presets are static colliders.
 * `InitResources` then gives the presets used by the game their special components (lights, particles, health, tower slots, ...).
 */
class StructureFactory : public Factory
{
public:
    StructureFactory() { mSavefilePath = "../data/config/structures.fac"; }

    /**
     * Registers a structure preset: `Factory::Register` plus a shape from the mesh bounds and a static `Collider`.
     *
     * @param asset_identifier Mesh name or path known to the asset manager; also the preset name.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);

    /**
     * Like above, but registers the preset under another name (used for `starting_bonfire`).
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param structure_identifier Preset name.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, bool boxshape, float scaling = 1.0);

    /**
     * Like above, but registers the preset under a name and a path. Unused.
     *
     * @param asset_identifier Mesh name or path known to the asset manager.
     * @param structure_identifier Preset name.
     * @param structure_path Preset path.
     * @param boxshape Use a `BoxShape` if true, else a `CircleShape`.
     * @param scaling Uniform scale of the preset.
     * @return The preset, or an invalid handle if the asset is unknown.
     */
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, std::filesystem::path structure_path, bool boxshape, float scaling = 1.0);

    /**
     * Registers a structure preset for every asset, all with the same scale and shape type.
     *
     * @param asset_identifiers Mesh names or paths known to the asset manager.
     * @param global_scaling Uniform scale of all presets.
     * @param global_boxshape Use `BoxShape`s if true, else `CircleShape`s.
     */
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    /**
     * Configures the structure presets used by the game (replacing their default components); call after registering the structure meshes.
     *
     * Also creates the `starting_bonfire` preset.
     */
    void InitResources();

private:
    /**
     * Gives every structure preset `Render` and `Destructible` (10 health).
     *
     * @param presetHandle Preset that `Factory::Register` is creating.
     */
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    // Each Init_* function replaces the components of one preset (or a group of presets).

    /// `bonfire`: small fire with a flickering light and fire/ember particles.
    void Init_RegularBonfire();

    /// `starting_bonfire` (from the `bonfire` mesh): the large home fire; its light is the shadow-casting point light.
    void Init_StartingBonfire();

    /// `forge`: destructible, with fire and smoke particles.
    void Init_Forge();

    /// `astreetlantern`: destructible lantern with a point light.
    void Init_aLantern();

    /// `tower0`: destructible tower (10 health) with a circle shape.
    void Init_Tower();

    /// Wooden fences (`afence*`): destructible walls (10 health).
    void Init_aFences();

    /// `astonewall`: destructible wall (20 health).
    void Init_aStonewall();

    /// `astonegate`: destructible gate without collider.
    void Init_aStoneGate();

    /// `astonepillar`: destructible pillar (20 health).
    void Init_aStonePillar();

    /// `aspikebarriers`: destructible barrier (5 health).
    void Init_aSpikeBarriers();

    /// `awagon`: destructible wagon without collider.
    void Init_aWagon();

    /// `abanner`: destructible banner without collider.
    void Init_aBanner();

    /// Stone and wood roads: render only.
    void Init_Streets();

    /// Watch towers (`awatchtower*`): static colliders with a `Tower` slot for a soldier.
    void Init_WatchTowers();

    /// `astonefence0`: static collider, not destructible.
    void Init_Stonefence();

    /// Sets the scale of the tent, tower and wall presets to 0.35 (`Init_Tower` then sets `tower0` to 0.25).
    void Init_Rescales();
};
}
