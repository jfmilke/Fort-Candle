#include "systems/CollisionSystem.hh"
#include <typed-geometry/tg-lean.hh>

void gamedev::CollisionSystem::AddEntity(InstanceHandle handle, Signature entitySignature)
{
    if (mECS->GetComponent<Collider>(handle)->dynamic)
    {
        mDynamicEntities.insert(handle);

        mStaticEntities.erase(handle);

        auto const& instance = mECS->GetInstance(handle);
        mDynamicColliderHashMap.AddInstance(handle, instance.xform.translation, instance.max_bounds);
        mStaticColliderHashMap.RemoveInstance(handle);
    }
    else
    {
        mStaticEntities.insert(handle);

        mDynamicEntities.erase(handle);

        auto const& instance = mECS->GetInstance(handle);
        mStaticColliderHashMap.AddInstance(handle, instance.xform.translation, instance.max_bounds);
        mDynamicColliderHashMap.RemoveInstance(handle);
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

void gamedev::CollisionSystem::Init(EngineECS& ecs)
{
    mECS = &ecs;

    mDynamicColliderHashMap.Init(tg::size2::unit, tg::aabb3::unit_from_zero);
    mStaticColliderHashMap.Init(tg::size2::unit, tg::aabb3::unit_from_zero);

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::NewPosition, CollisionSystem::PositionListener));
}

void gamedev::CollisionSystem::Init(EngineECS& ecs, tg::aabb3 map_bounds)
{
    mECS = &ecs;

    auto size = tg::size_of(map_bounds);

    mDynamicColliderHashMap.Init({size.width, size.depth}, map_bounds);
    mStaticColliderHashMap.Init({size.width, size.depth}, map_bounds);

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::NewPosition, CollisionSystem::PositionListener));
}


void gamedev::CollisionSystem::Update(float dt)
{
    mDT = dt;

    ResolveStaticCollisions();
    ResolveDynamicCollisions();
}

void gamedev::CollisionSystem::ResolveStaticCollisions()
{
    tg::pos3 min{0, 0, 0};
    tg::pos3 max{0, 0, 0};

    std::queue<std::pair<InstanceHandle, InstanceHandle>> staticDynamic_2nd;

    mSecondRun = false;

    for (const auto& handle_static : mStaticEntities)
    {
        // Lifecheck:
        auto hp = mECS->TryGetComponent<Destructible>(handle_static);
        if (hp)
        {
            if (hp->health.current <= 0.0f)
            {
                mECS->MarkDestruction(handle_static);
                // TODO: This event is never sent.
                Event e(EventType::BuildingDestroyed, handle_static, handle_static);
            }
        }

        // Collision Check:
        auto boxShape_s = mECS->TryGetComponent<BoxShape>(handle_static);
        auto circleShape_s = mECS->TryGetComponent<CircleShape>(handle_static);
        auto& instance_s = mECS->GetInstance(handle_static);

        auto bounds = instance_s.max_bounds;
        bounds.max *= instance_s.xform.scaling;
        bounds.min *= instance_s.xform.scaling;

        for (const auto& handle_dynamic : mDynamicColliderHashMap.FindNear(instance_s.xform.translation, instance_s.max_bounds))
        {
            if (mECS->TestSignature<InTower>(handle_dynamic))
                continue;

            auto collider = mECS->TryGetComponent<Collider>(handle_dynamic);

            if ((!collider) || (!collider->dynamic))
                continue;

            auto& instance_d = mECS->GetInstance(handle_dynamic);

            auto boxShape_d = mECS->TryGetComponent<BoxShape>(handle_dynamic);
            auto circleShape_d = mECS->TryGetComponent<CircleShape>(handle_dynamic);

            bool collision = false;

            if (boxShape_s && boxShape_d)
                collision = Collide(instance_d, instance_s, boxShape_d, boxShape_s);

            else if (boxShape_s && circleShape_d)
                collision = Collide(instance_d, instance_s, circleShape_d, boxShape_s);

            else if (circleShape_s && boxShape_d)
                collision = Collide(instance_d, instance_s, boxShape_d, circleShape_s);

            else if (circleShape_s && circleShape_d)
                collision = Collide(instance_d, instance_s, circleShape_d, circleShape_s);

            if (collision)
            {
                staticDynamic_2nd.push({handle_static, handle_dynamic});

                Event collision_event(EventType::StaticCollision, handle_static, handle_dynamic);
                mECS->SendFunctionalEvent(collision_event);
            }
        }
    }

    mSecondRun = true;

    for (int i = 0; i < staticDynamic_2nd.size(); i++)
    {
        auto handle_static = staticDynamic_2nd.front().first;
        auto handle_dynamic = staticDynamic_2nd.front().second;
        staticDynamic_2nd.pop();

        // Collision Check:
        auto boxShape_s = mECS->TryGetComponent<BoxShape>(handle_static);
        auto circleShape_s = mECS->TryGetComponent<CircleShape>(handle_static);
        auto& instance_s = mECS->GetInstance(handle_static);

        auto bounds = instance_s.max_bounds;
        bounds.max *= instance_s.xform.scaling;
        bounds.min *= instance_s.xform.scaling;

        auto collider = mECS->TryGetComponent<Collider>(handle_dynamic);

        if ((!collider) || (!collider->dynamic))
            continue;

        auto& instance_d = mECS->GetInstance(handle_dynamic);

        auto boxShape_d = mECS->TryGetComponent<BoxShape>(handle_dynamic);
        auto circleShape_d = mECS->TryGetComponent<CircleShape>(handle_dynamic);

        bool collision = false;

        if (boxShape_s && boxShape_d)
            collision = Collide(instance_d, instance_s, boxShape_d, boxShape_s);

        else if (boxShape_s && circleShape_d)
            collision = Collide(instance_d, instance_s, circleShape_d, boxShape_s);

        else if (circleShape_s && boxShape_d)
            collision = Collide(instance_d, instance_s, boxShape_d, circleShape_s);

        else if (circleShape_s && circleShape_d)
            collision = Collide(instance_d, instance_s, circleShape_d, circleShape_s);
    }
}

