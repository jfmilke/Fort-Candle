#pragma once
#include <glow/fwd.hh>
#include <filesystem>
#include "ecs/Engine.hh"
#include "Components.hh"
#include "Factory.hh"
#include "Types.hh"
#include "GameObjects.hh"
#include <typed-geometry/tg-lean.hh>


namespace gamedev
{
/**
 * Factory for units (meshes in `data/meshes/units/`). Saves to `data/config/units.fac`.
 *
 * Unit presets are rendered, animated, dynamic colliders with physics. `InitResources` turns these presets into game
 * units:
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
     * `Factory::Register` plus a box (`boxshape`) or circle shape from the mesh bounds, a dynamic `Collider` and
     * `Physics`.
     */
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);

    /**
     * Like above, registered under `unit_identifier`. Unused.
     *
     * TODO: Adds a second `Render` component (`PostProcessRegistration` already adds one), which asserts.
     */
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, bool boxshape, float scaling = 1.0);

    /// Like above, registered under `unit_identifier` and `unit_path`. Unused (TODO: same issue as above).
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, std::filesystem::path unit_path, bool boxshape, float scaling = 1.0);

    /// `Register`s every asset with the same scaling and shape type.
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    /**
     * Configures the unit presets used by the game (replacing their default components); call after registering the
     * unit meshes.
     */
    void InitResources();

private:
    /// Gives every unit preset `Render` (selectable) and `Animated`.
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
