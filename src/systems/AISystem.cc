#include "systems/AISystem.hh"
#include "advanced/transform.hh"

void gamedev::AISystem::AddEntity(InstanceHandle& handle, Signature entitySignature) {
    mEntities.insert(handle);
}

void gamedev::AISystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) {
    mEntities.erase(handle);
}

void gamedev::AISystem::RemoveEntity(InstanceHandle& handle) {
    mEntities.erase(handle);
}

void gamedev::AISystem::RemoveAllEntities() { mEntities.clear(); }

void gamedev::AISystem::Init(std::shared_ptr<EngineECS>& ecs) {
    mECS = ecs;
}

void gamedev::AISystem::Update(float dt)
{
    for (const auto& handle : mEntities)
    {

    }
}