void gamedev::CollisionSystem::ResolveDynamicCollisions()
{
    tg::pos3 min{0, 0, 0};
    tg::pos3 max{0, 0, 0};

    mSecondRun = false;

    for (const auto& handle_1 : mDynamicEntities)
    {
        if (mECS->TestSignature<InTower>(handle_1))
            continue;

        auto boxShape_1 = mECS->TryGetComponent<BoxShape>(handle_1);
        auto circleShape_1 = mECS->TryGetComponent<CircleShape>(handle_1);
        auto& instance_1 = mECS->GetInstance(handle_1);

        for (const auto& handle_2 : mDynamicColliderHashMap.FindNear(instance_1.xform.translation, instance_1.max_bounds))
        {
            if (handle_1 == handle_2)
                continue;

            auto collider = mECS->TryGetComponent<Collider>(handle_2);
            if ((!collider) || (!collider->dynamic))
                continue;

            auto& instance_2 = mECS->GetInstance(handle_2);

            auto boxShape_2 = mECS->TryGetComponent<BoxShape>(handle_2);
            auto circleShape_2 = mECS->TryGetComponent<CircleShape>(handle_2);

            bool collision = false;

            // TODO: The two mixed cases pass a null shape of the wrong kind. All dynamic colliders (units) use box
            // shapes, so only the box-box case is reached.
            if (boxShape_1 && boxShape_2)
                collision = Collide(instance_1, instance_2, boxShape_1, boxShape_2);

            else if (boxShape_1 && circleShape_2)
                collision = Collide(instance_1, instance_2, circleShape_1, boxShape_2);

            else if (circleShape_1 && boxShape_2)
                collision = Collide(instance_1, instance_2, boxShape_1, circleShape_2);

            else if (circleShape_1 && circleShape_2)
                collision = Collide(instance_1, instance_2, circleShape_1, circleShape_2);
        }
    }
}

bool gamedev::CollisionSystem::Collide(Instance& instance_dynamic, Instance& instance_any, const BoxShape* box_dynamic, const BoxShape* box_any)
{
    // Transform collision box
    const auto transform1 = affine_to_mat4(instance_dynamic.xform.transform_mat());
    auto tBox1 = box_dynamic->box;
    tBox1.center = transform1 * box_dynamic->box.center;
    tBox1.half_extents[0] = transform1 * box_dynamic->box.half_extents[0];
    tBox1.half_extents[2] = transform1 * box_dynamic->box.half_extents[2];

    const auto transform2 = affine_to_mat4(instance_any.xform.transform_mat());
    auto tBox2 = box_any->box;
    tBox2.center = transform2 * box_any->box.center;
    tBox2.half_extents[0] = transform2 * box_any->box.half_extents[0];
    tBox2.half_extents[2] = transform2 * box_any->box.half_extents[2];

    // Interpret as circles to do fast collision check
    auto distance = tg::distance(tBox1.center, tBox2.center);
    auto radius1 = tg::length(tBox1.half_extents[0] + tBox1.half_extents[2]);
    auto radius2 = tg::length(tBox2.half_extents[0] + tBox2.half_extents[2]);
    if (distance > (radius1 + radius2))
        return 0; // No Intersection

    tg::vec3 push = minimalPush(tBox1, tBox2);

    // If the push-vector is zero, no collision happend.
    if ((push.x == 0) && (push.z == 0))
        return 0;

    resolve(instance_dynamic, instance_any, push);

    return 1;
}

