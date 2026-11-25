#include "systems/PhysicsSystem.hh"
#include "advanced/transform.hh"

void gamedev::PhysicsSystem::Init(EngineECS& ecs)
{
    mECS = &ecs;
}

void gamedev::PhysicsSystem::SetTerrain(std::shared_ptr<Terrain>& t) { mTerrain = t; }

void gamedev::PhysicsSystem::Update(float dt)
{
    // Exponential smoothing factor of the observed velocity
    auto const smoothing = 1.f - tg::exp(-dt / kObservedVelocityTime);

    for (const auto& handle : mEntities)
    {
        if (mECS->TestSignature<InTower>(handle))
            continue;

        auto& Instance = mECS->GetInstance(handle);
        auto InstanceBody = mECS->GetComponent<Physics>(handle);

        // Observed velocity: the whole move since the last update (physics, collision pushes, teleports)
        if (dt > 0.f)
        {
            auto observed = (Instance.xform.translation - InstanceBody->lastPosition) / dt;
            observed.y = 0.f;
            InstanceBody->observedVelocity += smoothing * (observed - InstanceBody->observedVelocity);
        }

        InstanceBody->lastPosition = Instance.xform.translation;

        // Move; with gravity integrated exactly, so projectiles follow the parabola they were aimed along
        auto move = dt * InstanceBody->velocity;
        if (InstanceBody->gravity)
        {
            move.y -= 0.5f * kGravity * dt * dt;
            InstanceBody->velocity.y -= kGravity * dt;
        }

        if (move != tg::vec3::zero)
        {
            Instance.xform.move_absolute(move);
            Instance.new_position = true;
        }

        // Update height
        if (InstanceBody->forceGround)
        {
            float h = mTerrain->heightAt(tg::pos3::zero + Instance.xform.translation);
            Instance.xform.translation.y = h;

            // Landed: gravity must not build up speed while the body stands on the ground
            if (InstanceBody->velocity.y < 0.f)
                InstanceBody->velocity.y = 0.f;
        }
    }
}
