#pragma once
#include "advanced/World.hh"
#include "Constants.hh"
#include <chrono>
#include <concepts>
#include <set>

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
 *  - Whenever an entity's signature changes, the `SystemManager` calls `AddEntity` if the entity's signature contains the system's signature,
 *    and `RemoveEntity` otherwise (whether or not the entity was in the system before).
 *  - A system without a signature receives every entity.
 *  - `Prototype` entities are never added (see `EngineECS::UpdateSystems`).
 *  - By default the qualifying handles are kept in `mEntities`.
 *    Override `AddEntity`/`RemoveEntity`/`RemoveAllEntities` to store them in custom containers instead.
 *  - Systems may create, change and mark entities for destruction while iterating `mEntities`:
 *    `std::set` iterators survive insertions and the erasure of other elements.
 *    Do not destroy the entity at the current iterator (use `EngineECS::MarkDestruction`).
 *
 * Registering a new system in `Game::init()` (the components must be registered first):
 * @code
 * mCoolSys = mECS->RegisterSystem<gamedev::CoolSystem, gamedev::SpecialComponent>(); // signature: SpecialComponent
 * mCoolSys->Init(*mECS);
 * @endcode
 */
class System
{
public:
    /**
     * Called when an entity's new signature matches the system signature. Adding an entity twice is harmless.
     *
     * @param handle Entity to add.
     * @param entitySignature The entity's new signature.
     */
    virtual void AddEntity(InstanceHandle handle, Signature entitySignature) { mEntities.insert(handle); }

    /**
     * Called when an entity's new signature does not match the system signature, and when it is destroyed or becomes a prototype.
     *
     * @param handle Entity to remove; it may not be in the system.
     */
    virtual void RemoveEntity(InstanceHandle handle) { mEntities.erase(handle); }

    /// Called when all instances are destroyed at once.
    virtual void RemoveAllEntities() { mEntities.clear(); }

    virtual ~System() = default;

    std::set<InstanceHandle> mEntities; ///< Handles of all qualifying entities, ordered by handle value.
};

/// What `EngineECS::RegisterSystem` requires of a system type.
template <class T>
concept EcsSystem = std::derived_from<T, System> && std::default_initializable<T>;
}
