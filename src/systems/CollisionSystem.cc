#include "systems/CollisionSystem.hh"
#include <clean-core/assert.hh>
#include <algorithm>
#include <limits>
#include <typed-geometry/tg-lean.hh>

void gamedev::CollisionSystem::Init(EngineECS& ecs, tg::aabb3 map_bounds)
{
    mECS = &ecs;
    mShapes.resize(MAX_INSTANCES);

    auto size = tg::size_of(map_bounds);

    mDynamicColliderHashMap.Init({size.width, size.depth}, map_bounds);
    mStaticColliderHashMap.Init({size.width, size.depth}, map_bounds);

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::NewPosition, CollisionSystem::PositionListener));
}

void gamedev::CollisionSystem::AddEntity(InstanceHandle handle, Signature entitySignature)
{
    auto& shape = CachedShape(handle);
    shape = ComputeWorldShape(handle);

    auto const& translation = mECS->GetInstanceTransform(handle).translation;
    auto const bounds = BroadPhaseBounds(shape, translation);

    // Called on every signature change, so re-insert with the current bounds
    mStaticColliderHashMap.RemoveInstance(handle);
    mDynamicColliderHashMap.RemoveInstance(handle);

    if (mECS->GetComponent<Collider>(handle)->dynamic)
    {
        mDynamicEntities.insert(handle);
        mStaticEntities.erase(handle);

        // Units inside a tower take no part in collisions
        if (!mECS->TestSignature<InTower>(handle))
            mDynamicColliderHashMap.AddInstance(handle, translation, bounds);
    }
    else
    {
        mStaticEntities.insert(handle);
        mDynamicEntities.erase(handle);

        mStaticColliderHashMap.AddInstance(handle, translation, bounds);
    }
}

void gamedev::CollisionSystem::RemoveEntity(InstanceHandle handle)
{
    mDynamicEntities.erase(handle);
    mStaticEntities.erase(handle);

    mStaticColliderHashMap.RemoveInstance(handle);
    mDynamicColliderHashMap.RemoveInstance(handle);
}

void gamedev::CollisionSystem::RemoveAllEntities()
{
    mDynamicEntities.clear();
    mStaticEntities.clear();

    mStaticColliderHashMap.Clear();
    mDynamicColliderHashMap.Clear();
}

void gamedev::CollisionSystem::Update(float /*dt*/)
{
    CollectPairs();

    for (int round = 0; round < kIterations; ++round)
    {
        // Dynamic colliders share the push
        for (auto const& [a, b] : mDynamicPairs)
        {
            auto& shapeA = CachedShape(a);
            auto& shapeB = CachedShape(b);

            auto const contact = ComputeContact(shapeA, shapeB);
            if (contact.separation >= 0.f)
                continue;

            auto const push = contact.normal * (0.5f * (kSkin - contact.separation));
            Move(a, shapeA, push);
            Move(b, shapeB, -push);
        }

        // Static colliders last, so they win
        for (auto& pair : mStaticPairs)
        {
            auto& shape = CachedShape(pair.dynamic);

            auto const contact = ComputeContact(shape, CachedShape(pair.body));
            if (contact.separation < kContactDistance)
                pair.touching = true;

            if (contact.separation >= 0.f)
                continue;

            Move(pair.dynamic, shape, contact.normal * (kSkin - contact.separation));
        }
    }

    RevertWedgedColliders();

    // Listeners run after the positions are final
    for (auto const& pair : mStaticPairs)
    {
        if (!pair.touching)
            continue;

        Event collision_event(EventType::StaticCollision, pair.body, pair.dynamic);
        mECS->SendFunctionalEvent(collision_event);
    }
}

void gamedev::CollisionSystem::CollectPairs()
{
    mDynamicPairs.clear();
    mStaticPairs.clear();

    // Units turn and move without publishing every change, so their shapes are refreshed once per update
    for (auto const handle : mDynamicEntities)
    {
        if (!mECS->TestSignature<InTower>(handle))
            CachedShape(handle) = ComputeWorldShape(handle);
    }

    for (auto const handle : mDynamicEntities)
    {
        if (mECS->TestSignature<InTower>(handle))
            continue;

        auto const& translation = mECS->GetInstanceTransform(handle).translation;
        auto const bounds = BroadPhaseBounds(CachedShape(handle), translation, kPairMargin);

        for (auto const other : mDynamicColliderHashMap.FindNear(translation, bounds))
        {
            // Each pair once
            if (handle < other)
                mDynamicPairs.push_back({handle, other});
        }

        for (auto const body : mStaticColliderHashMap.FindNear(translation, bounds))
            mStaticPairs.push_back({handle, body});
    }
}

