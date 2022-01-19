#include "systems/PhysicsSystem.hh"
#include "advanced/transform.hh"

void gamedev::PhysicsSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    mEntities.insert(handle);
}

void gamedev::PhysicsSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature)
{
    mEntities.erase(handle);
}

void gamedev::PhysicsSystem::RemoveEntity(InstanceHandle& handle)
{
    mEntities.erase(handle);
}

void gamedev::PhysicsSystem::RemoveAllEntities() { mEntities.clear(); }

void gamedev::PhysicsSystem::Init(std::shared_ptr<EngineECS>& ecs)
{
    mECS = ecs;

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::StaticCollision, PhysicsSystem::CollisionListener));
}

void gamedev::PhysicsSystem::SetTerrain(std::shared_ptr<Terrain>& t) {
    mTerrain = t;
}

int gamedev::PhysicsSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    for (const auto& handle : mEntities)
    {
        if (mECS->TestSignature<InTower>(handle))
            continue;

        auto& Instance = mECS->GetInstance(handle);
        auto InstanceBody = mECS->GetComponent<Physics>(handle);
        InstanceBody->lastPosition = Instance.xform.translation;

        // Move
        if (InstanceBody->velocity != tg::vec3::zero)
        {

            auto move = dt * InstanceBody->velocity;
            Instance.xform.move_absolute(move);
            Instance.new_position = true;

            // Gravity
            if (InstanceBody->gravity)
            {
                InstanceBody->velocity += dt * mGravity;
            }

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
        }

        if (Instance.new_position && (mUpdateControl % 10))
        {
            auto e = Event(EventType::NewPosition, handle, handle);
            mECS->SendFunctionalEvent(e);
            UpdateHashmap(handle, Instance.xform.translation);
        }
    }

    mUpdateControl++;

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::PhysicsSystem::UpdateHashmap(InstanceHandle handle, tg::vec3& translation) { mECS->GetHashMap().UpdateInstance(handle, translation); }

void gamedev::PhysicsSystem::CollisionListener(Event& e)
{
    auto physics = mECS->TryGetComponent<Physics>(e.mSubject);

    if (physics)
    {
        physics->lastPosition = mECS->GetInstanceTransform(e.mSubject).translation;
    }
}