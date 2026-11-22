#pragma once
#include <glow/fwd.hh>

#include "ecs/ComponentManager.hh"
#include "ecs/SystemManager.hh"
#include "ecs/EventManager.hh"
#include "advanced/World.hh"
#include "Components.hh"
#include "utility/HashMap.hh"
#include <array>
#include "glow/common/log.hh"

namespace gamedev
{
GLOW_SHARED(class, EngineECS);

/**
 * Facade of the entity component system.
 *
 * The one object the rest of the code uses for entities, components, systems, events and the spatial hash map.
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 *
 * Ownership:
 *  - Created by `Game::init()` and shared (as `SharedEngineECS`) with every system and factory.
 *  - Owns the `ComponentManager`, `SystemManager` and `EventManager`.
 *  - Wraps the `World` that stores the instances (owned by `AdvancedFeatures`).
 *
 * Every function that changes a component set also updates the instance signature and re-evaluates the system
 * membership (`UpdateSystems`).
 *
 * TODO: Systems and factories hold `shared_ptr`s back to this object, so it is never destroyed (reference cycle).
 */
class EngineECS
{
public:
    /**
     * Creates the managers and wraps `world`, which must outlive this object.
     *
     * TODO: `world` is wrapped in an owning `shared_ptr` although `AdvancedFeatures` owns it. This is only harmless
     * because this object is never destroyed.
     */
    void Init(World& world)
    {
        mSystemManager = std::make_unique<SystemManager>();
        mComponentManager = std::make_unique<ComponentManager>();
        mEventManager = std::make_unique<EventManager>();
        mInstanceManager = std::shared_ptr<World>(&world);
    }

    // Entity Management (world)
    // -------------------------

    /// Creates an instance with the given mesh and textures, an identity transform, no components and an empty signature.
    [[nodiscard]] InstanceHandle CreateInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm)
    {
        return mInstanceManager->createInstance(vao, albedo, normal, arm);
    }

    /**
     * Creates an instance that shares mesh and textures with `source_handle` and copies its albedo bias, transform and
     * bounds. Components are not copied (see `CloneComponents`).
     */
    [[nodiscard]] InstanceHandle CloneInstance(InstanceHandle source_handle)
    {
        auto& source_instance = GetInstance(source_handle);
        auto copy_handle = mInstanceManager->createInstance(source_instance.vao, source_instance.texAlbedo, source_instance.texNormal, source_instance.texARM);
        auto& copy_instance = GetInstance(copy_handle);

        copy_instance.albedoBias = source_instance.albedoBias;
        copy_instance.xform = source_instance.xform;
        copy_instance.max_bounds = source_instance.max_bounds;

        return copy_handle;
    }

    /// Returns the instance data of a live handle.
    Instance& GetInstance(InstanceHandle handle) { return mInstanceManager->getInstance(handle); }

    /// Returns the world transform of a live handle (for rendering and gameplay positions).
    transform& GetInstanceTransform(InstanceHandle handle) { return mInstanceManager->getInstance(handle).xform; }

    /// Number of live instances, including prototypes.
    uint32_t getNumLiveInstances() const { return mInstanceManager->getNumLiveInstances(); }

    /// Flags `handle` for destruction; `CleanupSystem` destroys it at the end of the current update.
    void MarkDestruction(InstanceHandle handle)
    {
        mInstanceManager->getInstance(handle).destroy = true;
    }

    /**
     * Immediately frees `handle` and removes it from all components, systems and the spatial hash map.
     * Prefer `MarkDestruction` while systems iterate.
     */
    void DestroyInstance(InstanceHandle handle)
    {
        mInstanceManager->destroyInstance(handle);
        InstanceDestroyed(handle);
    }

    /// Removes `handle` from all components, systems and the spatial hash map, without freeing the instance itself.
    void InstanceDestroyed(InstanceHandle handle)
    {
        mComponentManager->EntityDestroyed(handle);
        mSystemManager->EntityDestroyed(handle);
        mSpatialHashMap.RemoveInstance(handle);
    }

    /// Clears all components, systems and the spatial hash map after the `World` was emptied by the caller.
    void AllInstancesDestroyed()
    {
        mComponentManager->AllEntitiesDestroyed();
        mSystemManager->AllEntitiesDestroyed();
        mSpatialHashMap.Clear();
    }

    /// True if `handle` refers to a live instance.
    bool IsLiveHandle(InstanceHandle handle) const { return mInstanceManager->isLiveHandle(handle); }

    // Component Management
    // --------------------