void gamedev::CollisionSystem::RevertWedgedColliders()
{
    // CollectPairs stores the static pairs of each dynamic collider next to each other
    auto const overlapsAny = [&](size_t begin, size_t end, WorldShape const& shape)
    {
        for (auto i = begin; i < end; ++i)
            if (ComputeContact(shape, CachedShape(mStaticPairs[i].body)).separation < -kWedgeTolerance)
                return true;
        return false;
    };

    for (size_t begin = 0; begin < mStaticPairs.size();)
    {
        auto const handle = mStaticPairs[begin].dynamic;
        auto end = begin + 1;
        while (end < mStaticPairs.size() && mStaticPairs[end].dynamic == handle)
            ++end;

        auto& shape = CachedShape(handle);
        auto const physics = mECS->TryGetComponent<Physics>(handle);

        if (overlapsAny(begin, end, shape))
        {
            // Back to the position before this update's physics move, if that one is free
            auto const& translation = mECS->GetInstanceTransform(handle).translation;
            auto reverted = shape;
            if (physics)
                reverted.center += tg::vec2(physics->lastPosition.x - translation.x, physics->lastPosition.z - translation.z);

            if (physics && !overlapsAny(begin, end, reverted))
                Move(handle, shape, reverted.center - shape.center);
            else if (auto const offset = FindFreeSpot(handle, shape))
                Move(handle, shape, *offset);
        }

        begin = end;
    }
}

std::optional<tg::vec2> gamedev::CollisionSystem::FindFreeSpot(InstanceHandle handle, WorldShape const& shape)
{
    auto const& translation = mECS->GetInstanceTransform(handle).translation;
    auto const bodies = mStaticColliderHashMap.FindNear(translation, BroadPhaseBounds(shape, translation, kEscapeDistance));

    // Rings of candidate positions, nearest first
    for (auto distance = kEscapeStep; distance <= kEscapeDistance; distance += kEscapeStep)
    {
        for (int i = 0; i < kEscapeDirections; ++i)
        {
            auto const angle = tg::angle::from_degree(360.f * float(i) / float(kEscapeDirections));
            auto const offset = tg::vec2(tg::cos(angle), tg::sin(angle)) * distance;

            auto candidate = shape;
            candidate.center += offset;

            auto const blocks = [&](InstanceHandle body) { return ComputeContact(candidate, CachedShape(body)).separation < 0.f; };
            if (std::ranges::none_of(bodies, blocks))
                return offset;
        }
    }

    return std::nullopt;
}

void gamedev::CollisionSystem::Move(InstanceHandle handle, WorldShape& shape, tg::vec2 offset)
{
    auto& instance = mECS->GetInstance(handle);
    instance.xform.translation.x += offset.x;
    instance.xform.translation.z += offset.y;
    instance.new_position = true;

    shape.center += offset;
}

gamedev::CollisionSystem::Contact gamedev::CollisionSystem::ComputeContact(WorldShape const& a, WorldShape const& b)
{
    // Bounding circles first; the separation is then only a lower bound, which is enough to rule out a contact
    auto const distance = tg::distance(a.center, b.center);
    if (distance - a.radius - b.radius > kContactDistance)
        return {tg::vec2(1, 0), distance - a.radius - b.radius};

    if (!a.box && !b.box)
        return CircleCircle(a.center, a.radius, b.center, b.radius);

    if (!a.box)
        return CircleBox(a, b);

    if (!b.box)
    {
        auto contact = CircleBox(b, a);
        contact.normal = -contact.normal;
        return contact;
    }

    return BoxBox(a, b);
}

gamedev::CollisionSystem::Contact gamedev::CollisionSystem::CircleCircle(tg::pos2 centerA, float radiusA, tg::pos2 centerB, float radiusB)
{
    auto const d = centerA - centerB;
    auto const distance = tg::length(d);

    Contact contact;
    contact.normal = distance > 1e-6f ? d / distance : tg::vec2(1, 0);
    contact.separation = distance - radiusA - radiusB;
    return contact;
}

gamedev::CollisionSystem::Contact gamedev::CollisionSystem::CircleBox(WorldShape const& circle, WorldShape const& box)
{
    // Box frame: unit axes and half extents
    tg::vec2 axis[2];
    float extent[2];
    for (int i = 0; i < 2; ++i)
    {
        extent[i] = tg::length(box.halfAxes[i]);
        axis[i] = extent[i] > 1e-6f ? box.halfAxes[i] / extent[i] : tg::vec2::zero;
    }

    // Flat boxes: complete the frame with the perpendicular axis
    if (extent[0] <= 1e-6f && extent[1] <= 1e-6f)
        return CircleCircle(circle.center, circle.radius, box.center, 0.f);
    if (extent[0] <= 1e-6f)
        axis[0] = {-axis[1].y, axis[1].x};
    if (extent[1] <= 1e-6f)
        axis[1] = {-axis[0].y, axis[0].x};

    auto const p = circle.center - box.center;
    float const local[2] = {tg::dot(p, axis[0]), tg::dot(p, axis[1])};

    Contact contact;

    // Center inside the box: leave through the nearest face
    if (tg::abs(local[0]) <= extent[0] && tg::abs(local[1]) <= extent[1])
    {
        int const i = (extent[0] - tg::abs(local[0]) < extent[1] - tg::abs(local[1])) ? 0 : 1;
        contact.normal = local[i] < 0.f ? -axis[i] : axis[i];
        contact.separation = tg::abs(local[i]) - extent[i] - circle.radius;
        return contact;
    }

    // Center outside: distance to the closest point on the box
    auto const closest = box.center + axis[0] * tg::clamp(local[0], -extent[0], extent[0]) + axis[1] * tg::clamp(local[1], -extent[1], extent[1]);
    auto const d = circle.center - closest;
    auto const distance = tg::length(d);

    if (distance > 1e-6f)
    {
        contact.normal = d / distance;
    }
    else
    {
        // On the boundary: leave along the axis the center is outside of
        int const i = tg::abs(local[0]) > extent[0] ? 0 : 1;
        contact.normal = local[i] < 0.f ? -axis[i] : axis[i];
    }

    contact.separation = distance - circle.radius;
    return contact;
}

