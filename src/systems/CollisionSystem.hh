#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "components/ShapeComp.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

/***
* Tests for 2D collisions between CollisionShapes or tg::box2, tg::circle2 and tg::aabb2.
* 
* If a CollisionShape is tested it applies the transformations of the Instance first.
* Then it calls an appropriate method of a tg object.
* 
* If a tg object method is called, it is assumed, that all transformations have been applied to the object.
* 
***/

namespace gamedev
{
class CollisionSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);
    void Init(std::shared_ptr<EngineECS>& ecs, tg::aabb3 map_bounds);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    int Update(float dt);

    void ResolveStaticCollisions();
    void ResolveDynamicCollisions();

    bool intersects(const BoxShape* boxA, const BoxShape* boxB);
    bool intersects(const BoxShape* box, const CircleShape* circle);
    bool intersects(const CircleShape* circle, const BoxShape* box) { return intersects(box, circle); }
    bool intersects(const CircleShape* circleA, const CircleShape* circleB);

    bool intersects(const tg::box3& boxA, const tg::box3& boxB);
    bool intersects(const tg::box3& box, const tg::sphere3& circle);
    bool intersects(const tg::sphere3& circle, const tg::box3& box) { return intersects(box, circle); }
    bool intersects(const tg::sphere3& circleA, const tg::sphere3& circleB);

    // // ToDo: Give a position of intersection
    // tg::pos2 intersection(const BoxShape& boxA, const BoxShape& boxB);
    // tg::pos2 intersection(const BoxShape& box, const CircleShape& circle);
    // tg::pos2 intersection(const CircleShape& circle, const BoxShape& box);
    // tg::pos2 intersection(const CircleShape& circleA, const CircleShape& circleB);

private:
    float mDT = 0.0;

    // Tests if a collision happens & resolves it.
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box1, const BoxShape* box2);
    bool Collide(Instance& instance_dyn, Instance& instance_any, const BoxShape* box, const CircleShape* circle);
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle, const BoxShape* box);
    bool Collide(Instance& instance_dyn, Instance& instance_any, const CircleShape* circle1, const CircleShape* circle2);

    void resolve(Instance& instance1, Instance& instance2, tg::vec3 push);

    // If a collision happens, a minimum push-vector is returned which pushes box1 away from box2.
    // Else, a 0-vector is returned.
    tg::vec3 minimalPush(const tg::box3& box1, const tg::box3& box2);
    tg::vec3 minimalPush(const tg::sphere3& circle1, const tg::sphere3& circle2);
    tg::vec3 minimalPush(const tg::box3& box, const tg::sphere3& circle);
    tg::vec3 minimalPush(const tg::sphere3& circle_dynamic, const tg::box3& box_static);

    // Helper method which is part of Collide(box, box).
    tg::vec3 SAT_AxisTest(const tg::vec3& axis, const std::array<tg::pos3, 4>& points1, const std::array<tg::pos3, 4>& points2);

    // http://www.metanetsoftware.com/technique/tutorialA.html#section5
    // Collision Response Approach
    // 1. Project our of collision
    // 2. Split velocity vector into two components: parallel & perpendicular to the collision surface
    // 3. Calculate bounce using perpendicular component
    // 4. Calculate friction using parallel component

private:
    void PositionListener(Event& e);

private:
    std::shared_ptr<EngineECS> mECS;

    std::set<InstanceHandle> mStaticEntities;
    std::set<InstanceHandle> mDynamicEntities;

    bool mSecondRun = false;


    Hash2D mStaticColliderHashMap;
    Hash2D mDynamicColliderHashMap;
};
}

