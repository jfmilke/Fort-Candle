#pragma once
#include "advanced/World.hh"
#include "components/Component.hh"
#include "glow/common/log.hh"
#include <clean-core/assert.hh>
#include <array>
#include <cstdint>
#include <typeinfo>
#include <utility>

namespace gamedev
{
/// Type-erased interface of `ComponentArray`, so the `ComponentManager` can notify the arrays of all component types.
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;

    /// `ComponentArray::GetData` as a pointer to the `Component` base. Unused.
    virtual gamedev::Component* GetBaseComponent(InstanceHandle handle) = 0;

    /// `ComponentArray::TryGetData` as a pointer to the `Component` base. Unused.
    virtual gamedev::Component* TryGetBaseComponent(InstanceHandle handle) = 0;

    /// See `ComponentArray::TryRemoveData`.
    virtual void TryRemoveData(InstanceHandle handle) = 0;

    /// See `ComponentArray::CloneEntityComponent`.
    virtual void CloneEntityComponent(InstanceHandle handle_source, InstanceHandle handle_copy) = 0;

    /// See `ComponentArray::EntityDestroyed`.
    virtual void EntityDestroyed(InstanceHandle handle) = 0;

    /// See `ComponentArray::AllEntitiesDestroyed`.
    virtual void AllEntitiesDestroyed() = 0;
};

/**
 * Stores all components of type `CompT` densely packed and maps instance handles to them (a sparse set).
 *
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 * Created and owned by the `ComponentManager` when `CompT` is registered; accessed through `EngineECS`.
 *
 * Layout:
 *  - `mComponents[0, mSize)` holds the components, `mOwners[i]` the instance that owns `mComponents[i]`.
 *  - `mSlotToIndex[slot]` maps an instance's stable `World` slot to its index in `mComponents` (`kNoIndex` if it has none).
 *  - All lookups are two array reads and a handle compare; nothing allocates after construction.
 *
 * Removing a component moves the last element into the gap.
 * A pointer returned by `GetData` may therefore refer to another entity's component after any removal of this type.
 *
 * Lookups need the owner to be alive in the `World`: remove an instance's components before the `World` frees it (see `EngineECS::DestroyInstance`).
 * The asserts fire in Debug and RelWithDebInfo (`CC_ASSERT_MSG`).
 */
template <EcsComponent CompT>
class ComponentArray final : public IComponentArray
{
public:
    /**
     * Creates an empty array.
     *
     * @param world Instance storage used to map handles to slots; must outlive this array.
     */
    explicit ComponentArray(World const& world) : mWorld(world)
    {
        CC_ASSERT_MSG(world.getMaxNumInstances() <= MAX_INSTANCES, "World slots would overflow the component arrays.");
        mSlotToIndex.fill(kNoIndex);
    }

    /**
     * Stores a component in the next free slot. Asserts if the instance already has a component of this type.
     *
     * @param handle Live instance the component belongs to.
     * @param component Component to store.
     */
    void InsertData(InstanceHandle handle, CompT component)
    {
        auto const slot = mWorld.SlotIndex(handle);
        CC_ASSERT_MSG(slot != World::kNoSlot, "Adding a component to a dead instance.");
        CC_ASSERT_MSG(mSlotToIndex[slot] == kNoIndex, "Component added to same entity more than once.");

        Append(handle, slot, std::move(component));
    }

    /**
     * Like `InsertData`, but if the instance already has a component of this type it only logs and keeps that one.
     *
     * @param handle Live instance the component belongs to.
     * @param component Component to store.
     */
    void TryInsertData(InstanceHandle handle, CompT component)
    {
        auto const slot = mWorld.SlotIndex(handle);
        CC_ASSERT_MSG(slot != World::kNoSlot, "Adding a component to a dead instance.");

        if (mSlotToIndex[slot] != kNoIndex)
        {
            glow::log() << "Tried to add component to same entity more than once.";
            return;
        }

        Append(handle, slot, std::move(component));
    }

    /**
     * Removes a component and moves the last element into its slot. Asserts if there is none.
     *
     * @param handle Instance whose component is removed.
     */
    void RemoveData(InstanceHandle handle)
    {
        auto const index = IndexOf(handle);
        CC_ASSERT_MSG(index != kNoIndex, "Removing non-existent component.");

        if (index != kNoIndex)
            Erase(index);
    }

    /**
     * Like `RemoveData`, but does nothing if there is no component of this type.
     *
     * @param handle Instance whose component is removed.
     */
    void TryRemoveData(InstanceHandle handle) override
    {
        if (auto const index = IndexOf(handle); index != kNoIndex)
            Erase(index);
    }

    /// Removes the components of all instances; works after the `World` was emptied.
    void RemoveAll()
    {
        for (std::size_t i = 0; i < mSize; ++i)
        {
            mSlotToIndex[mOwners[i].slot] = kNoIndex;
            mComponents[i] = CompT{};
        }
        mSize = 0;
    }

