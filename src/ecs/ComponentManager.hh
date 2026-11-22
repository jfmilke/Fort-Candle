#pragma once
#include "ecs/ComponentArray.hh"
#include "advanced/World.hh"
#include <unordered_map>
#include <assert.h>

namespace gamedev
{
/**
 * Registers component types at runtime and owns one `ComponentArray` per type.
 *
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 *  - Owned by `EngineECS`, which also updates the instance signatures; this class only stores the component data.
 *  - Types are keyed by the `typeid(CompT).name()` pointer.
 *  - Types get consecutive `ComponentType` IDs (bit indices into a `Signature`) in registration order.
 */
class ComponentManager
{
public:
    /// Registers `CompT`: assigns the next `ComponentType` ID and creates its array. Asserts if registered twice.
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

    /// Returns the ID (signature bit) of `CompT`. Asserts if `CompT` is not registered.
    template <typename CompT>
    ComponentType GetComponentType()
    {
        const char* typeName = typeid(CompT).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        return mComponentTypes[typeName];
    }

    /**
     * Adds a default `CompT` to an instance. Asserts if the instance already has a `CompT`.
     *
     * Does not touch the signature; use `EngineECS::CreateComponent`, which does.
     *
     * @param handle Instance to add the component to.
     * @param instance Data of `handle`; becomes the component's `instance` pointer.
     * @return The new component, valid until the next removal of a `CompT`.
     */
    template <typename CompT>
    CompT* CreateComponent(InstanceHandle handle, Instance& instance)
    {
        CompT Component(&instance);
        Component.handle_value = handle._value;

        auto CompArray = GetComponentArray<CompT>();
        CompArray->InsertData(handle, Component);
        return CompArray->GetData(handle);
    }

    /**
     * Like `CreateComponent`, but if the instance already has a `CompT` it is kept and returned.
     *
     * @param handle Instance to add the component to.
     * @param instance Data of `handle`; becomes the new component's `instance` pointer.
     * @return The new or existing component, valid until the next removal of a `CompT`.
     */
    template <typename CompT>
    CompT* TryCreateComponent(InstanceHandle handle, Instance& instance)
    {
        CompT Component(&instance);
        Component.handle_value = handle._value;

        auto CompArray = GetComponentArray<CompT>();
        CompArray->TryInsertData(handle, Component);
        return CompArray->GetData(handle);
    }

    /**
     * Adds a copy of a component to an instance. Asserts if the instance already has a `CompT`. Unused.
     *
     * @param handle Instance to add the component to.
     * @param component Component to copy; the copy gets `handle`'s `handle_value` and keeps the `instance` pointer.
     */
    template <typename CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        component.handle_value = handle._value;
        GetComponentArray<CompT>()->InsertData(handle, component);
    }

    /**
     * Removes an instance's `CompT`. Asserts if it has none.
     *
     * @param handle Instance to remove the component from.
     */
    template <typename CompT>
    void RemoveComponent(InstanceHandle handle) { GetComponentArray<CompT>()->RemoveData(handle); }

    /**
     * Removes an instance's `CompT` if it has one.
     *
     * @param handle Instance to remove the component from.
     */
    template <typename CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        GetComponentArray<CompT>()->TryRemoveData(handle);
    }

    /**
     * Removes all components of an instance.
     *
     * @param handle Instance to remove the components from.
     */
    void RemoveAllComponents(InstanceHandle handle)
    {
        for (auto& arr : mComponentArrays)
        {
            arr.second->TryRemoveData(handle);
        }
    }

    /// Returns `handle`'s `CompT`. Asserts if it has none.
    template <typename CompT>
    CompT* GetComponent(InstanceHandle handle) { return GetComponentArray<CompT>()->GetData(handle); }

    /// Returns `handle`'s `CompT`, or nullptr if it has none.
    template <typename CompT>
    CompT* TryGetComponent(InstanceHandle handle) { return GetComponentArray<CompT>()->TryGetData(handle); }

    /**
     * Removes a destroyed instance's components from all component arrays.
     *
     * @param handle Destroyed instance.
     */
    void EntityDestroyed(InstanceHandle handle)
    {
        for (const auto& pair : mComponentArrays)
        {
            const auto& component = pair.second;

            component->EntityDestroyed(handle);
        }
    }

    /// Clears all component arrays.
    void AllEntitiesDestroyed()
    {
        for (const auto& pair : mComponentArrays)
        {
            const auto& component = pair.second;

            component->AllEntitiesDestroyed();
        }
    }

    /**
     * Copies every component of one instance to another, overwriting existing ones.
     *
     * Does not update the copy's signature.
     *
     * @param instance_source Instance to copy from.
     * @param instance_copy Instance to copy to; becomes the copies' `instance` pointer.
     */
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
    std::unordered_map<const char*, ComponentType> mComponentTypes{};                     ///< Type name -> type ID.
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> mComponentArrays{}; ///< Type name -> array.
    ComponentType mNextComponentType{};                                                   ///< ID for the next registered type.

    /// Returns the array of `CompT`, cast to its concrete type. Asserts if `CompT` is not registered.
    template <typename CompT>
    std::shared_ptr<ComponentArray<CompT>> GetComponentArray()
    {
        const char* typeName = typeid(CompT).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        return std::static_pointer_cast<ComponentArray<CompT>>(mComponentArrays[typeName]);
    }
};
}