bool gamedev::CollisionSystem::Collide(Instance& instance_dynamic, Instance& instance_any, const BoxShape* box_dynamic, const CircleShape* circle_static)
{
    // Transform collision box
    const auto transform1 = instance_dynamic.xform.transform_mat2D();
    tg::box3 tBox;
    tBox.center = transform1 * box_dynamic->box.center;
    tBox.half_extents[0] = transform1 * box_dynamic->box.half_extents[0];
    tBox.half_extents[2] = transform1 * box_dynamic->box.half_extents[2];

    const auto transform2 = instance_any.xform.transform_mat2D();
    const auto scale2 = tg::max(instance_any.xform.scaling.width, instance_any.xform.scaling.depth);
    tg::sphere3 tCircle;
    tCircle.center = transform2 * circle_static->circle.center;
    tCircle.radius = scale2 * circle_static->circle.radius;
    
    // Interpret as circles to do fast collision check
    auto distance = tg::distance_sqr(tBox.center, tCircle.center);
    auto radius1 = tg::length(tBox.half_extents[0] + tBox.half_extents[2]);
    if (distance > (radius1 + tCircle.radius) * (radius1 + tCircle.radius))
        return 0; // No Intersection

    tg::vec3 push = minimalPush(tBox, tCircle);

    // If the push-vector is zero, no collision happend.
    if ((push.x == 0) && (push.z == 0))
        return 0;

    resolve(instance_dynamic, instance_any, push);

    return 1;
}

bool gamedev::CollisionSystem::Collide(Instance& instance_dynamic, Instance& instance_any, const CircleShape* circle_dynamic, const BoxShape* box_static)
{
    // Transform collision box
    const auto transform1 = instance_any.xform.transform_mat2D();
    tg::box3 tBox;
    tBox.center = transform1 * box_static->box.center;
    tBox.half_extents[0] = transform1 * box_static->box.half_extents[0];
    tBox.half_extents[2] = transform1 * box_static->box.half_extents[2];

    const auto transform2 = instance_dynamic.xform.transform_mat2D();
    const auto scale2 = tg::max(instance_dynamic.xform.scaling.width, instance_dynamic.xform.scaling.depth);
    tg::sphere3 tCircle;
    tCircle.center = transform2 * circle_dynamic->circle.center;
    tCircle.radius = scale2 * circle_dynamic->circle.radius;

    // Interpret as circles to do fast collision check
    auto distance = tg::distance_sqr(tBox.center, tCircle.center);
    auto radius1 = tg::length(tBox.half_extents[0] + tBox.half_extents[2]);
    if (distance > (radius1 + tCircle.radius) * (radius1 + tCircle.radius))
        return 0; // No Intersection

    tg::vec3 push = minimalPush(tCircle, tBox);

    // If the push-vector is zero, no collision happend.
    if ((push.x == 0) && (push.z == 0))
        return 0;

    resolve(instance_dynamic, instance_any, push);

    return 1;
}

bool gamedev::CollisionSystem::Collide(Instance& instance_dynamic, Instance& instance_any, const CircleShape* circle_dynamic, const CircleShape* circle_static)
{
    // Transform collision box
    const auto transform1 = instance_dynamic.xform.transform_mat2D();
    const auto scale1 = tg::max(instance_dynamic.xform.scaling.width, instance_dynamic.xform.scaling.depth);
    tg::sphere3 tCircle1;
    tCircle1.center = transform1 * circle_dynamic->circle.center;
    tCircle1.radius = scale1 * circle_dynamic->circle.radius;

    const auto transform2 = instance_any.xform.transform_mat2D();
    const auto scale2 = tg::max(instance_any.xform.scaling.width, instance_any.xform.scaling.depth);
    tg::sphere3 tCircle2;
    tCircle2.center = transform2 * circle_static->circle.center;
    tCircle2.radius = scale2 * circle_static->circle.radius;

    // Do fast circle collision check
    auto distance = tg::distance_sqr(tCircle1.center, tCircle2.center);
    if (distance > (tCircle1.radius + tCircle2.radius) * (tCircle1.radius + tCircle2.radius))
        return 0; // No Intersection

    tg::vec3 push = minimalPush(tCircle1, tCircle2);

    // If the push-vector is zero, no collision happend.
    if ((push.x == 0) && (push.z == 0))
        return 0;

    resolve(instance_dynamic, instance_any, push);

    return 1;
}

