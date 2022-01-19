#pragma once
#include <glow/fwd.hh>
#include "ecs/Engine.hh"
#include "Components.hh"
#include "Types.hh"
#include "GameObjects.hh"
#include "Factory.hh"
#include <typed-geometry/tg-lean.hh>

/* Structure Factory
 *
 * See Factory.hh for more information.
 * Used to manage any instance related to structures.
 * 
 */

namespace gamedev
{
class StructureFactory : public Factory
{
public:
    StructureFactory() { mSavefilePath = "../data/config/structures.fac"; }

    // Special registration methods to use for structures (see Factory.hh for more info)
    InstanceHandle Register(std::string asset_identifier, bool boxshape, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, bool boxshape, float scaling = 1.0);
    InstanceHandle Register(std::string asset_identifier, std::string structure_identifier, std::filesystem::path structure_path, bool boxshape, float scaling = 1.0);

    // Special auto registration methods to use for structures (see Factory.hh for more info)
    void RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape);

    // Assign special (in case something got default settings, those will be removed prior)
    void InitResources();

private:
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    // Assign special (in case something got default settings, those will be removed prior)
    void Init_RegularBonfire();
    void Init_StartingBonfire();
    void Init_Forge();
    //void Init_aStoneWall();
    //void Init_aStonePillar();
    //void Init_aStoneGate();
    void Init_aLantern();
    void Init_Tower();
    void Init_aFences();
    void Init_aStonewall();
    void Init_aStoneGate();
    void Init_aStonePillar();
    void Init_aSpikeBarriers();
    void Init_aWagon();
    void Init_aBanner();
    void Init_Streets();
    void Init_WatchTowers();
    void Init_Stonefence();

    void Init_Rescales();
};
}
