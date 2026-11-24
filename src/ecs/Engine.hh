#pragma once
#include <glow/fwd.hh>

#include <memory>
#include "Components.hh"
#include "advanced/World.hh"
#include "ecs/ComponentManager.hh"
#include "ecs/EventManager.hh"
#include "ecs/SystemManager.hh"
#include "glow/common/log.hh"
#include "utility/HashMap.hh"

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
 *  - Created and owned by `Game` (`Game::init()`); systems and factories keep a non-owning pointer.
 *  - Owns the `ComponentManager`, `SystemManager` and `EventManager`, and through the `SystemManager` the systems.
 *  - Uses the `World` that stores the instances (owned by `AdvancedFeatures`), which must outlive this object.
 *  - `Game::onClose()` destroys it while the GL context still exists, because the systems own GL resources.
 *
 * Every function that changes a component set also updates the instance signature and re-evaluates the system membership (`UpdateSystems`).
 */
class EngineECS
{
public:
    /**
     * Creates the managers and stores the instance storage.
     *
     * @param world Instance storage; not owned, must outlive this object.
     */
    void Init(World& world)
    {
        mInstanceManager = &world;
        mSystemManager = std::make_unique<SystemManager>();
        mComponentManager = std::make_unique<ComponentManager>(world);
        mEventManager = std::make_unique<EventManager>();
    }

    // Entity Management (world)
    // -------------------------

