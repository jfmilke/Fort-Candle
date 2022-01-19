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
class NatureFactory : public Factory
{
public:
    NatureFactory() { mSavefilePath = "../data/config/nature.fac"; }

private:
    void PostProcessRegistration(InstanceHandle presetHandle) override;
};
}