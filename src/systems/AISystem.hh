#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
/**
 * Empty system skeleton, kept as a template for new systems.
 *
 * Unused: `Game` does not register it.
 *
 * To turn a copy into a working system, register and initialize it as shown in `System`, and call its `Update` from `Game::update()`.
 * Then:
 *  - Keep the signatures of `AddEntity`/`RemoveEntity` (the `SystemManager` calls them).
 *    Their bodies may store the handles in custom containers (always remove an entity when asked).
 *  - To access components, use `mECS->TryGetComponent<T>(handle)` (nullptr if missing),
 *    or `mECS->TestSignature<T>(handle)` followed by `mECS->GetComponent<T>(handle)`.
 */
class AISystem : public System
{
public:
    /// Stores the ECS (and other systems it needs).
    void Init(std::shared_ptr<EngineECS>& ecs);

    /// Called when an entity's signature starts matching the system signature.
    void AddEntity(InstanceHandle& handle, Signature entitySignature);

    /// Called when an entity's signature no longer matches.
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);

    /// Called when an entity is destroyed.
    void RemoveEntity(InstanceHandle& handle);

    /// Called when all entities are destroyed.
    void RemoveAllEntities();

    /**
     * Main update; does nothing.
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.
};
}
