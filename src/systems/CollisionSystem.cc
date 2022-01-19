#include "systems/CollisionSystem.hh"
#include <typed-geometry/tg-lean.hh>

void gamedev::CollisionSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    if (mECS->GetComponent<Collider>(handle)->dynamic)
    {
        mDynamicEntities.insert(handle);

        mStaticEntities.erase(handle);

        auto instance = mECS->GetInstance(handle);
        mDynamicColliderHashMap.AddInstance(handle, instance.xform.translation, instance.max_bounds);
        mStaticColliderHashMap.RemoveInstance(handle);
    }
    else
    {
        mStaticEntities.insert(handle);

        mDynamicEntities.erase(handle);

        auto instance = mECS->GetInstance(handle);
        mStaticColliderHashMap.AddInstance(handle, instance.xform.translation, instance.max_bounds);
        mDynamicColliderHashMap.RemoveInstance(handle);
    }
}

void gamedev::CollisionSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature)
{
    mDynamicEntities.erase(handle);
    mStaticEntities.erase(handle);

    mStaticColliderHashMap.RemoveInstance(handle);
    mDynamicColliderHashMap.RemoveInstance(handle);
}

void gamedev::CollisionSystem::RemoveEntity(InstanceHandle& handle)
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

void gamedev::CollisionSystem::Init(std::shared_ptr<EngineECS>& ecs)
{
    mECS = ecs;

    mDynamicColliderHashMap.Init(tg::size2::unit, tg::aabb3::unit_from_zero);
    mStaticColliderHashMap.Init(tg::size2::unit, tg::aabb3::unit_from_zero);

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::NewPosition, CollisionSystem::PositionListener));
}

void gamedev::CollisionSystem::Init(std::shared_ptr<EngineECS>& ecs, tg::aabb3 map_bounds)
{
    mECS = ecs;

    auto size = tg::size_of(map_bounds);

    mDynamicColliderHashMap.Init({size.width, size.depth}, map_bounds);
    mStaticColliderHashMap.Init({size.width, size.depth}, map_bounds);

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::NewPosition, CollisionSystem::PositionListener));
}


int gamedev::CollisionSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    mDT = dt;

    ResolveStaticCollisions();
    ResolveDynamicCollisions();

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
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

        //for (const auto& handle_dynamic : mDynamicEntities)
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

        /*
        if (collision)
        {
            Event collision_event(EventType::StaticCollision, handle_static, handle_dynamic);
            mECS->SendFunctionalEvent(collision_event);
        }
        */
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

        //for (const auto& handle_2 : mDynamicEntities)
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

