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
/// Factory for the meshes in `data/meshes/items/` (arrows, bows, ...). Saves to `data/config/items.fac`.
class ItemFactory : public Factory
{
public:
    ItemFactory() { mSavefilePath = "../data/config/items.fac"; }

    /// Configures the special item presets (`arrow`, `bow`); call after registering the item meshes.
    void InitResources();

private:
    /**
     * Gives every item preset a `Render` component.
     *
     * @param presetHandle Preset that `Factory::Register` is creating.
     */
    void PostProcessRegistration(InstanceHandle presetHandle) override;

    /// `arrow`: rendered, not selectable, with `Arrow`, `Physics` and `Animated`; `GamelogicSystem` sets up the flight.
    void Init_Arrow();

    /// `bow`: adds `Animated` (drawn/scaled when a soldier shoots).
    void Init_Bow();
};
}