gamedev::CollisionSystem::Contact gamedev::CollisionSystem::BoxBox(WorldShape const& a, WorldShape const& b)
{
    auto const d = a.center - b.center;

    Contact best;
    best.separation = -std::numeric_limits<float>::max();

    // The axis with the largest separation (or least penetration) wins
    for (auto const* box : {&a, &b})
    {
        for (auto const& halfAxis : box->halfAxes)
        {
            auto const length = tg::length(halfAxis);
            if (length <= 1e-6f)
                continue;

            auto const n = halfAxis / length;
            auto const extentA = tg::abs(tg::dot(a.halfAxes[0], n)) + tg::abs(tg::dot(a.halfAxes[1], n));
            auto const extentB = tg::abs(tg::dot(b.halfAxes[0], n)) + tg::abs(tg::dot(b.halfAxes[1], n));
            auto const distance = tg::dot(d, n);
            auto const separation = tg::abs(distance) - extentA - extentB;

            if (separation > best.separation)
            {
                best.separation = separation;
                best.normal = distance < 0.f ? -n : n;
            }
        }
    }

    // Both boxes flat in both directions: points
    if (best.separation == -std::numeric_limits<float>::max())
        return CircleCircle(a.center, 0.f, b.center, 0.f);

    return best;
}

gamedev::CollisionSystem::WorldShape& gamedev::CollisionSystem::CachedShape(InstanceHandle handle)
{
    auto const slot = mECS->GetInstanceManager()->SlotIndex(handle);
    CC_ASSERT_MSG(slot != World::kNoSlot, "Collision shape of a dead instance.");
    return mShapes[slot];
}

void gamedev::CollisionSystem::PositionListener(Event& e)
{
    auto collider = mECS->TryGetComponent<Collider>(e.mSubject);
    if (!collider)
        return;

    // Rotation and scale may have changed too, so the shape and bounds are recomputed
    auto& shape = CachedShape(e.mSubject);
    shape = ComputeWorldShape(e.mSubject);

    auto const& translation = mECS->GetInstanceTransform(e.mSubject).translation;
    auto const bounds = BroadPhaseBounds(shape, translation);

    if (collider->dynamic)
        mDynamicColliderHashMap.UpdateInstance(e.mSubject, translation, bounds);
    else
        mStaticColliderHashMap.UpdateInstance(e.mSubject, translation, bounds);
}

gamedev::CollisionSystem::WorldShape gamedev::CollisionSystem::ComputeWorldShape(InstanceHandle handle)
{
    auto const& xform = mECS->GetInstanceTransform(handle);
    auto const m = xform.transform_mat2D();

    WorldShape shape;

    if (auto const box = mECS->TryGetComponent<BoxShape>(handle))
    {
        auto const center = m * box->box.center;
        auto const axis0 = m * box->box.half_extents[0];
        auto const axis1 = m * box->box.half_extents[2];

        shape.box = true;
        shape.center = {center.x, center.z};
        shape.halfAxes[0] = {axis0.x, axis0.z};
        shape.halfAxes[1] = {axis1.x, axis1.z};
        shape.radius = tg::length(shape.halfAxes[0] + shape.halfAxes[1]);
    }
    else if (auto const circle = mECS->TryGetComponent<CircleShape>(handle))
    {
        auto const center = m * circle->circle.center;

        shape.center = {center.x, center.z};
        shape.radius = circle->circle.radius * tg::max(xform.scaling.width, xform.scaling.depth);
    }
    else
    {
        shape.center = {xform.translation.x, xform.translation.z};
    }

    return shape;
}

tg::aabb3 gamedev::CollisionSystem::BroadPhaseBounds(WorldShape const& shape, tg::vec3 const& translation, float margin)
{
    auto extent = tg::vec2(shape.radius, shape.radius);
    if (shape.box)
    {
        extent.x = tg::abs(shape.halfAxes[0].x) + tg::abs(shape.halfAxes[1].x);
        extent.y = tg::abs(shape.halfAxes[0].y) + tg::abs(shape.halfAxes[1].y);
    }
    extent += tg::vec2(margin, margin);

    auto const offset = shape.center - tg::pos2(translation.x, translation.z);
    return {tg::pos3(offset.x - extent.x, 0, offset.y - extent.y), tg::pos3(offset.x + extent.x, 0, offset.y + extent.y)};
}
