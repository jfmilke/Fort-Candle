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

            // Arrow arc (fake gravity): straight line from start to end height plus a parabolic bulge
            if (InstanceBody->arrowArc)
            {
                auto move_flat = move;
                move_flat.y = 0;
                InstanceBody->arc_x += tg::length(move_flat);

                auto const u = InstanceBody->arc_x / InstanceBody->arc_xMax;
                auto const rise = InstanceBody->arc_yEnd - InstanceBody->arc_yStart;
                auto const bulge = 4.f * InstanceBody->arc_height;

                Instance.xform.translation.y = InstanceBody->arc_yStart + rise * u + bulge * u * (1.f - u);
                InstanceBody->arc_ySlope = (rise + bulge * (1.f - 2.f * u)) / InstanceBody->arc_xMax;
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
