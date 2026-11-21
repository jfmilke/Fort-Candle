#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
/**
 * Empty system skeleton (template for new systems).
 *
 * Not registered by `Game`, so it never receives entities. To use a system like this:
 *
 *  1. Register it and set its signature in `Game::init()` (see `System`).
 *  2. Call `Init`, and call `Update` from `Game::update()`.
 *  3. `AddEntity`/`RemoveEntity` are called by the `SystemManager`. Keep their signatures, but their bodies may store
 *     the handles in custom containers (always remove an entity when asked).
 *  4. To access components, use `mECS->TryGetComponent<T>(handle)` (nullptr if missing), or
 *     `mECS->TestSignature<T>(handle)` followed by `mECS->GetComponent<T>(handle)`.
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

    /// Main update; does nothing.
    void Update(float dt);

private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.
};
}