tg::vec3 gamedev::CollisionSystem::SAT_AxisTest(const tg::vec3& axis, const std::array<tg::pos3, 4>& points1, const std::array<tg::pos3, 4>& points2)
{
    float min1 = INFINITY;
    float max1 = -INFINITY;

    float min2 = INFINITY;
    float max2 = -INFINITY;

    // Project all points on the given axis & find maximum/minimum
    for (const auto& p : points1)
    {
        float projection = tg::dot(p, axis);

        min1 = tg::min(min1, projection);
        max1 = tg::max(max1, projection);
    }

    for (const auto& p : points2)
    {
        float projection = tg::dot(p, axis);

        min2 = tg::min(min2, projection);
        max2 = tg::max(max2, projection);
    }

    // On overlap, create a push-vector along the given axis
    if ((max1 >= min2) && (max2 >= min1))
    {
        // Push a bit more than needed
        float d = tg::min(max2 - min1, max1 - min2) / tg::dot(axis, axis) + 1e-10;
        return d * axis;
    }

    // Else, return a zero-vector (equal to: no overlap found).
    return tg::vec3::zero;
}

tg::vec3 gamedev::CollisionSystem::minimalPush(const tg::box3& box_dynamic, const tg::box3& box_any)
{
    // half_extents are already the axes
    auto axes = std::array<tg::vec3, 4>();
    axes[0] = box_dynamic.half_extents[0]; // boxA
    axes[1] = box_dynamic.half_extents[2]; //
    axes[2] = box_any.half_extents[0];     // boxB
    axes[3] = box_any.half_extents[2];     //

    // points must be calculated from the center
    auto center1 = box_dynamic.center;
    auto center2 = box_any.center;
    std::array<tg::pos3, 4> points1;
    std::array<tg::pos3, 4> points2;

    points1[0] = center1 + box_dynamic.half_extents[0] + box_dynamic.half_extents[2];
    points1[1] = center1 + box_dynamic.half_extents[0] - box_dynamic.half_extents[2];
    points1[2] = center1 - box_dynamic.half_extents[0] + box_dynamic.half_extents[2];
    points1[3] = center1 - box_dynamic.half_extents[0] - box_dynamic.half_extents[2];

    points2[0] = center2 + box_any.half_extents[0] + box_any.half_extents[2];
    points2[1] = center2 + box_any.half_extents[0] - box_any.half_extents[2];
    points2[2] = center2 - box_any.half_extents[0] + box_any.half_extents[2];
    points2[3] = center2 - box_any.half_extents[0] - box_any.half_extents[2];

    // SAT Test all axis for a potential collision. Break if a separting axis is found.
    // push is the vector, which holds the displacement to avoid collision on this axis.
    // push == (0,0) => separating axis / no collision
    // push  > (0,0) => overlap
    std::vector<tg::vec3> push_vectors;
    for (const auto& axis : axes)
    {
        auto push = SAT_AxisTest(axis, points1, points2);
        if (push.x != 0 || push.z != 0)
            push_vectors.push_back(push);
        else
            return tg::vec3::zero;
    }

    // Get minimum necessary vector to resolve collision.
    tg::vec3 minimum_push;
    float minimum = INFINITY;
    for (const auto& push : push_vectors)
    {
        if (tg::dot(push, push) < minimum)
        {
            minimum_push = push;
            minimum = tg::dot(push, push);
        }
    }

    // Ensure that the dynamic shape is pushed away from the other one
    tg::vec3 d = box_any.center - box_dynamic.center;
    if (tg::dot(d, minimum_push) > 0)
        minimum_push *= -1;

    return minimum_push;
}


