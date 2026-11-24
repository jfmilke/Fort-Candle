#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"
#include "Terrain.hh"

#include <array>

namespace gamedev
{
/**
 * Performs the deferred destruction of instances flagged with `EngineECS::MarkDestruction`.
 *
 *  - Registered without a signature, so it sees every non-prototype instance.
 *  - `Game::update()` runs it last, after all other systems, so that no system iterates over an instance while it is destroyed.
 */
class CleanupSystem : public System
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    /**
     * Destroys all instances whose `Instance::destroy` flag is set.
     *
     * They are removed from components, systems and the spatial hash map.
     * Marked prototypes are not in this system and stay alive.
     */
    void Update();

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).
};
}
