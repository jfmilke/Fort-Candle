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
/// Factory for the decorative meshes in `data/meshes/nature/` (trees, rocks, grass). Saves to `data/config/nature.fac`.
class NatureFactory : public Factory
{
public:
    NatureFactory() { mSavefilePath = "../data/config/nature.fac"; }

private:
    /**
     * Gives every nature preset a `Render` component (no collision).
     *
     * @param presetHandle Preset that `Factory::Register` is creating.
     */
    void PostProcessRegistration(InstanceHandle presetHandle) override;
};
}