/*
// Bouncing means mutual displacement
void gamedev::CollisionSystem::bounce(const Shape& shapeA, const Shape& shapeB, float dt)
{
    tg::pos3 centerA; 
    centerA.x = shapeA.aabb.min.x + shapeA.aabb.max.x;
    centerA.z = shapeA.aabb.min.y + shapeA.aabb.max.y;
    centerA += shapeA.instance->xform.translation;

    tg::pos3 centerB;
    centerB.x = shapeB.aabb.min.x + shapeB.aabb.max.x;
    centerB.z = shapeB.aabb.min.y + shapeB.aabb.max.y;
    centerB += shapeB.instance->xform.translation;


    auto dir0 = (centerA - centerB);
    auto dir1 = (centerB - centerA);


    dir0 = tg::normalize_safe(dir0);
    dir1 = tg::normalize_safe(dir1);


    shapeA.instance->xform.translation.x += dt * dir0.x * 8;
    shapeA.instance->xform.translation.z += dt * dir0.z * 8;

    shapeB.instance->xform.translation.x += dt * dir1.x * 8;
    shapeB.instance->xform.translation.z += dt * dir1.z * 8;
}

// Collision means there is one stationary rocklike badass object
void gamedev::CollisionSystem::collide(const Shape& centerDynamic, const Shape& centerStationary, float dt)
{
    tg::pos3 centerA;
    centerA.x = centerDynamic.aabb.min.x + centerDynamic.aabb.max.x;
    centerA.z = centerDynamic.aabb.min.y + centerDynamic.aabb.max.y;
    centerA += centerDynamic.instance->xform.translation;

    tg::pos3 centerB;
    centerB.x = centerStationary.aabb.min.x + centerStationary.aabb.max.x;
    centerB.z = centerStationary.aabb.min.y + centerStationary.aabb.max.y;
    centerB += centerStationary.instance->xform.translation;


    auto dir0 = 8 * tg::normalize_safe(centerA - centerB);

    centerDynamic.instance->xform.translation.x += dt * dir0.x;
    centerDynamic.instance->xform.translation.z += dt * dir0.z;
}

bool gamedev::CollisionSystem::intersects(const Shape& aabb1, const Shape& aabb2)
{
    const auto scale_1 = aabb1.instance->xform.scale_mat2D();
    const auto scale_2 = aabb2.instance->xform.scale_mat2D();

    tg::vec2 translate_1;
    translate_1.x = aabb1.instance->xform.translation.x;
    translate_1.y = aabb1.instance->xform.translation.z;

    tg::vec2 translate_2;
    translate_2.x = aabb2.instance->xform.translation.x;
    translate_2.y = aabb2.instance->xform.translation.z;

    tg::aabb2 ab1 = aabb1.aabb;
    ab1.min = scale_1 * ab1.min + translate_1;
    ab1.max = scale_1 * ab1.max + translate_1;

    tg::aabb2 ab2 = aabb2.aabb;
    ab2.min = scale_2 * ab2.min + translate_2;
    ab2.max = scale_2 * ab2.max + translate_2;

    return intersects(ab1, ab2);
}

bool gamedev::CollisionSystem::intersects(const tg::aabb2& aabb1, const tg::aabb2& aabb2)
{
    bool xMatch = false;
    bool yMatch = false;

    if (aabb1.min.x < aabb2.max.x && aabb1.max.x > aabb2.min.x && aabb1.min.y < aabb2.max.y && aabb1.max.y > aabb2.min.y)
        return 1; // Intersection

    return 0;
}



// Calculates aabb of box and tests intersection between (aabb, circle).
bool gamedev::CollisionSystem::intersects(const tg::box2& box, const tg::sphere2& circle)
{
    // Reference vectors to calculate rotation between box & unit box
    auto b0 = tg::normalize(box.half_extents * tg::vec2::one);
    auto u0 = box.unit_centered.half_extents * tg::vec2::one;

    // Matrix which transforms unit vector b0 into unit vector u0 ("inverse rotate"):
    tg::mat3 invRotate{tg::mat3::identity};
    invRotate[0][0], invRotate[1][1] = b0.x * u0.x + b0.y * u0.y;
    invRotate[1][0] = u0.x * b0.y - b0.x - u0.y;
    invRotate[0][1] = b0.x * u0.y - u0.x - b0.y;

    // box -> AABB
    tg::box2 b;
    b.center = invRotate * box.center;
    b.half_extents = tg::mat2(invRotate * box.half_extents[0], invRotate * box.half_extents[1]);
    
    tg::aabb2 aabb = tg::aabb_of(b);

    return intersects(aabb, circle);
}

bool gamedev::CollisionSystem::intersects(const tg::aabb2& aabb, const tg::sphere2& circle)
{
    tg::pos2 closest(INFINITY, INFINITY);

    if (circle.center.x < aabb.min.x)
        closest.x = aabb.min.x; // left of aabb
    else if (circle.center.x > aabb.max.x)
        closest.x = aabb.max.x; // right of aabb
    else
        closest.x = circle.center.x; // horizontal match


    if (circle.center.y < aabb.min.y)
        closest.y = aabb.min.y; // bottom of aabb
    else if (circle.center.y > aabb.max.y)
        closest.y = aabb.max.y; // top of aabb
    else
        closest.y = circle.center.y; // vertical match

    // If the closest point lies within circle => collision
    if (tg::distance_sqr(circle.center, closest) < (circle.radius * circle.radius))
        return 1; // Intersection

    return 0;
}


bool gamedev::CollisionSystem::intersects(const tg::sphere2& circleA, const tg::sphere2& circleB)
{
    // If distance between both midpoints is less than any radius of both => collision
    auto distance = tg::distance(circleA.center, circleB.center);

    if (distance < circleA.radius + circleB.radius)
        return 1; // Intersection

    return 0;
}

bool gamedev::CollisionSystem::intersects(const BoxShape& box, const CircleShape& circle)
{
    // Box interpreted as AABB, therefore only scaling & translation
    const auto boxScale = box.instance->xform.scale_mat2D();
    const auto boxRot = box.instance->xform.rotation_mat2D();
    const auto boxTrans = tg::translation(tg::vec2(box.instance->xform.translation));

    tg::box2 transformedBox;
    transformedBox.center = boxScale * boxTrans * box.box.center;
    transformedBox.half_extents[0] = boxScale * box.box.half_extents[0];
    transformedBox.half_extents[1] = boxScale * box.box.half_extents[1];

    tg::aabb2 AABB = tg::aabb_of(transformedBox);

    // As box is AABB, circle needs to account for the box' rotation & translation:
    // 1. Transform circle by its own means
    // 2. Rotate circle back by inverse rotation of box around box center (since both assume (0,0) center just rotate, no need for prior translation to box center)
    tg::sphere2 transformedCircle;
    transformedCircle.center = circle.instance->xform.transform_mat2D() * tg::transpose(boxRot) * circle.circle.center;
    transformedCircle.radius = std::max(circle.instance->xform.scaling.width, circle.instance->xform.scaling.depth) * circle.circle.radius;

    return intersects(AABB, transformedCircle);
}

bool gamedev::CollisionSystem::intersects(const CircleShape& circleA, const CircleShape& circleB)
{ 
    // If distance between both midpoints is less than any radius => collision
    tg::sphere2 cA;
    cA.center = circleA.circle.center + tg::vec2(circleA.instance->xform.translation);
    cA.radius = std::max(circleA.instance->xform.scaling.width, circleA.instance->xform.scaling.depth) * circleA.circle.radius;

    tg::sphere2 cB;
    cB.center = circleB.circle.center + tg::vec2(circleB.instance->xform.translation);
    cB.radius = std::max(circleB.instance->xform.scaling.width, circleB.instance->xform.scaling.depth) * circleB.circle.radius;
    auto centerB = tg::pos2(circleB.instance->xform.translation);

    return intersects(cA, cB);
}
*/

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
    //if (distance > (radius1 + radius2) + (radius1 + radius2))
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

    // Assert, that box1 is pushed away from box2
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

    // Assert, that box1 is pushed away from box2
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

            //physics1->velocity = friction_damping * friction;
            instance_dynamic.xform.move_absolute((friction_damping * friction) * mDT);

            return;
        }
    }
    
    instance_dynamic.xform.move_absolute(push); 

    // Old stuff
    //{
    //    const float bounce_damping = 0.5;
    //    const float friction_damping = 1.0;
    //
    //    // 1. Push dynamic object outside
    //    instance_dynamic.xform.move_absolute(push);
    //
    //    if (physics1)
    //    {
    //        tg::vec3 N = tg::normalize(push);
    //
    //        tg::vec3 vec_collision = physics1->velocity;
    //
    //        // todo: fix bounce
    //        // if (physics2)
    //        //    vec_collision -= physics2->velocity;
    //
    //        tg::vec3 bounce = tg::dot(vec_collision, N) * N; // perpendicular: bounce
    //        tg::vec3 friction = vec_collision - bounce;
    //
    //        physics1->velocity = bounce_damping * bounce + friction_damping * friction;
    //    }
    //}
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