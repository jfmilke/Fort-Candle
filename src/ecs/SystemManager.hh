#pragma once
#include <clean-core/assert.hh>
#include <memory>
#include <typeindex>
#include <vector>
#include "advanced/World.hh"
#include "ecs/System.hh"

namespace gamedev
{
/**
 * Owns all registered systems and their signatures.
 *
 * Keeps each system's entity set in sync with the entity signatures.
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 *  - Owned by `EngineECS`; destroying it destroys the systems (unless `Game` still holds them).
 *  - Systems are kept in registration order, which is also the order in which they are notified.
 */
class SystemManager
{
public:
    /// Creates and stores a system of type `SysT` with an empty signature and returns it. Asserts if registered twice.
    template <EcsSystem SysT>
    std::shared_ptr<SysT> RegisterSystem()
    {
        CC_ASSERT_MSG(!Find<SysT>(), "Registering system more than once.");

        auto system = std::make_shared<SysT>();
        mSystems.push_back({typeid(SysT), system, {}});
        return system;
    }

    /**
     * Sets the component signature that entities need to be added to system `SysT`.
     *
     * Existing entities are not re-checked, so set it before entities with matching components are created.
     *
     * @param signature Components an entity needs; an empty signature matches every entity.
     */
    template <EcsSystem SysT>
    void SetSignature(Signature signature)
    {
        auto entry = Find<SysT>();
        CC_ASSERT_MSG(entry, "System used before registered.");

        if (entry)
            entry->signature = signature;
    }

    /**
     * Removes an entity from every system.
     *
     * @param handle Entity that is destroyed or became a prototype.
     */
    void EntityDestroyed(InstanceHandle handle)
    {
        for (auto const& entry : mSystems)
            entry.system->RemoveEntity(handle);
    }

    /// Removes all entities from every system.
    void AllEntitiesDestroyed()
    {
        for (auto const& entry : mSystems)
            entry.system->RemoveAllEntities();
    }

    /**
     * Adds an entity to every system whose signature is contained in the entity's signature and removes it from all others.
     *
     * @param handle Entity whose signature changed.
     * @param entitySignature The entity's new signature.
     */
    void EntitySignatureChanged(InstanceHandle handle, Signature entitySignature)
    {
        for (auto const& entry : mSystems)
        {
            if ((entitySignature & entry.signature) == entry.signature)
                entry.system->AddEntity(handle, entitySignature);
            else
                entry.system->RemoveEntity(handle);
        }
    }

private:
    /// A registered system with the components it requires.
    struct Entry
    {
        std::type_index type;           ///< Type of `system`, to find it by type.
        std::shared_ptr<System> system; ///< The system.
        Signature signature;            ///< Components an entity needs to be part of `system`.
    };

    std::vector<Entry> mSystems; ///< In registration order; a handful of entries, so lookups are linear.

    /// Entry of `SysT`, or nullptr if it is not registered.
    template <EcsSystem SysT>
    Entry* Find()
    {
        for (auto& entry : mSystems)
            if (entry.type == typeid(SysT))
                return &entry;
        return nullptr;
    }
};
}
