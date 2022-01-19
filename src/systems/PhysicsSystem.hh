#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"
#include "Terrain.hh"

#include <array>

namespace gamedev
{
class PhysicsSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);
    void SetTerrain(std::shared_ptr<Terrain>& t);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    int Update(float dt);

private:
    void UpdateHashmap(InstanceHandle handle, tg::vec3& translation);
    void CollisionListener(Event& e);

private:
    int mUpdateControl = 0;

private:
    const float mGravity = -9.81f;
    const float mDamping = 0.02f;
    float last_dt = 0.0;

private:
    std::shared_ptr<EngineECS> mECS;
    std::shared_ptr<Terrain> mTerrain;
};
}

