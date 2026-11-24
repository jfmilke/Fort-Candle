#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

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
    void Init(EngineECS& ecs);

    /// Called when an entity's signature starts matching the system signature.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;

    /// Called when an entity's signature no longer matches, and when it is destroyed or becomes a prototype.
    void RemoveEntity(InstanceHandle handle) override;

    /// Called when all entities are destroyed.
    void RemoveAllEntities() override;

    /**
     * Main update; does nothing.
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).
};
}
