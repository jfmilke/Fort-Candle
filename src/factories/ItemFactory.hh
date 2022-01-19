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
class ItemFactory : public Factory
{
public:
    ItemFactory() { mSavefilePath = "../data/config/items.fac"; }

    void InitResources();

private:
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    void Init_Arrow();
    void Init_Bow();
};
}
