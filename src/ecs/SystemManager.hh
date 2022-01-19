#pragma once
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/ComponentManager.hh"
#include <unordered_map>

namespace gamedev
{
/***
 * https://austinmorlan.com/posts/entity_component_system/
 *
 * The System Manager registers System Types at runtime.
 * It also is in charge of maintaining a record of registered Systems and their Signatures.
 *
 * Fun Facts:
 *    - Like the ComponentManager again the typeid(..).name() pointer is used as a unique key to map to the systems.
 *
 ***/

class SystemManager
{
public:
    // Registers a new System Type.
    template <typename SysT>
    std::shared_ptr<SysT> RegisterSystem()
    {
        const char* typeName = typeid(SysT).name();

        assert(mSystems.find(typeName) == mSystems.end() && "Registering system more than once.");

        auto system = std::make_shared<SysT>();
        mSystems.insert({typeName, system});
        return system;
    }

    // Defines the Signature for a System Type.
    template <typename SysT>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(SysT).name();

        assert(mSystems.find(typeName) != mSystems.end() && "System used before registered.");

        mSignatures.insert({typeName, signature});
    }

    // Removes the destroyed Entity from all Systems if necessary.
    void EntityDestroyed(InstanceHandle handle)
    {
        for (const auto& pair : mSystems)
        {
            const auto& system = pair.second;

            system->RemoveEntity(handle);
        }
    }

    // Notifies all systems, that all entities have been destroyed.
    void AllEntitiesDestroyed()
    {
        for (const auto& pair : mSystems)
        {
            const auto& system = pair.second;

            system->RemoveAllEntities();
        }
    }

    // Checks the (new) Entity Signature against all System Signatures to add or remove them from Systems.
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
    // Map typeid(..).name() to a Signature
    std::unordered_map<const char*, Signature> mSignatures{};
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems{};
};
}