    /**
     * Creates an instance with an identity transform, no components and an empty signature (see `World::createInstance`).
     *
     * The instance is not added to the spatial hash map; the factories do that.
     *
     * @param vao Main mesh.
     * @param albedo Base color texture.
     * @param normal Normal map (the factories pass nullptr).
     * @param arm Ambient occlusion + roughness + metallic texture (the factories pass nullptr).
     * @return Handle of the new instance.
     */
    [[nodiscard]] InstanceHandle CreateInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm)
    {
        return mInstanceManager->createInstance(vao, albedo, normal, arm);
    }

    /**
     * Creates an instance that shares mesh and textures with another one and copies its albedo bias, transform and bounds.
     *
     * Components are not copied (see `CloneComponents`), and the copy is not added to the spatial hash map.
     *
     * @param source_handle Live instance to copy.
     * @return Handle of the copy.
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
    [[nodiscard]] Instance& GetInstance(InstanceHandle handle) { return mInstanceManager->getInstance(handle); }

    /// Returns the world transform of a live handle (for rendering and gameplay positions).
    [[nodiscard]] transform& GetInstanceTransform(InstanceHandle handle) { return mInstanceManager->getInstance(handle).xform; }

    /// Number of live instances, including prototypes.
    [[nodiscard]] uint32_t getNumLiveInstances() const { return mInstanceManager->getNumLiveInstances(); }

    /**
     * Flags an instance for destruction; `CleanupSystem` destroys it at the end of the current update.
     *
     * @param handle Live instance to destroy.
     */
    void MarkDestruction(InstanceHandle handle) { mInstanceManager->getInstance(handle).destroy = true; }

    /**
     * Immediately removes an instance from all systems, the spatial hash map and all components, then frees it.
     *
     * Prefer `MarkDestruction` while systems iterate.
     *
     * @param handle Live instance to destroy.
     */
    void DestroyInstance(InstanceHandle handle)
    {
        // Components are found through the instance's slot, so detach everything before the World frees it
        InstanceDestroyed(handle);
        mInstanceManager->destroyInstance(handle);
    }

    /**
     * Removes an instance from all systems, the spatial hash map and all components, without freeing the instance itself.
     *
     * @param handle Instance that is still alive in the `World` (the caller frees it afterwards).
     */
    void InstanceDestroyed(InstanceHandle handle)
    {
        mSystemManager->EntityDestroyed(handle);
        mSpatialHashMap.RemoveInstance(handle);
        mComponentManager->EntityDestroyed(handle);
    }

    /// Clears all components, systems and the spatial hash map after the `World` was emptied by the caller.
    void AllInstancesDestroyed()
    {
        mComponentManager->AllEntitiesDestroyed();
        mSystemManager->AllEntitiesDestroyed();
        mSpatialHashMap.Clear();
    }

    /// True if `handle` refers to a live instance.
    [[nodiscard]] bool IsLiveHandle(InstanceHandle handle) const { return mInstanceManager->isLiveHandle(handle); }

    // Component Management
    // --------------------

    /// Registers component type `CompT`. Must happen before any other use of `CompT`.
    template <EcsComponent CompT>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<CompT>();
    }

    /**
     * Adds a default `CompT` to an instance and updates its signature and systems.
     *
     * Asserts if the instance already has a `CompT`.
     *
     * @param handle Live instance to add the component to.
     * @return The new component, valid until the next removal of a `CompT`.
     */
    template <EcsComponent CompT>
    CompT* CreateComponent(InstanceHandle handle)
    {
        auto component = mComponentManager->CreateComponent<CompT>(handle);
        SetSignatureBit<CompT>(handle, true);
        return component;
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
        auto component = mComponentManager->TryCreateComponent<CompT>(handle);
        SetSignatureBit<CompT>(handle, true);
        return component;
    }

    /**
     * Adds a copy of a component to an instance and updates its signature and systems. Unused.
     *
     * Asserts if the instance already has a `CompT`.
     *
     * @param handle Live instance to add the component to.
     * @param component Component to copy.
     */
    template <EcsComponent CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        mComponentManager->AddComponent<CompT>(handle, std::move(component));
        SetSignatureBit<CompT>(handle, true);
    }

    /**
     * Copies all components and the signature of one instance to another and updates the copy's systems.
     *
     * @param source_handle Live instance to copy from.
     * @param copy_handle Live instance to copy to; its existing components of the same types are overwritten.
     */
    void CloneComponents(InstanceHandle source_handle, InstanceHandle copy_handle)
    {
        mComponentManager->CloneEntityComponents(source_handle, copy_handle);
        mInstanceManager->SetSignature(copy_handle, mInstanceManager->GetSignature(source_handle));

        UpdateSystems(copy_handle);
    }

    /**
     * Removes an instance's `CompT` and updates its signature and systems. Asserts if it has none.
     *
     * @param handle Live instance to remove the component from.
     */
    template <EcsComponent CompT>
    void RemoveComponent(InstanceHandle handle)
    {
        mComponentManager->RemoveComponent<CompT>(handle);
        SetSignatureBit<CompT>(handle, false);
    }

    /**
     * Like `RemoveComponent`, but does nothing to the components if the instance has no `CompT`.
     *
     * @param handle Live instance to remove the component from.
     */
    template <EcsComponent CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        mComponentManager->TryRemoveComponent<CompT>(handle);
        SetSignatureBit<CompT>(handle, false);
    }

    /**
     * Removes all components of an instance, clears its signature and removes it from all systems.
     *
     * @param handle Live instance to remove the components from.
     */
    void RemoveAllComponents(InstanceHandle handle)
    {
        mComponentManager->RemoveAllComponents(handle);
        mInstanceManager->SetSignature(handle, {});

        UpdateSystems(handle);
    }

    /// Returns `handle`'s `CompT`. Asserts if it has none. Do not keep the pointer across component removals.
    template <EcsComponent CompT>
    [[nodiscard]] CompT* GetComponent(InstanceHandle handle)
    {
        return mComponentManager->GetComponent<CompT>(handle);
    }

    /// Returns `handle`'s `CompT`, or nullptr if it has none or is dead. Do not keep the pointer across component removals.
    template <EcsComponent CompT>
    [[nodiscard]] CompT* TryGetComponent(InstanceHandle handle)
    {
        return mComponentManager->TryGetComponent<CompT>(handle);
    }

    /// Returns the signature bit of `CompT`, e.g. to build system signatures.
    template <EcsComponent CompT>
    [[nodiscard]] ComponentType GetComponentType() const
    {
        return mComponentManager->GetComponentType<CompT>();
    }

    /// Signature with the bits of all `CompTs` set, e.g. `MakeSignature<Living, Attacker>()`.
    template <EcsComponent... CompTs>
    [[nodiscard]] Signature MakeSignature() const
    {
        Signature signature;
        (signature.set(GetComponentType<CompTs>()), ...);
        return signature;
    }

    /// True if `handle`'s signature contains `CompT`.
    template <EcsComponent CompT>
    [[nodiscard]] bool TestSignature(InstanceHandle handle)
    {
        return mInstanceManager->getInstance(handle).mSignature.test(GetComponentType<CompT>());
    }

    // System Management
    // -----------------

    /**
     * Adds an instance to all systems whose signature it matches and removes it from the others.
     *
     * `Prototype` instances are removed from all systems instead.
     *
     * @param handle Live instance whose signature changed.
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

    /**
     * Creates system `SysT` (owned by the `SystemManager`) that receives every entity with all `Required` components.
     *
     * Register the components first. Without `Required`, the system receives every entity.
     * Register systems before creating entities: existing entities are not re-checked.
     *
     * @return The new system, e.g. to call its `Init` and `Update`.
     */
    template <EcsSystem SysT, EcsComponent... Required>
    std::shared_ptr<SysT> RegisterSystem()
    {
        auto system = mSystemManager->RegisterSystem<SysT>();
        mSystemManager->SetSignature<SysT>(MakeSignature<Required...>());
        return system;
    }

    /// Replaces the components an entity needs to be part of `SysT`; existing entities are not re-checked.
    template <EcsSystem SysT>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<SysT>(signature);
    }

    /// Direct access to the instance storage.
    World* GetInstanceManager() { return mInstanceManager; }

    /// Direct access to the system manager.
    SystemManager* GetSystemManager() { return mSystemManager.get(); }

    /// Direct access to the component manager.
    ComponentManager* GetComponentManager() { return mComponentManager.get(); }

    // Event Management
    // ----------------

    /**
     * Registers a listener for all functional events of one type; use with `FUNCTIONAL_LISTENER`.
     *
     * @param type Event type to listen to.
     * @param listener Called synchronously by `SendFunctionalEvent`; cannot be removed.
     */
    void AddFunctionalListener(EventType type, std::function<void(Event&)> listener)
    {
        mEventManager->AddFunctionalListener(type, std::move(listener));
    }

    /**
     * Calls all listeners of the event's type synchronously, in registration order.
     *
     * @param event Event to send; listeners may modify it.
     */
    void SendFunctionalEvent(Event& event) { mEventManager->SendFunctionalEvent(event); }

    /**
     * Queues an event for manual handling via `GetPendingEvents`. Unused.
     *
     * @param event Event to copy into the queue.
     */
    void SendEvent(Event& event) { mEventManager->SendEvent(event); }

    /// Events queued with `SendEvent`.
    std::vector<Event>& GetPendingEvents() { return mEventManager->GetPendingEvents(); }

    /**
     * Spatial hash of instances on the ground plane for neighbourhood queries (e.g. targets in range, free spawn spots).
     *
     * Factories add created instances and `PhysicsSystem` updates moved ones.
     */
    Hash2D& GetHashMap() { return mSpatialHashMap; }

private:
    /// Sets or clears the signature bit of `CompT` and re-evaluates the system membership.
    template <EcsComponent CompT>
    void SetSignatureBit(InstanceHandle handle, bool value)
    {
        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(GetComponentType<CompT>(), value);
        mInstanceManager->SetSignature(handle, signature);

        UpdateSystems(handle);
    }

    World* mInstanceManager = nullptr;                   ///< Instance storage (not owned).
    std::unique_ptr<SystemManager> mSystemManager;       ///< Systems and their signatures.
    std::unique_ptr<ComponentManager> mComponentManager; ///< Component types and arrays.
    std::unique_ptr<EventManager> mEventManager;         ///< Event listeners and queue.
    Hash2D mSpatialHashMap;                              ///< See `GetHashMap`; instances are removed when destroyed.
};
}
