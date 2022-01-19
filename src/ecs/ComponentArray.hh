#pragma once
#include "advanced/World.hh"
#include "components/Component.hh"
#include "glow/common/log.hh"
#include <unordered_map>
#include <array>
#include <assert.h>

namespace gamedev
{
/***
* https://austinmorlan.com/posts/entity_component_system/
* 
* A Component Array holds all Components of a certain type.
* It is always packed and accessed by the InstanceHandle.
* 
***/

// Interface of different ComponentArrays.
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    virtual gamedev::Component* GetBaseComponent(InstanceHandle handle) = 0;
    virtual gamedev::Component* TryGetBaseComponent(InstanceHandle handle) = 0;
    virtual void TryRemoveData(InstanceHandle handle) = 0;
    virtual void CloneEntityComponent(InstanceHandle& handle_source, InstanceHandle& handle_copy) = 0;
    virtual void EntityDestroyed(InstanceHandle handle) = 0;
    virtual void AllEntitiesDestroyed() = 0;
};

// Actual ComponentArray for a single Component.
template <typename CompT>
class ComponentArray : public IComponentArray
{
public:
    // Creates a mapping from InstanceHandle to Array,
    // and adds the component to the array.
    void InsertData(InstanceHandle handle, CompT& component)
    {
        assert(mEntityToIndexMap.find(handle) == mEntityToIndexMap.end() && "Component added to same entity more than once.");
        
        size_t newIndex = mSize;
        //mEntityToIndexMap[handle] = newIndex;
        //mIndexToEntityMap[newIndex] = handle;
        mEntityToIndexMap.insert_or_assign(handle, newIndex);
        mIndexToEntityMap.insert_or_assign(newIndex, handle);
        mComponentArray[newIndex] = component;

        ++mSize;

        IntegrityCheck();
    }

    void TryInsertData(InstanceHandle handle, CompT& component)
    {
        if(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end())
        {
            glow::log() << "Tried to add component to same entity more than once.";
            return;
        }

        size_t newIndex = mSize;
        //mEntityToIndexMap[handle] = newIndex;
        //mIndexToEntityMap[newIndex] = handle;
        mEntityToIndexMap.insert_or_assign(handle, newIndex);
        mIndexToEntityMap.insert_or_assign(newIndex, handle);
        mComponentArray[newIndex] = component;
        ++mSize;

        IntegrityCheck();
    }

    // Removes the component from the Instance by updating the mapping into the array.
    void RemoveData(InstanceHandle handle)
    {
        assert(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end() && "Removing non-existent component.");

		    // Copy element at end into deleted element's place to maintain density
        size_t indexOfRemovedEntity = mEntityToIndexMap[handle];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        // Update map to the moved position
        InstanceHandle entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
        //mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        //mIndexToEntityMap[indexOfLastElement] = entityOfLastElement;
        mEntityToIndexMap.insert_or_assign(entityOfLastElement, indexOfRemovedEntity);
        mIndexToEntityMap.insert_or_assign(indexOfRemovedEntity, entityOfLastElement);

        // Remove empty
        mEntityToIndexMap.erase(handle);
        mIndexToEntityMap.erase(indexOfLastElement);

        if (mSize > 0)
            --mSize;

        IntegrityCheck();
    }

    // Removes the component from the Instance by updating the mapping into the array.
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
        //mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        //mIndexToEntityMap[indexOfLastElement] = entityOfLastElement;
        mEntityToIndexMap.insert_or_assign(entityOfLastElement, indexOfRemovedEntity);
        mIndexToEntityMap.insert_or_assign(indexOfRemovedEntity, entityOfLastElement);

        // Remove empty
        mEntityToIndexMap.erase(handle);
        mIndexToEntityMap.erase(indexOfLastElement);

        if (mSize > 0)
            --mSize;

        IntegrityCheck();
    }

    // Remove the component from all instances
    void RemoveAll()
    {
        mEntityToIndexMap.clear();
        mIndexToEntityMap.clear();
        mSize = 0;
    }

    // Returns a reference to the Instances component
    CompT* GetData(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) == mEntityToIndexMap.end())
        {
            glow::error() << "Retrieving non-existent component: " << typeid(CompT).name();
            assert(mEntityToIndexMap.find(handle) != mEntityToIndexMap.end());
        }

        return &mComponentArray[mEntityToIndexMap[handle]];
    }

    // Tries to return a reference to the Instance component, else it will return an empty component.
    CompT* TryGetData(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) == mEntityToIndexMap.end())
            return nullptr;

        return &mComponentArray[mEntityToIndexMap[handle]];
    }

    Component* GetBaseComponent(InstanceHandle handle)
    {
        return reinterpret_cast<Component*>(GetData(handle));
    }

    Component* TryGetBaseComponent(InstanceHandle handle)
    {
        return reinterpret_cast<Component*>(TryGetData(handle));
    }

    // Clones the component of source if it exists & sets the instance reference to nullptr (correct this before using!)
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

    // Will check if the entity had this component and destroy it.
    void EntityDestroyed(InstanceHandle handle)
    {
        if (mEntityToIndexMap.find(handle) != mEntityToIndexMap.end())
            RemoveData(handle);
    }

    // Will check if the entity had this component and destroy it.
    void AllEntitiesDestroyed()
    {
        RemoveAll();
    }

    // Checks integrity.
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
    // Densely packed array of components.
    // Maximum corresponds to the maximum of living instances.
    std::array<CompT, MAX_INSTANCES> mComponentArray;

    // Maps InstanceHandle to array index.
    std::unordered_map<InstanceHandle, size_t, InstanceHandleHash> mEntityToIndexMap;
    
    // Maps array index to InstanceHandle.
    std::unordered_map<size_t, InstanceHandle> mIndexToEntityMap;

    // Valid entries in the array.
    size_t mSize;

    CompT mDummy;
};
}