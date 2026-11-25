#include "systems/AnimationSystem.hh"
#include "systems/CollisionSystem.hh"
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

// Parameter in [0, 1] of the point on the segment from a to b that is closest to p.
float ClosestOnSegment(tg::pos3 a, tg::pos3 b, tg::pos3 p)
{
    auto ab = b - a;
    auto lengthSqr = tg::dot(ab, ab);
    return lengthSqr > 0.f ? tg::clamp(tg::dot(p - a, ab) / lengthSqr, 0.f, 1.f) : 0.f;
}

// Rotation that turns the local +z axis (the arrow tip) into the direction dir (normalized): yaw about y, then pitch about the local x axis.
tg::quat RotationAlong(tg::vec3 dir)
{
    auto const yaw = tg::atan2(dir.x, dir.z);
    auto const pitch = tg::atan2(dir.y, tg::length(tg::vec2(dir.x, dir.z)));
    return tg::quat::from_axis_angle(tg::dir3::pos_y, yaw) * tg::quat::from_axis_angle(tg::dir3::pos_x, -pitch);
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
    std::erase(mStuckArrows, handle);
}

void gamedev::AnimationSystem::RemoveAllEntities()
{
    mArrows.clear();
    mEntities.clear();
    mStuckArrows.clear();
}

void gamedev::AnimationSystem::Init(EngineECS& ecs) { mECS = &ecs; }

void gamedev::AnimationSystem::SetTerrain(std::shared_ptr<Terrain> terrain) { mTerrain = std::move(terrain); }

void gamedev::AnimationSystem::SetCollision(std::shared_ptr<CollisionSystem> collision) { mCollision = std::move(collision); }

void gamedev::AnimationSystem::Update(float dt)
{
    for (const auto& handle : mEntities)
    {
        auto animation = mECS->GetComponent<Animated>(handle);
        auto& instance = mECS->GetInstance(handle);

        Rotate(instance, animation, dt);
        Scale(instance, animation, dt);
    }

    // After the animations, so arrows stuck in a unit follow its final transform of this update
    for (const auto& handle : mArrows)
    {
        auto arrow = mECS->GetComponent<Arrow>(handle);

        if (arrow->state == ArrowState::Flying)
            UpdateFlyingArrow(handle, *arrow, dt);
        else if (arrow->state == ArrowState::InObject)
            FollowObject(handle, *arrow);
    }

    // The oldest arrows in the ground or in structures make room for new ones
    while (mStuckArrows.size() > kMaxStuckArrows)
    {
        mECS->MarkDestruction(mStuckArrows.front());
        mStuckArrows.pop_front();
    }
}

