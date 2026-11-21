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
 *  - `Game::update()` runs it last, after all other systems, so that no system iterates over an instance while it is
 *    destroyed.
 */
class CleanupSystem : public System
{
public:
    /// Stores the ECS.
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /**
     * Destroys all instances whose `Instance::destroy` flag is set (removing them from components, systems and the
     * spatial hash map). Returns the elapsed time in microseconds.
     */
    int Update();

private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.
};
}
