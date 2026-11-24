#include "systems/AnimationSystem.hh"
#include "advanced/transform.hh"
#include "glow/common/log.hh"

namespace
{
// Eases a remaining angle: returns the part to apply in this update (the fraction 7 * dt, or all of it once less than 0.1° would be left)
// and clears `active` when the angle is used up.
tg::angle StepRemainingAngle(tg::angle& remaining, bool& active, float dt)
{
    auto step = remaining * tg::min(7.f * dt, 1.f);

    if (tg::abs(remaining - step).degree() < 0.1)
    {
        step = remaining;
        active = false;
    }

    remaining -= step;
    return step;
}

// Squared distance from the point p to the segment from a to b.
float SegmentPointDistanceSqr(tg::pos3 a, tg::pos3 b, tg::pos3 p)
{
    auto ab = b - a;
    auto lengthSqr = tg::dot(ab, ab);
    auto t = lengthSqr > 0.f ? tg::clamp(tg::dot(p - a, ab) / lengthSqr, 0.f, 1.f) : 0.f;
    auto d = p - (a + t * ab);
    return tg::dot(d, d);
}
}

void gamedev::AnimationSystem::AddEntity(InstanceHandle handle, Signature entitySignature)
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

void gamedev::AnimationSystem::RemoveEntity(InstanceHandle handle)
{
    mArrows.erase(handle);
    mEntities.erase(handle);
}

void gamedev::AnimationSystem::RemoveAllEntities()
{
    mArrows.clear();
    mEntities.clear();
}

void gamedev::AnimationSystem::Init(EngineECS& ecs) { mECS = &ecs; }

void gamedev::AnimationSystem::Update(float dt)
{
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

                // Clearly past the end of its arc (missed): it would only fly on underground
                if (physics->arrowArc && physics->arc_x > physics->arc_xMax + 1.f)
                {
                    mECS->MarkDestruction(handle);
                    continue;
                }
            }

            // Collision: the segment travelled in this update against the target's body center
            if (mECS->IsLiveHandle(arrow->target))
            {
                auto& target = mECS->GetInstance(arrow->target);
                auto aim = tg::pos3(target.xform.translation) + tg::vec3(0.f, Arrow::aimHeight, 0.f);
                auto end = tg::pos3(instance.xform.translation);
                auto start = physics ? tg::pos3(physics->lastPosition) : end;

                if (SegmentPointDistanceSqr(start, end, aim) < tg::pow2(Arrow::hitRadius))
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
}

void gamedev::AnimationSystem::AlignTrajectory(Instance& instance, Physics* physics)
{
    auto angle
        = tg::acos(tg::dot(instance.xform.rotation_quat_x() * instance.xform.global_forward(), tg::normalize(tg::vec3(0.0f, physics->arc_ySlope, 1.f))));
    instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_x, angle));
}

void gamedev::AnimationSystem::Rotate(Instance& instance, Animated* subject, float dt)
{
    if (subject->rotQuatAnim)
    {
        auto alpha = tg::saturate(subject->rot_speed * dt);
        instance.xform.inaccurate_lerp_to_rotation(subject->rot_quat, alpha);
    }

    if (subject->rotXAnim)
        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_x, StepRemainingAngle(subject->rot_x, subject->rotXAnim, dt)));

    if (subject->rotYAnim)
        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_y, StepRemainingAngle(subject->rot_y, subject->rotYAnim, dt)));

    if (subject->rotZAnim)
        instance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_z, StepRemainingAngle(subject->rot_z, subject->rotZAnim, dt)));
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

void gamedev::AnimationSystem::Wiggle(Living* subject, float dt) {}
