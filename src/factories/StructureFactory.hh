#pragma once
#include <glow/fwd.hh>
#include "ecs/Engine.hh"
#include "Components.hh"
#include "Types.hh"
#include "GameObjects.hh"
#include "Factory.hh"
#include <typed-geometry/tg-lean.hh>

namespace gamedev
{
/**
 * Factory for buildings and other structures (meshes in `data/meshes/structures/`).
 *
 * Saves to `data/config/structures.fac`; the start layout and the supply route upgrades are other `.fac` files in
 * that folder.
 *
 * Structure presets are static colliders. `InitResources` then gives the presets used by the game their special
 * components (lights, particles, health, tower slots, ...).
 */
class StructureFactory : public Factory
{
public:
    StructureFactory() { mSavefilePath = "../data/config/structures.fac"; }

    /// `Factory::Register` plus a static `Collider` with a box (`boxshape`) or circle shape from the mesh bounds.
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);

    /// Like above, registered under `structure_identifier`.
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, bool boxshape, float scaling = 1.0);

    /// Like above, registered under `structure_identifier` and `structure_path`.
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, std::filesystem::path structure_path, bool boxshape, float scaling = 1.0);

    /// `Register`s every asset with the same scaling and shape type.
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    /**
     * Configures the structure presets used by the game (replacing their default components); call after registering
     * the structure meshes. Also creates the `starting_bonfire` preset.
     */
    void InitResources();

private:
    /// Gives every structure preset `Render` and `Destructible` (10 health).
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    // Each Init_* function replaces the components of one preset (or a group of presets).

    /// `bonfire`: small fire with a flickering light and fire/ember particles.
    void Init_RegularBonfire();

    /// `starting_bonfire`: the large home fire; its light is the shadow-casting point light.
    void Init_StartingBonfire();

    /// `forge`: destructible, with fire and smoke particles.
    void Init_Forge();

    /// `astreetlantern`: destructible lantern with a point light.
    void Init_aLantern();

    /// `tower0`: destructible tower with a circle shape.
    void Init_Tower();

    /// Wooden fences (`afence*`): destructible walls.
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

    /// Sets the scale of the tent, tower and wall presets to 0.35.
    void Init_Rescales();
};
}
