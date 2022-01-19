#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
class AnimationSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    int Update(float dt);

    void Rotate(Instance& instance, Animated* subject, float dt);
    void Scale(Instance& instance, Animated* subject, float dt);
    void Wiggle(Living* subject, float dt);
    void AlignTrajectory(Instance& instance, Physics* physics);

private:
    std::shared_ptr<EngineECS> mECS;

    std::set<InstanceHandle> mArrows;
    std::set<InstanceHandle> mLiving;
};
}

