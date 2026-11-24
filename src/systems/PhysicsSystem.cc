#include "systems/PhysicsSystem.hh"
#include "advanced/transform.hh"

void gamedev::PhysicsSystem::Init(EngineECS& ecs)
{
    mECS = &ecs;
}

void gamedev::PhysicsSystem::SetTerrain(std::shared_ptr<Terrain>& t) { mTerrain = t; }

void gamedev::PhysicsSystem::Update(float dt)
{
    for (const auto& handle : mEntities)
    {
        if (mECS->TestSignature<InTower>(handle))
            continue;

        auto& Instance = mECS->GetInstance(handle);
        auto InstanceBody = mECS->GetComponent<Physics>(handle);
        InstanceBody->lastPosition = Instance.xform.translation;

        // Gravity (also accelerates resting bodies)
        if (InstanceBody->gravity)
        {
            InstanceBody->velocity.y += dt * mGravity;
        }

        // Move
        if (InstanceBody->velocity != tg::vec3::zero)
        {
            auto move = dt * InstanceBody->velocity;
            Instance.xform.move_absolute(move);
            Instance.new_position = true;

            // Arrow parabola (fake gravity)
            if (InstanceBody->arrowArc)
            {
                auto move_flat = move;
                move_flat.y = 0;

                InstanceBody->arc_x += tg::length(move_flat);
                auto a = (-4.f * InstanceBody->arc_yMax) / tg::pow2(InstanceBody->arc_xMax);
                auto b = (4 * InstanceBody->arc_yMax) / InstanceBody->arc_xMax;
                auto c = InstanceBody->arc_yBase;

                auto slope = 2.f * a * InstanceBody->arc_x + b;

                Instance.xform.translation.y = a * tg::pow2(InstanceBody->arc_x) + b * InstanceBody->arc_x + c;
                InstanceBody->arc_ySlope = slope;
            }
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

        // Publish moves (by this system or by collision pushes) once
        if (Instance.new_position)
        {
            Instance.new_position = false;
            auto e = Event(EventType::NewPosition, handle, handle);
            mECS->SendFunctionalEvent(e);
            UpdateHashmap(handle, Instance.xform.translation);
        }
    }
}

void gamedev::PhysicsSystem::UpdateHashmap(InstanceHandle handle, tg::vec3& translation) { mECS->GetHashMap().UpdateInstance(handle, translation); }