tg::vec3 gamedev::CollisionSystem::minimalPush(const tg::box3& box_dynamic, const tg::sphere3& circle_any)
{
    // half_extents are already the axes
    auto axes = std::array<tg::vec3, 2>();
    axes[0] = tg::normalize_safe(box_dynamic.half_extents[0]);
    axes[1] = tg::normalize_safe(box_dynamic.half_extents[2]);

    // points must be calculated from the center
    auto center1 = box_dynamic.center;
    auto center2 = circle_any.center;
    std::array<tg::pos3, 4> points1;
    std::array<tg::pos3, 4> points2;

    points1[0] = center1 + box_dynamic.half_extents[0] + box_dynamic.half_extents[2];
    points1[1] = center1 + box_dynamic.half_extents[0] - box_dynamic.half_extents[2];
    points1[2] = center1 - box_dynamic.half_extents[0] + box_dynamic.half_extents[2];
    points1[3] = center1 - box_dynamic.half_extents[0] - box_dynamic.half_extents[2];

    // Circle is interpreted as a rectangle for collision, but it works
    points2[0] = center2 + circle_any.radius * axes[0];
    points2[1] = center2 + circle_any.radius * axes[1];
    points2[2] = center2 - circle_any.radius * axes[0];
    points2[3] = center2 - circle_any.radius * axes[1];

    // SAT Test all axis for a potential collision. Break if a separting axis is found.
    // push is the vector, which holds the displacement to avoid collision on this axis.
    // push == (0,0) => separating axis / no collision
    // push  > (0,0) => overlap
    std::vector<tg::vec3> push_vectors;
    for (const auto& axis : axes)
    {
        auto push = SAT_AxisTest(axis, points1, points2);
        if (push.x > 0 || push.z > 0)
            push_vectors.push_back(push);
        else
            return tg::vec3::zero;
    }

    // Get minimum necessary vector to resolve collision.
    tg::vec3 minimum_push;
    float minimum = INFINITY;
    for (const auto& push : push_vectors)
    {
        if (tg::dot(push, push) < minimum)
        {
            minimum_push = push;
            minimum = tg::dot(push, push);
        }
    }

    // Ensure that the dynamic shape is pushed away from the other one
    tg::vec3 d = circle_any.center - box_dynamic.center;
    if (tg::dot(d, minimum_push) > 0)
        minimum_push *= -1;

    return minimum_push;
}


tg::vec3 gamedev::CollisionSystem::minimalPush(const tg::sphere3& circle_dynamic, const tg::box3& box_static)
{
    return -1.0 * minimalPush(box_static, circle_dynamic);
}

tg::vec3 gamedev::CollisionSystem::minimalPush(const tg::sphere3& circle_dynamic, const tg::sphere3& circle_static)
{
    tg::vec3 dir = circle_dynamic.center - circle_static.center;
    float len = tg::length(dir);

    if (len >= circle_dynamic.radius + circle_static.radius)
        return tg::vec3::zero;

    dir /= len;

    tg::vec3 push = dir * tg::abs(len - circle_dynamic.radius - circle_static.radius);

    return push;
}

void gamedev::CollisionSystem::resolve(Instance& instance_dynamic, Instance& instance_any, tg::vec3 push)
{
    auto physics1 = mECS->TryGetComponent<Physics>(instance_dynamic.mHandle);

    if (physics1)
    {
        if (!mSecondRun && tg::any(tg::comp<3, bool>(physics1->velocity)))
        {
            instance_dynamic.xform.move_absolute(physics1->lastPosition - instance_dynamic.xform.translation);

            const float bounce_damping = 0.0;
            const float friction_damping = 0.4;

            tg::vec3 N = tg::normalize(push);
            tg::vec3 vec_collision = tg::normalize_safe(physics1->velocity);

            // todo: fix bounce
            // if (physics2)
            //    vec_collision -= physics2->velocity;

            tg::vec3 bounce = tg::dot(vec_collision, N) * N; // perpendicular: bounce
            tg::vec3 friction = vec_collision - bounce;

            instance_dynamic.xform.move_absolute((friction_damping * friction) * mDT);

            return;
        }
    }
    
    instance_dynamic.xform.move_absolute(push); 
}

void gamedev::CollisionSystem::PositionListener(Event& e)
{
    auto collider = mECS->TryGetComponent<Collider>(e.mSubject);

    if (collider)
    {
        if (collider->dynamic)
            mDynamicColliderHashMap.UpdateInstance(e.mSender, mECS->GetInstanceTransform(e.mSender).translation);
        else
            mStaticColliderHashMap.UpdateInstance(e.mSender, mECS->GetInstanceTransform(e.mSender).translation);
    }
}
