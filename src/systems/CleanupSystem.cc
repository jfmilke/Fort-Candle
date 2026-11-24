#include "systems/CleanupSystem.hh"
#include <queue>
#include "advanced/transform.hh"
#include "glow/common/log.hh"

void gamedev::CleanupSystem::Init(EngineECS& ecs) { mECS = &ecs; }

void gamedev::CleanupSystem::Update()
{
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
}
