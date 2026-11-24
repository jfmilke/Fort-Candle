#pragma once
#include <array>
#include <clean-core/assert.hh>
#include <memory>
#include "advanced/World.hh"
#include "ecs/ComponentArray.hh"

namespace gamedev
{
/**
 * Registers component types at runtime and owns one `ComponentArray` per type.
 *
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 *  - Owned by `EngineECS`, which also updates the instance signatures; this class only stores the component data.
 *  - Each type gets a `ComponentType` ID (bit index into a `Signature`) on first use; it indexes `mComponentArrays` directly.
 *    IDs are process-wide (shared by all managers), so different managers may register different subsets.
 */
class ComponentManager
{
public:
    /**
     * Creates a manager without registered types.
     *
     * @param world Instance storage, passed on to every `ComponentArray`; must outlive this manager.
     */
    explicit ComponentManager(World const& world) : mWorld(world) {}

    /// Registers `CompT`: creates its array. Asserts if registered twice.
    template <EcsComponent CompT>
    void RegisterComponent()
    {
        auto& array = mComponentArrays[TypeId<CompT>()];
        CC_ASSERT_MSG(!array, "Registering component type more than once.");

        array = std::make_unique<ComponentArray<CompT>>(mWorld);
    }

    /// Returns the ID (signature bit) of `CompT`. Asserts if `CompT` is not registered.
    template <EcsComponent CompT>
    ComponentType GetComponentType() const
    {
        auto const type = TypeId<CompT>();
        CC_ASSERT_MSG(mComponentArrays[type], "Component not registered before use.");

        return type;
    }

    /**
     * Adds a default `CompT` to an instance. Asserts if the instance already has a `CompT`.
     *
     * Does not touch the signature; use `EngineECS::CreateComponent`, which does.
     *
     * @param handle Live instance to add the component to.
     * @return The new component, valid until the next removal of a `CompT`.
     */
    template <EcsComponent CompT>
    CompT* CreateComponent(InstanceHandle handle)
    {
        CompT component;
        component.handle_value = handle._value;

        auto& array = GetComponentArray<CompT>();
        array.InsertData(handle, std::move(component));
        return array.GetData(handle);
    }

    /**
     * Like `CreateComponent`, but if the instance already has a `CompT` it is kept and returned.
     *
     * @param handle Live instance to add the component to.
     * @return The new or existing component, valid until the next removal of a `CompT`.
     */
    template <EcsComponent CompT>
    CompT* TryCreateComponent(InstanceHandle handle)
    {
        CompT component;
        component.handle_value = handle._value;

        auto& array = GetComponentArray<CompT>();
        array.TryInsertData(handle, std::move(component));
        return array.GetData(handle);
    }

    /**
     * Adds a copy of a component to an instance. Asserts if the instance already has a `CompT`. Unused.
     *
     * @param handle Live instance to add the component to.
     * @param component Component to copy; the copy gets `handle`'s `handle_value`.
     */
    template <EcsComponent CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        component.handle_value = handle._value;
        GetComponentArray<CompT>().InsertData(handle, std::move(component));
    }

    /**
     * Removes an instance's `CompT`. Asserts if it has none.
     *
     * @param handle Instance to remove the component from.
     */
    template <EcsComponent CompT>
    void RemoveComponent(InstanceHandle handle)
    {
        GetComponentArray<CompT>().RemoveData(handle);
    }

    /**
     * Removes an instance's `CompT` if it has one.
     *
     * @param handle Instance to remove the component from.
     */
    template <EcsComponent CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        GetComponentArray<CompT>().TryRemoveData(handle);
    }

    /**
     * Removes all components of an instance.
     *
     * @param handle Instance to remove the components from.
     */
    void RemoveAllComponents(InstanceHandle handle)
    {
        for (auto const& array : mComponentArrays)
            if (array)
                array->TryRemoveData(handle);
    }

    /// Returns `handle`'s `CompT`. Asserts if it has none.
    template <EcsComponent CompT>
    CompT* GetComponent(InstanceHandle handle)
    {
        return GetComponentArray<CompT>().GetData(handle);
    }

    /// Returns `handle`'s `CompT`, or nullptr if it has none.
    template <EcsComponent CompT>
    CompT* TryGetComponent(InstanceHandle handle)
    {
        return GetComponentArray<CompT>().TryGetData(handle);
    }

    /**
     * Removes the components of an instance that is being destroyed from all component arrays.
     *
     * @param handle Instance that is still alive in the `World`.
     */
    void EntityDestroyed(InstanceHandle handle)
    {
        for (auto const& array : mComponentArrays)
            if (array)
                array->EntityDestroyed(handle);
    }

    /// Clears all component arrays.
    void AllEntitiesDestroyed()
    {
        for (auto const& array : mComponentArrays)
            if (array)
                array->AllEntitiesDestroyed();
    }

    /**
     * Copies every component of one instance to another, overwriting existing ones.
     *
     * Does not update the copy's signature.
     *
     * @param handle_source Live instance to copy from.
     * @param handle_copy Live instance to copy to.
     */
    void CloneEntityComponents(InstanceHandle handle_source, InstanceHandle handle_copy)
    {
        for (auto const& array : mComponentArrays)
            if (array)
                array->CloneEntityComponent(handle_source, handle_copy);
    }

private:
    World const& mWorld;                                                           ///< Passed to new arrays.
    std::array<std::unique_ptr<IComponentArray>, MAX_COMPONENTS> mComponentArrays; ///< Type ID -> array (null: not registered).

    /// Next free ID for `TypeId`.
    static inline ComponentType sNextTypeId = 0;

    /// Process-wide ID of `CompT`, assigned on first use (the function-local static exists once per type).
    template <EcsComponent CompT>
    static ComponentType TypeId()
    {
        static ComponentType const id = []
        {
            CC_ASSERT_MSG(sNextTypeId < MAX_COMPONENTS, "More component types than MAX_COMPONENTS.");
            return sNextTypeId++;
        }();
        return id;
    }

    /// Returns the array of `CompT`, cast to its concrete type. Asserts if `CompT` is not registered.
    template <EcsComponent CompT>
    ComponentArray<CompT>& GetComponentArray()
    {
        auto const& array = mComponentArrays[GetComponentType<CompT>()];
        return static_cast<ComponentArray<CompT>&>(*array);
    }
};
}