    /// Registers component type `CompT`. Must happen before any other use of `CompT`.
    template <typename CompT>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<CompT>();
    }

    /**
     * Adds a new `CompT` to `handle`, updates its signature and systems, and returns the component.
     * Asserts if `handle` already has a `CompT`.
     */
    template <typename CompT>
    CompT* CreateComponent(InstanceHandle handle)
    {
        auto Component = mComponentManager->CreateComponent<CompT>(handle, mInstanceManager->getInstance(handle));

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);

        return Component;
    }

    /// Like `CreateComponent`, but if `handle` already has a `CompT` it is kept and returned.
    template <typename CompT>
    CompT* TryCreateComponent(InstanceHandle handle)
    {
        auto Component = mComponentManager->TryCreateComponent<CompT>(handle, mInstanceManager->getInstance(handle));

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);

        return Component;
    }

    /**
     * Adds a copy of `component` to `handle` and updates its signature and systems. The copy's `instance` pointer is
     * taken from `component` as is. Asserts if `handle` already has a `CompT`.
     */
    template <typename CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        mComponentManager->AddComponent<CompT>(handle, component);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);
    }

    /// Copies all components and the signature of `source_handle` to `copy_handle` and updates its systems.
    void CloneComponents(InstanceHandle source_handle, InstanceHandle copy_handle)
    {
        auto& source_instance = GetInstance(source_handle);
        auto& copy_instance = GetInstance(copy_handle);

        mComponentManager->CloneEntityComponents(source_instance, copy_instance);

        mInstanceManager->SetSignature(copy_handle, source_instance.mSignature);

        UpdateSystems(copy_handle);
    }

    /// Removes `handle`'s `CompT` and updates its signature and systems. Asserts if it has none.
    template <typename CompT>
    void RemoveComponent(InstanceHandle handle)
    {
        mComponentManager->RemoveComponent<CompT>(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), false);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);
    }

    /// Like `RemoveComponent`, but does nothing to the components if `handle` has no `CompT`.
    template <typename CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        mComponentManager->TryRemoveComponent<CompT>(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), false);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);
    }

    /// Removes all of `handle`'s components, clears its signature and removes it from all systems.
    void RemoveAllComponents(InstanceHandle handle)
    {
        mComponentManager->RemoveAllComponents(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.reset();
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);
    }

    /// Returns `handle`'s `CompT`. Asserts if it has none. Do not keep the pointer across component removals.
    template <typename CompT>
    CompT* GetComponent(InstanceHandle handle)
    {
        return mComponentManager->GetComponent<CompT>(handle);
    }

    /// Returns `handle`'s `CompT`, or nullptr if it has none. Do not keep the pointer across component removals.
    template <typename CompT>
    CompT* TryGetComponent(InstanceHandle handle)
    {
        return mComponentManager->TryGetComponent<CompT>(handle);
    }

    /// Returns the signature bit of `CompT`, e.g. to build system signatures.
    template <typename CompT>
    ComponentType GetComponentType()
    {
        return mComponentManager->GetComponentType<CompT>();
    }

    /// True if `handle`'s signature contains `CompT`.
    template <typename CompT>
    bool TestSignature(InstanceHandle handle)
    {
        return mInstanceManager->getInstance(handle).mSignature.test(mComponentManager->GetComponentType<CompT>());
    }

    // System Management
    // -----------------

    /**
     * Adds `handle` to all systems whose signature it matches and removes it from the others. `Prototype` instances are
     * removed from all systems instead.
     */
    void UpdateSystems(InstanceHandle handle)
    {
        // Don't add template/prototype entities to systems (and remove them from any system if they are part of any)
        if (TestSignature<gamedev::Prototype>(handle))
        {
            mSystemManager->EntityDestroyed(handle);
            return;
        }

        mSystemManager->EntitySignatureChanged(handle, GetInstance(handle).mSignature);
    }

    /// Creates system `SysT` (owned by the `SystemManager`) and returns it. Set its signature before adding entities.
    template <typename SysT>
    std::shared_ptr<SysT> RegisterSystem()
    {
        return mSystemManager->RegisterSystem<SysT>();
    }

    /// Sets the components an entity needs to be part of `SysT`. Only the first call per system has an effect.
    template <typename SysT>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<SysT>(signature);
    }

    /// Direct access to the instance storage.
    World* GetInstanceManager() { return mInstanceManager.get(); }

    /// Direct access to the system manager.
    SystemManager* GetSystemManager() { return mSystemManager.get(); }

    /// Direct access to the component manager.
    ComponentManager* GetComponentManager() { return mComponentManager.get(); }

    // Event Management
    // ----------------

    /// See `EventManager::AddFunctionalListener`; use with `FUNCTIONAL_LISTENER`.
    void AddFunctionalListener(EventType type, std::function<void(Event&)> const&listener)
    {
        mEventManager->AddFunctionalListener(type, listener);
    }

    /// Calls all listeners of `event.mType` synchronously.
    void SendFunctionalEvent(Event& event)
    {
        mEventManager->SendFunctionalEvent(event);
    }

    /// Queues `event` for manual handling via `GetPendingEvents`.
    void SendEvent(Event& event)
    {
        mEventManager->SendEvent(event);
    }

    /// Events queued with `SendEvent`.
    std::vector<Event>& GetPendingEvents() { return mEventManager->GetPendingEvents(); }

    /**
     * Spatial hash of instances on the ground plane for neighbourhood queries (e.g. targets in range, free spawn spots).
     * Factories add created instances and `PhysicsSystem` updates moved ones.
     */
    Hash2D& GetHashMap() { return mSpatialHashMap; }


private:
    std::shared_ptr<World> mInstanceManager;             ///< Instance storage (see the note on `Init`).
    std::unique_ptr<SystemManager> mSystemManager;       ///< Systems and their signatures.
    std::unique_ptr<ComponentManager> mComponentManager; ///< Component types and arrays.
    std::unique_ptr<EventManager> mEventManager;         ///< Event listeners and queue.

private:
    Hash2D mSpatialHashMap; ///< See `GetHashMap`; instances are removed when destroyed.
};
}
