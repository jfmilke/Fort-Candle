#pragma once
#include "advanced/World.hh"
#include "components/Component.hh"
#include "glow/common/log.hh"
#include <unordered_map>
#include <array>
#include <assert.h>

namespace gamedev
{
/// Type-erased interface of `ComponentArray`, so the `ComponentManager` can notify the arrays of all component types.
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;

    /// `ComponentArray::GetData` as a pointer to the `Component` base.
    virtual gamedev::Component* GetBaseComponent(InstanceHandle handle) = 0;

    /// `ComponentArray::TryGetData` as a pointer to the `Component` base.
    virtual gamedev::Component* TryGetBaseComponent(InstanceHandle handle) = 0;

    /// See `ComponentArray::TryRemoveData`.
    virtual void TryRemoveData(InstanceHandle handle) = 0;

    /// See `ComponentArray::CloneEntityComponent`.
    virtual void CloneEntityComponent(InstanceHandle& handle_source, InstanceHandle& handle_copy) = 0;

    /// See `ComponentArray::EntityDestroyed`.
    virtual void EntityDestroyed(InstanceHandle handle) = 0;

    /// See `ComponentArray::AllEntitiesDestroyed`.
    virtual void AllEntitiesDestroyed() = 0;
};

/**
 * Stores all components of type `CompT`, densely packed in a fixed-size array, and maps instance handles to slots.
 *
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 * Created and owned by the `ComponentManager` when `CompT` is registered; accessed through `EngineECS`.
 *
 * Removing a component moves the last element into the gap.
 * A pointer returned by `GetData` may therefore refer to another entity's component after any removal of this type.
 *
 * The asserts only fire in Debug builds; RelWithDebInfo and Release define `NDEBUG`.
 */
template <typename CompT>
class ComponentArray : public IComponentArray
{
public:
    /**
     * Stores a copy of a component in the next free slot. Asserts if the instance already has a component of this type.
     *
     * @param handle Instance the component belongs to.
     * @param component Component to copy.
     */
    void InsertData(InstanceHandle handle, CompT& component)
    {
        assert(mEntityToIndexMap.find(handle) == mEntityToIndexMap.end() && "Component added to same entity more than once.");

        size_t newIndex = mSize;
        mEntityToIndexMap.insert_or_assign(handle, newIndex);
        mIndexToEntityMap.insert_or_assign(newIndex, handle);
        mComponentArray[newIndex] = component;

        ++mSize;

        IntegrityCheck();
    }

    /**
     * Like `InsertData`, but if the instance already has a component of this type it only logs and keeps that one.
     *
     * @param handle Instance the component belongs to.
     * @param component Component to copy.
     */
    void TryInsertData(InstanceHandle handle, CompT& component)
    {
        if(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end())
        {
            glow::log() << "Tried to add component to same entity more than once.";
            return;
        }

        size_t newIndex = mSize;
        mEntityToIndexMap.insert_or_assign(handle, newIndex);
        mIndexToEntityMap.insert_or_assign(newIndex, handle);
        mComponentArray[newIndex] = component;
        ++mSize;

        IntegrityCheck();
    }

    /**
     * Removes a component and moves the last element into its slot. Asserts if there is none.
     *
     * @param handle Instance whose component is removed.
     */
    void RemoveData(InstanceHandle handle)
    {
        assert(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end() && "Removing non-existent component.");

        // Copy element at end into deleted element's place to maintain density
        size_t indexOfRemovedEntity = mEntityToIndexMap[handle];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        // Update map to the moved position
        InstanceHandle entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
        mEntityToIndexMap.insert_or_assign(entityOfLastElement, indexOfRemovedEntity);
        mIndexToEntityMap.insert_or_assign(indexOfRemovedEntity, entityOfLastElement);

        // Remove empty
        mEntityToIndexMap.erase(handle);
        mIndexToEntityMap.erase(indexOfLastElement);

        if (mSize > 0)
            --mSize;

        IntegrityCheck();
    }

    /**
     * Like `RemoveData`, but does nothing if there is no component of this type.
     *
     * @param handle Instance whose component is removed.
     */
    void TryRemoveData(InstanceHandle handle)
    {
        if(mEntityToIndexMap.find(handle) == mEntityToIndexMap.end())
        {
            return;
        }

        // Copy element at end into deleted element's place to maintain density
        size_t indexOfRemovedEntity = mEntityToIndexMap[handle];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        // Update map to the moved position
        InstanceHandle entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
        mEntityToIndexMap.insert_or_assign(entityOfLastElement, indexOfRemovedEntity);
        mIndexToEntityMap.insert_or_assign(indexOfRemovedEntity, entityOfLastElement);

        // Remove empty
        mEntityToIndexMap.erase(handle);
        mIndexToEntityMap.erase(indexOfLastElement);

        if (mSize > 0)
            --mSize;

        IntegrityCheck();
    }

