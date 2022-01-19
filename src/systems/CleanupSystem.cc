#include "systems/CleanupSystem.hh"
#include <queue>
#include "advanced/transform.hh"
#include "glow/common/log.hh"

void gamedev::CleanupSystem::AddEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.insert(handle); }

void gamedev::CleanupSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.erase(handle); }

void gamedev::CleanupSystem::RemoveEntity(InstanceHandle& handle) { mEntities.erase(handle); }

void gamedev::CleanupSystem::RemoveAllEntities() { mEntities.clear(); }

void gamedev::CleanupSystem::Init(std::shared_ptr<EngineECS>& ecs) {
    mECS = ecs;
}

int gamedev::CleanupSystem::Update()
{
    auto t0 = std::chrono::steady_clock::now();

    std::vector<InstanceHandle> destroy;

    for (const auto& handle : mEntities)
    {
        if (mECS->GetInstance(handle).destroy)
        {
            destroy.push_back(handle);
        }
    }

    for (auto& handle : destroy)
    {
        mECS->DestroyInstance(handle);
    }

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}
