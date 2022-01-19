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

/***
 * https://austinmorlan.com/posts/entity_component_system/
 *
 * GameEngine is a convenience class to manage all the introduced functionality.
 * Instead of accessing several globals (ComponentManager, SystemManager, ..) to manage every change, only this global is needed.
 *
 ***/

namespace gamedev
{
GLOW_SHARED(class, EngineECS);

class EngineECS
{
public:
    // Creates all necessary systems
    void Init(World& world)
    {
        mSystemManager = std::make_unique<SystemManager>();
        mComponentManager = std::make_unique<ComponentManager>();
        mEventManager = std::make_unique<EventManager>();
        mInstanceManager = std::shared_ptr<World>(&world);
    }

    // Entity Management (world)
    // -------------------------
    [[nodiscard]] InstanceHandle CreateInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm)
    {
        return mInstanceManager->createInstance(vao, albedo, normal, arm);
    }

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

    Instance& GetInstance(InstanceHandle handle) { return mInstanceManager->getInstance(handle); }

    transform& GetInstanceTransform(InstanceHandle handle) { return mInstanceManager->getInstance(handle).xform; }

    uint32_t getNumLiveInstances() const { return mInstanceManager->getNumLiveInstances(); }

    void MarkDestruction(InstanceHandle handle)
    {
        mInstanceManager->getInstance(handle).destroy = true;
    }

    void DestroyInstance(InstanceHandle handle)
    {
        mInstanceManager->destroyInstance(handle);
        InstanceDestroyed(handle);
    }

    void InstanceDestroyed(InstanceHandle handle)
    {
        mComponentManager->EntityDestroyed(handle);
        mSystemManager->EntityDestroyed(handle);
        mSpatialHashMap.RemoveInstance(handle);
    }
    
    void AllInstancesDestroyed()
    {
        mComponentManager->AllEntitiesDestroyed();
        mSystemManager->AllEntitiesDestroyed();
        mSpatialHashMap.Clear();
    }

    bool IsLiveHandle(InstanceHandle handle) const { return mInstanceManager->isLiveHandle(handle); }

    // Component Management
    // --------------------
    template <typename CompT>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<CompT>();
    }

    template <typename CompT>
    CompT* CreateComponent(InstanceHandle handle)
    {
        auto Component = mComponentManager->CreateComponent<CompT>(handle, mInstanceManager->getInstance(handle));

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        // mSystemManager->EntitySignatureChanged(handle, signature);
        UpdateSystems(handle);

        return Component;
    }

    template <typename CompT>
    CompT* TryCreateComponent(InstanceHandle handle)
    {
        auto Component = mComponentManager->TryCreateComponent<CompT>(handle, mInstanceManager->getInstance(handle));

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        // mSystemManager->EntitySignatureChanged(handle, signature);
        UpdateSystems(handle);

        return Component;
    }

    template <typename CompT>
    void AddComponent(InstanceHandle handle, CompT component)
    {
        mComponentManager->AddComponent<CompT>(handle, component);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), true);
        mInstanceManager->SetSignature(handle, signature);

        // mSystemManager->EntitySignatureChanged(handle, signature);
        UpdateSystems(handle);
    }

    void CloneComponents(InstanceHandle source_handle, InstanceHandle copy_handle)
    {
        auto& source_instance = GetInstance(source_handle);
        auto& copy_instance = GetInstance(copy_handle);

        mComponentManager->CloneEntityComponents(source_instance, copy_instance);

        mInstanceManager->SetSignature(copy_handle, source_instance.mSignature);

        UpdateSystems(copy_handle);
    }

    template <typename CompT>
    void RemoveComponent(InstanceHandle handle)
    {
        mComponentManager->RemoveComponent<CompT>(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), false);
        mInstanceManager->SetSignature(handle, signature);
        
        UpdateSystems(handle);
    }

    template <typename CompT>
    void TryRemoveComponent(InstanceHandle handle)
    {
        mComponentManager->TryRemoveComponent<CompT>(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.set(mComponentManager->GetComponentType<CompT>(), false);
        mInstanceManager->SetSignature(handle, signature);

        // mSystemManager->EntitySignatureChanged(handle, signature);

        UpdateSystems(handle);
    }

    void RemoveAllComponents(InstanceHandle handle)
    {
        mComponentManager->RemoveAllComponents(handle);

        auto signature = mInstanceManager->GetSignature(handle);
        signature.reset();
        mInstanceManager->SetSignature(handle, signature);

        // mSystemManager->EntitySignatureChanged(handle, signature);

        UpdateSystems(handle);
    }

    template <typename CompT>
    CompT* GetComponent(InstanceHandle handle)
    {
        return mComponentManager->GetComponent<CompT>(handle);
    }

    template <typename CompT>
    CompT* TryGetComponent(InstanceHandle handle)
    {
        return mComponentManager->TryGetComponent<CompT>(handle);
    }

    template <typename CompT>
    ComponentType GetComponentType()
    {
        return mComponentManager->GetComponentType<CompT>();
    }

    template <typename CompT>
    bool TestSignature(InstanceHandle handle)
    {
        return mInstanceManager->getInstance(handle).mSignature.test(mComponentManager->GetComponentType<CompT>());
    }

    // System Management
    // -----------------
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

    template <typename SysT>
    std::shared_ptr<SysT> RegisterSystem()
    {
        return mSystemManager->RegisterSystem<SysT>();
    }

    template <typename SysT>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<SysT>(signature);
    }

    World* GetInstanceManager() { return mInstanceManager.get(); }
    SystemManager* GetSystemManager() { return mSystemManager.get(); }
    ComponentManager* GetComponentManager() { return mComponentManager.get(); }

    // Event Management
    // ----------------
    void AddFunctionalListener(EventType type, std::function<void(Event&)> const&listener)
    {
        mEventManager->AddFunctionalListener(type, listener);
    }

    void SendFunctionalEvent(Event& event)
    {
        mEventManager->SendFunctionalEvent(event);
    }

    void SendEvent(Event& event)
    {
        mEventManager->SendEvent(event);
    }

    std::vector<Event>& GetPendingEvents() { return mEventManager->GetPendingEvents(); }

    // Spatial Hash Map
    Hash2D& GetHashMap() { return mSpatialHashMap; }


private:
    std::shared_ptr<World> mInstanceManager;
    std::unique_ptr<SystemManager> mSystemManager;
    std::unique_ptr<ComponentManager> mComponentManager;
    std::unique_ptr<EventManager> mEventManager;

private:
    // Spatial Hash Map
    Hash2D mSpatialHashMap;
};
}