void gamedev::AnimationSystem::UpdateFlyingArrow(InstanceHandle handle, Arrow& arrow, float dt)
{
    // Lifetime (arrows that never land)
    if (arrow.lifetime < 0)
    {
        mECS->MarkDestruction(handle);
        return;
    }
    arrow.lifetime -= dt;

    auto physics = mECS->TryGetComponent<Physics>(handle);
    if (!physics || physics->velocity == tg::vec3::zero)
        return;

    // Point along the flight direction
    auto& xform = mECS->GetInstanceTransform(handle);
    auto const dir = tg::normalize(physics->velocity);
    xform.rotation = RotationAlong(dir);

    // Path of the tip in this update
    auto const tipBefore = tg::pos3(physics->lastPosition) + dir * Arrow::tipOffset;
    auto const tipNow = tg::pos3(xform.translation) + dir * Arrow::tipOffset;

    // Target: the tip passes its aim point
    auto targetFraction = 2.f; // none
    auto aim = tg::pos3::zero;
    auto targetOffset = tg::vec3::zero;
    if (mECS->IsLiveHandle(arrow.target) && !mECS->GetInstance(arrow.target).destroy)
    {
        aim = tg::pos3(mECS->GetInstanceTransform(arrow.target).translation) + tg::vec3(0.f, Arrow::aimHeight, 0.f);
        auto const fraction = ClosestOnSegment(tipBefore, tipNow, aim);
        targetOffset = tg::mix(tipBefore, tipNow, fraction) - aim;

        if (tg::dot(targetOffset, targetOffset) < tg::pow2(Arrow::hitRadius))
            targetFraction = fraction;
    }

    // Structure: straight shots stick in the first one the tip enters, unless the target comes first
    if (arrow.hitsBlockers && mCollision)
    {
        if (auto const hit = mCollision->FindSegmentHit(tipBefore, tipNow, arrow.launchPosition); hit && hit->fraction < targetFraction)
        {
            auto const impact = tg::mix(tipBefore, tipNow, hit->fraction);
            xform.translation = tg::vec3(impact + dir * (Arrow::embedDepth - Arrow::tipOffset));

            mStuckArrows.push_back(handle);
            StickIn(handle, arrow, hit->body);
            return;
        }
    }

    if (targetFraction <= 1.f)
    {
        Event e(EventType::ArrowHit, handle, arrow.target);
        mECS->SendFunctionalEvent(e);

        // The tip sticks near the aim point, at most `stuckRadius` off it
        auto const stuck = tg::length(targetOffset) > Arrow::stuckRadius ? tg::normalize(targetOffset) * Arrow::stuckRadius : targetOffset;
        xform.translation = tg::vec3(aim + stuck - dir * Arrow::tipOffset);

        StickIn(handle, arrow, arrow.target);
        return;
    }

    // Landing: the tip goes below the terrain
    if (!mTerrain)
        return;

    auto const depthNow = tipNow.y - mTerrain->heightAt(tipNow);
    if (depthNow >= 0.f)
        return;

    auto const depthBefore = tipBefore.y - mTerrain->heightAt(tipBefore);
    auto const fraction = depthBefore > 0.f ? depthBefore / (depthBefore - depthNow) : 0.f;
    auto const impact = tg::mix(tipBefore, tipNow, fraction);
    xform.translation = tg::vec3(impact + dir * (Arrow::embedDepth - Arrow::tipOffset));

    arrow.state = ArrowState::InGround;
    mStuckArrows.push_back(handle);
    StopArrow(handle);
}

void gamedev::AnimationSystem::StickIn(InstanceHandle handle, Arrow& arrow, InstanceHandle object)
{
    auto const& host = mECS->GetInstanceTransform(object);
    auto const& xform = mECS->GetInstanceTransform(handle);

    // Relative to the object: position in its local frame before scaling, and rotation
    auto const inverseRotation = tg::conjugate(host.rotation);
    auto const local = inverseRotation * (xform.translation - host.translation);
    arrow.stuckTo = object;
    arrow.stuckOffset = {local.x / host.scaling.width, local.y / host.scaling.height, local.z / host.scaling.depth};
    arrow.stuckRotation = tg::normalize(inverseRotation * xform.rotation);
    arrow.state = ArrowState::InObject;

    StopArrow(handle);

    // If this arrow killed the target, it goes with it
    FollowObject(handle, arrow);
}

void gamedev::AnimationSystem::FollowObject(InstanceHandle handle, Arrow& arrow)
{
    if (!mECS->IsLiveHandle(arrow.stuckTo) || mECS->GetInstance(arrow.stuckTo).destroy || mECS->TestSignature<Prototype>(arrow.stuckTo))
    {
        mECS->MarkDestruction(handle);
        return;
    }

    auto const& host = mECS->GetInstanceTransform(arrow.stuckTo);
    auto& xform = mECS->GetInstanceTransform(handle);
    xform.translation = host.translation + host.rotation * (arrow.stuckOffset * host.scaling);
    xform.rotation = tg::normalize(host.rotation * arrow.stuckRotation);
}

void gamedev::AnimationSystem::StopArrow(InstanceHandle handle)
{
    // No more flight, smoke trail or neighbourhood queries; the transform is set directly from now on
    mECS->TryRemoveComponent<Physics>(handle);
    mECS->TryRemoveComponent<ParticleEmitter>(handle);
    mECS->GetHashMap().RemoveInstance(handle);
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
