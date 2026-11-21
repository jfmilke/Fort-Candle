#pragma once
#include "advanced/World.hh"
#include "Constants.hh"
#include <set>
#include <chrono>

namespace gamedev
{
/**
 * Base class of all ECS systems.
 *
 * A system is a piece of functionality that iterates over the entities with a certain set of components.
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 * Lifetime:
 *  - Created by `EngineECS::RegisterSystem` and owned by the `SystemManager`.
 *  - `Game` keeps a second `shared_ptr` to call `Init` and `Update`.
 *
 * Membership:
 *  - Whenever an entity's signature changes, the `SystemManager` calls `AddEntity` if the entity's signature contains
 *    the system's signature, and `RemoveEntity` otherwise (whether or not the entity was in the system before).
 *  - A system without a signature receives every entity.
 *  - `Prototype` entities are never added (see `EngineECS::UpdateSystems`).
 *  - By default the qualifying handles are kept in `mEntities`. Override `AddEntity`/`RemoveEntity` (same
 *    signatures) to store them in custom containers instead.
 *
 * Registering a new system in `Game::init()`:
 * @code
 * mCoolSys = mECS->RegisterSystem<gamedev::CoolSystem>();
 *
 * Signature signature;
 * signature.set(mECS->GetComponentType<gamedev::SpecialComponent>()); // component must be registered first
 * mECS->SetSystemSignature<gamedev::CoolSystem>(signature);
 *
 * mCoolSys->Init(mECS);
 * @endcode
 */
class System
{
public:
    /// Called when `handle`'s signature (`entitySignature`) matches the system signature. Adding twice is harmless.
    virtual void AddEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.insert(handle); };

    /// Called when `handle`'s new signature no longer matches; the entity may not have been in the system.
    virtual void RemoveEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.erase(handle); };

    /// Called when `handle` is destroyed or becomes a prototype; the entity may not have been in the system.
    virtual void RemoveEntity(InstanceHandle& handle) { mEntities.erase(handle); };

    /// Called when all instances are destroyed at once.
    virtual void RemoveAllEntities() { mEntities.clear(); }

    virtual ~System() {}

    std::set<InstanceHandle> mEntities; ///< Handles of all qualifying entities, ordered by handle value.
};
}
