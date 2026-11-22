#pragma once
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/ComponentManager.hh"
#include <unordered_map>

namespace gamedev
{
/**
 * Owns all registered systems and their signatures.
 *
 * Keeps each system's entity set in sync with the entity signatures.
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 *  - Owned by `EngineECS`.
 *  - Like the `ComponentManager`, it keys systems by the `typeid(SysT).name()` pointer.
 */
class SystemManager
{
public:
    /// Creates and stores a system of type `SysT` and returns a shared pointer to it. Asserts if registered twice.
    template <typename SysT>
    std::shared_ptr<SysT> RegisterSystem()
    {
        const char* typeName = typeid(SysT).name();

        assert(mSystems.find(typeName) == mSystems.end() && "Registering system more than once.");

        auto system = std::make_shared<SysT>();
        mSystems.insert({typeName, system});
        return system;
    }

    /**
     * Sets the component signature that entities need to be added to system `SysT`. Only the first call per system has an
     * effect (`insert` does not overwrite). Does not re-check existing entities.
     */
    template <typename SysT>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(SysT).name();

        assert(mSystems.find(typeName) != mSystems.end() && "System used before registered.");

        mSignatures.insert({typeName, signature});
    }

    /// Removes `handle` from every system.
    void EntityDestroyed(InstanceHandle handle)
    {
        for (const auto& pair : mSystems)
        {
            const auto& system = pair.second;

            system->RemoveEntity(handle);
        }
    }

    /// Removes all entities from every system.
    void AllEntitiesDestroyed()
    {
        for (const auto& pair : mSystems)
        {
            const auto& system = pair.second;

            system->RemoveAllEntities();
        }
    }

    /// Adds `handle` to every system whose signature is contained in `entitySignature` and removes it from all others.
    void EntitySignatureChanged(InstanceHandle handle, Signature entitySignature)
    {
        for (const auto& pair : mSystems)
        {
            const auto& type = pair.first;
            const auto& system = pair.second;
            const auto& systemSignature = mSignatures[type];

            if ((entitySignature & systemSignature) == systemSignature)
            {
                system->AddEntity(handle, entitySignature);
            }
            else
            {
                system->RemoveEntity(handle, entitySignature);
            }
        }
    }

private:
    std::unordered_map<const char*, Signature> mSignatures{};            ///< System type name -> required signature.
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems{}; ///< System type name -> system.
};
}