    /**
     * Returns `handle`'s component. Logs an error and asserts if `handle` has none.
     *
     * @return The component, or nullptr if there is none (only reached in Release, where asserts are off).
     */
    CompT* GetData(InstanceHandle handle)
    {
        auto const index = IndexOf(handle);
        if (index == kNoIndex)
        {
            glow::error() << "Retrieving non-existent component: " << typeid(CompT).name();
            CC_ASSERT_MSG(false, "Retrieving non-existent component.");
            return nullptr;
        }

        return &mComponents[index];
    }

    /// Returns `handle`'s component, or nullptr if it has none (or `handle` is dead).
    CompT* TryGetData(InstanceHandle handle)
    {
        auto const index = IndexOf(handle);
        return index != kNoIndex ? &mComponents[index] : nullptr;
    }

    /// `GetData` as a pointer to the `Component` base. Unused.
    Component* GetBaseComponent(InstanceHandle handle) override { return GetData(handle); }

    /// `TryGetData` as a pointer to the `Component` base. Unused.
    Component* TryGetBaseComponent(InstanceHandle handle) override { return TryGetData(handle); }

    /**
     * Copies a component from one instance to another, overwriting an existing one. Sets the copy's `handle_value`.
     *
     * @param handle_source Instance to copy from; nothing happens if it has no component of this type.
     * @param handle_copy Live instance to copy to.
     */
    void CloneEntityComponent(InstanceHandle handle_source, InstanceHandle handle_copy) override
    {
        auto const source = IndexOf(handle_source);
        if (source == kNoIndex)
            return;

        CompT copy = mComponents[source];
        copy.handle_value = handle_copy._value;

        if (auto const target = IndexOf(handle_copy); target != kNoIndex)
            mComponents[target] = std::move(copy);
        else
            InsertData(handle_copy, std::move(copy));
    }

    /**
     * Removes the component of an instance that is being destroyed, if it has one.
     *
     * @param handle Instance that is still alive in the `World`.
     */
    void EntityDestroyed(InstanceHandle handle) override { TryRemoveData(handle); }

    /// Removes the components of all instances (`RemoveAll`).
    void AllEntitiesDestroyed() override { RemoveAll(); }

private:
    using Index = std::uint16_t;
    static constexpr Index kNoIndex = Index(-1);
    static_assert(MAX_INSTANCES < kNoIndex, "Index type too small for MAX_INSTANCES.");

    /// Owner of a stored component; the slot is kept so that `Erase` and `RemoveAll` need no `World` lookup.
    struct Owner
    {
        InstanceHandle handle;
        std::uint32_t slot = World::kNoSlot;
    };

    /// Index of `handle`'s component in `mComponents`, or `kNoIndex`.
    Index IndexOf(InstanceHandle handle) const
    {
        auto const slot = mWorld.SlotIndex(handle);
        if (slot == World::kNoSlot)
            return kNoIndex;

        // The owner check guards against a stale entry if a slot was reused without removing the old components.
        auto const index = mSlotToIndex[slot];
        return (index != kNoIndex && mOwners[index].handle == handle) ? index : kNoIndex;
    }

    /// Stores `component` at the end of the dense range.
    void Append(InstanceHandle handle, std::uint32_t slot, CompT&& component)
    {
        CC_ASSERT_MSG(mSize < MAX_INSTANCES, "Component array full.");

        auto const index = Index(mSize++);
        mComponents[index] = std::move(component);
        mOwners[index] = {handle, slot};
        mSlotToIndex[slot] = index;
    }

    /// Removes the component at `index`: the last component moves into the gap, the vacated slot is reset.
    void Erase(Index index)
    {
        auto const last = Index(mSize - 1);
        mSlotToIndex[mOwners[index].slot] = kNoIndex;

        if (index != last)
        {
            mComponents[index] = std::move(mComponents[last]);
            mOwners[index] = mOwners[last];
            mSlotToIndex[mOwners[index].slot] = index;
        }

        // Release what the moved-from component still holds (vectors, sounds, ...)
        mComponents[last] = CompT{};
        mOwners[last] = {};
        --mSize;
    }

    World const& mWorld;                           ///< Maps handles to slots.
    std::array<CompT, MAX_INSTANCES> mComponents;  ///< Dense storage; [0, mSize) are in use.
    std::array<Owner, MAX_INSTANCES> mOwners;      ///< Dense index -> owning instance.
    std::array<Index, MAX_INSTANCES> mSlotToIndex; ///< `World` slot -> dense index, or `kNoIndex`.
    std::size_t mSize = 0;                         ///< Number of stored components.
    CompT mDummy;                                  ///< Unused.
};
}
