#include "systems/AnimationSystem.hh"
#include "advanced/transform.hh"
#include "glow/common/log.hh"

void gamedev::AnimationSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    if (entitySignature.test(mECS->GetComponentType<Arrow>()))
    {
        mArrows.insert(handle);
        mEntities.erase(handle);
    }
    else
    {
        mEntities.insert(handle);
        mArrows.erase(handle);
    }
}

void gamedev::AnimationSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) {
    mArrows.erase(handle);
    mEntities.erase(handle);
}

void gamedev::AnimationSystem::RemoveEntity(InstanceHandle& handle) {
    mArrows.erase(handle);
    mEntities.erase(handle);
}

void gamedev::AnimationSystem::RemoveAllEntities()
{
    mArrows.clear();
    mEntities.clear();
}

void gamedev::AnimationSystem::Init(std::shared_ptr<EngineECS>& ecs) {
    mECS = ecs;
}

int gamedev::AnimationSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    for (const auto& handle : mArrows)
    {
        auto arrow = mECS->TryGetComponent<Arrow>(handle);

        if (arrow)
        {
            auto& instance = mECS->GetInstance(handle);

            // Lifetime
            if (arrow->lifetime < 0)
            {
                mECS->MarkDestruction(handle);
                continue;
            }
            arrow->lifetime -= dt;

            // Orientation
            auto physics = mECS->TryGetComponent<Physics>(handle);
            if (physics)
            {
                AlignTrajectory(instance, physics);
            }

            // Collision
            if (mECS->IsLiveHandle(arrow->target))
            {
                auto& target = mECS->GetInstance(arrow->target);
                if (tg::distance_sqr(instance.xform.translation, target.xform.translation) < 0.1)
                {
                    Event e(EventType::ArrowHit, handle, arrow->target);
                    mECS->SendFunctionalEvent(e);
                }
            }
        }
    }

    for (const auto& handle : mEntities)
    {
        auto animation = mECS->GetComponent<Animated>(handle);
        auto& instance = mECS->GetInstance(handle);

        Rotate(instance, animation, dt);
        Scale(instance, animation, dt);
    }

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::AnimationSystem::AlignTrajectory(Instance& instance, Physics* physics)
{
    auto angle = tg::acos(tg::dot(instance.xform.rotation_quat_x() * instance.xform.global_forward(), tg::normalize(tg::vec3(0.0f, physics->arc_ySlope, 1.f))));
    instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_x, angle));
}

void gamedev::AnimationSystem::Rotate(Instance& instance, Animated* subject, float dt)
{
    auto rotate = 0_deg;

    if (subject->rotQuatAnim)
    {
        auto alpha = tg::saturate(subject->rot_speed * dt);
        instance.xform.inaccurate_lerp_to_rotation(subject->rot_quat, alpha);
    }

    if (subject->rotXAnim)
    {
        if (subject->rot_x < 0_deg)
        {
            rotate = tg::max(subject->rot_x, subject->rot_x * 7.f * dt);
            subject->rot_x -= rotate;

            if (subject->rot_x >= 0_deg)
                subject->rotXAnim = false;
        }
        else
        {
            rotate = tg::min(subject->rot_x, subject->rot_x * 7.f * dt);
            subject->rot_x -= rotate;

            if (subject->rot_x <= 0_deg)
                subject->rotXAnim = false;
        }

        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_x, rotate));
    }

    if (subject->rotYAnim)
    {
        if (subject->rot_y < 0_deg)
        {
            rotate = tg::max(subject->rot_y, subject->rot_y * 7.f * dt);
            subject->rot_y -= rotate;

            if (subject->rot_y >= 0_deg)
                subject->rotYAnim = false;
        }
        else
        {
            rotate = tg::min(subject->rot_y, subject->rot_y * 7.f * dt);
            subject->rot_y -= rotate;

            if (subject->rot_y <= 0_deg)
                subject->rotYAnim = false;
        }

        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_y, rotate));
    }
    
    if (subject->rotZAnim)
    {
        if (subject->rot_z < 0_deg)
        {
            rotate = tg::max(subject->rot_z, subject->rot_z * 7.f * dt);
            subject->rot_z -= rotate;

            if (subject->rot_z >= 0_deg)
                subject->rotZAnim = false;
        }
        else
        {
            rotate = tg::min(subject->rot_z, subject->rot_z * 7.f * dt);
            subject->rot_z -= rotate;

            if (subject->rot_z <= 0_deg)
                subject->rotZAnim = false;
        }

        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_z, rotate));
    }
}

void gamedev::AnimationSystem::Scale(Instance& instance, Animated* subject, float dt)
{
    if (!subject->scaleAnim)
        return;

    auto& xform = instance.xform;
    auto currentScale = instance.xform.scaling.depth;
    auto scaleLeft = subject->scale - currentScale;

    auto scale = 0.f;

    // Shrink
    if (subject->scale < currentScale)
    {
        if (scaleLeft >= 0.f)
        {
            subject->scaleAnim = false;
            return;
        }

        scale = tg::max(scaleLeft, -1.f * dt);
    }
    // Enlarge
    else
    {
        if (scaleLeft <= 0.f)
        {
            subject->scaleAnim = false;
            return;
        }

        scale = tg::min(scaleLeft, 1.f * dt);
    }

    xform.scaling += scale;
}

void gamedev::AnimationSystem::Wiggle(Living* subject, float dt)
{
    
}