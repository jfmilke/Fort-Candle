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
class UnitFactory : public Factory
{
public:
    UnitFactory() { mSavefilePath = "../data/config/units.fac"; }

    // Special registration methods to use for structures (see Factory.hh for more info)
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, bool boxshape, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string unit_identifier, std::filesystem::path unit_path, bool boxshape, float scaling = 1.0);

    // Special auto registration methods to use for structures (see Factory.hh for more info)
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    // Assign special (in case something got default settings, those will be removed prior)
    void InitResources();

private:
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    void Init_Monster();
    void Init_Person();
    void Init_Artisan();
    void Init_Soldier();
};
}