    /// Forgets the components of all instances. The stored objects are not destroyed until overwritten.
    void RemoveAll()
    {
        mEntityToIndexMap.clear();
        mIndexToEntityMap.clear();
        mSize = 0;
    }

    /**
     * Returns `handle`'s component. Logs an error and asserts if `handle` has none.
     *
     * TODO: Without asserts (RelWithDebInfo, Release), a missing component returns slot 0, i.e. another instance's component,
     * and `operator[]` adds a wrong map entry for `handle`.
     */
    CompT* GetData(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) == mEntityToIndexMap.end())
        {
            glow::error() << "Retrieving non-existent component: " << typeid(CompT).name();
            assert(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end());
        }

        return &mComponentArray[mEntityToIndexMap[handle]];
    }

    /// Returns `handle`'s component, or nullptr if it has none.
    CompT* TryGetData(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) == mEntityToIndexMap.end())
            return nullptr;

        return &mComponentArray[mEntityToIndexMap[handle]];
    }

    /// `GetData` as a pointer to the `Component` base.
    Component* GetBaseComponent(InstanceHandle handle)
    {
        return reinterpret_cast<Component*>(GetData(handle));
    }

    /// `TryGetData` as a pointer to the `Component` base.
    Component* TryGetBaseComponent(InstanceHandle handle)
    {
        return reinterpret_cast<Component*>(TryGetData(handle));
    }

    /**
     * Copies a component from one instance to another, overwriting an existing one.
     *
     * Sets the copy's `handle_value`.
     * The copy's `instance` pointer is set to nullptr; the caller must set it before use.
     *
     * @param handle_source Instance to copy from; nothing happens if it has no component of this type.
     * @param handle_copy Instance to copy to.
     */
    void CloneEntityComponent(InstanceHandle& handle_source, InstanceHandle& handle_copy)
    {
        if (mEntityToIndexMap.find(handle_source) == mEntityToIndexMap.end())
            return;

        if (mEntityToIndexMap.find(handle_copy) != mEntityToIndexMap.end())
        {
            mComponentArray[mEntityToIndexMap[handle_copy]] = mComponentArray[mEntityToIndexMap[handle_source]];
            mComponentArray[mEntityToIndexMap[handle_copy]].instance = nullptr;
            mComponentArray[mEntityToIndexMap[handle_copy]].handle_value = handle_copy._value;
        }
        else
        {
            InsertData(handle_copy, mComponentArray[mEntityToIndexMap[handle_source]]);
            mComponentArray[mEntityToIndexMap[handle_copy]].instance = nullptr;
            mComponentArray[mEntityToIndexMap[handle_copy]].handle_value = handle_copy._value;
        }

        GetBaseComponent(handle_copy)->instance = nullptr;
    }

    /**
     * Removes the component of a destroyed instance, if it has one.
     *
     * @param handle Destroyed instance.
     */
    void EntityDestroyed(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) != mEntityToIndexMap.end())
            RemoveData(handle);
    }

    /// Removes the components of all instances (`RemoveAll`).
    void AllEntitiesDestroyed()
    {
        RemoveAll();
    }

    /**
     * Debugging aid, called after every insertion and removal.
     *
     * Set breakpoints on the `int i = 0;` lines to catch duplicate slots or handles.
     *
     * TODO: Walks all slot pairs (O(n^2) per insertion/removal) but checks nothing, because the assert is commented out.
     * Remove it or enable the check in debug builds only.
     */
    void IntegrityCheck()
    {
        for (const auto& i1 : mIndexToEntityMap)
        {
            for (const auto& i2 : mIndexToEntityMap)
            {
                if ((i1.first == i2.first) && (i1.second._value == i2.second._value))
                    continue;

                if (i1.first == i2.first)
                    int i = 0;

                if (i1.second._value == i2.second._value)
                    int i = 0;
        
                //assert((i1.second._value != i2.second._value)) && "Double id");
            }
        }
    }

private:
    std::array<CompT, MAX_INSTANCES> mComponentArray;                                 ///< Slots [0, mSize) are in use.
    std::unordered_map<InstanceHandle, size_t, InstanceHandleHash> mEntityToIndexMap; ///< Instance handle -> slot.
    std::unordered_map<size_t, InstanceHandle> mIndexToEntityMap;                     ///< Slot -> instance handle.
    size_t mSize;                                                                     ///< Number of used slots.
    CompT mDummy;                                                                     ///< Unused.
};
}