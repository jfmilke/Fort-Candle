#pragma once
#include "ecs/ComponentArray.hh"
#include "advanced/World.hh"
#include <unordered_map>
#include <assert.h>

namespace gamedev
{
/***
* https://austinmorlan.com/posts/entity_component_system/
* 
* The Component Manager registers Component Types at runtime.
* It also does all the talking to Component Arrays when a Component needs to be added or removed from an Entity.
* It will automatically create a unique Signature for a Component Type.
* 
* Fun Fact:
*    - The integer representation of the typeid(..).name() pointer is used as a unique key to map Component to ComponentType.
*    - The Signature is a bitset which allows for easy bitwise comparison to check if an entity has certain Component Types.
* 
***/
class ComponentManager
{
public:
    // Register the component struct (once) to the manager.
    template <typename CompT>
    void RegisterComponent()
    {
      const char* typeName = typeid(CompT).name();

      assert(mComponentTypes.find(typeName) == mComponentTypes.end() && "Registering component type more than once.");

      // Add ComponentType to map
      mComponentTypes.insert({typeName, mNextComponentType});
      // Create ComponentType pool and add it to map
      mComponentArrays.insert({typeName, std::make_shared<ComponentArray<CompT>>()});
      // Update free ComponentType bit
      ++mNextComponentType;
    }

    // Return the components type id
    template <typename CompT>
    ComponentType GetComponentType()
    {
        const char* typeName = typeid(CompT).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        return mComponentTypes[typeName];
    }

    // Add a fresh component to an instance
    template <typename CompT>
    CompT* CreateComponent(InstanceHandle handle, Instance& instance)
    {
        CompT Component(&instance);
        Component.handle_value = handle._value;

        auto CompArray = GetComponentArray<CompT>();
        CompArray->InsertData(handle, Component);
        return CompArray->GetData(handle);
    }

    // Add a fresh component to an instance
    template <typename CompT>
    CompT* TryCreateComponent(InstanceHandle handle, Instance& instance)
    {
        CompT Component(&instance);
        Component.handle_value = handle._value;

        auto CompArray = GetComponentArray<CompT>();
        CompArray->TryInsertData(handle, Component);
        return CompArray->GetData(handle);
    }

    // Add an existing component to an instance
    template <typename CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        component.handle_value = handle._value;
        GetComponentArray<CompT>()->InsertData(handle, component);
    }

    // Remove a component from an instance
    template <typename CompT>
    void RemoveComponent(InstanceHandle handle) { GetComponentArray<CompT>()->RemoveData(handle); }

    template <typename CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        GetComponentArray<CompT>()->TryRemoveData(handle);
    }

    void RemoveAllComponents(InstanceHandle handle)
    {
        for (auto& arr : mComponentArrays)
        {
            arr.second->TryRemoveData(handle);
        }
    }

    // Get reference to a component from an instance
    template <typename CompT>
    CompT* GetComponent(InstanceHandle handle) { return GetComponentArray<CompT>()->GetData(handle); }

    template <typename CompT>
    CompT* TryGetComponent(InstanceHandle handle) { return GetComponentArray<CompT>()->TryGetData(handle); }

    // Notifies all component arrays that an entity has been destroyed.
    // If necessary, all component arrays will delete the entity from their mapping.
    void EntityDestroyed(InstanceHandle handle)
    {
        for (const auto& pair : mComponentArrays)
        {
            const auto& component = pair.second;

            component->EntityDestroyed(handle);
        }
    }

    // Notifies all component arrays, that all entities have been destroyed.
    void AllEntitiesDestroyed()
    {
        for (const auto& pair : mComponentArrays)
        {
            const auto& component = pair.second;

            component->AllEntitiesDestroyed();
        }
    }

    // Clones all components from the source instance & corrects the Instance reference of those components.
    void CloneEntityComponents(Instance& instance_source, Instance& instance_copy)
    {
        for (const auto& cArray : mComponentArrays)
        {
            cArray.second->CloneEntityComponent(instance_source.mHandle, instance_copy.mHandle);
            auto clone = cArray.second->TryGetBaseComponent(instance_copy.mHandle);

            if (clone)
                clone->instance = &instance_copy;
        }
    }

private:
    // Map typeid(..).name() to component type
    std::unordered_map<const char*, ComponentType> mComponentTypes{};
    // Map typeid(..).name() to component array
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> mComponentArrays{};
    // ComponentType to be assigned for the next registered component
    ComponentType mNextComponentType{};

    // Convenience: Cast component array to correct component type
    template <typename CompT>
    std::shared_ptr<ComponentArray<CompT>> GetComponentArray()
    {
        const char* typeName = typeid(CompT).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        return std::static_pointer_cast<ComponentArray<CompT>>(mComponentArrays[typeName]);
    }
};
}