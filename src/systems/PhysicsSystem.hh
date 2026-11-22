#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"
#include "Terrain.hh"

#include <array>

namespace gamedev
{
/**
 * Integrates the `Physics` components: velocity, gravity, arrow arcs and snapping to the terrain height.
 *
 * Entities: all instances with a `Physics` component; units inside a tower (`InTower`) are skipped.
 *
 *  - It publishes the positions of instances with `Instance::new_position`: it sends `EventType::NewPosition`
 *    (so the `CollisionSystem` updates its hash maps) and updates the ECS spatial hash map (see the TODO on `Update`).
 *  - On `EventType::StaticCollision` it resets the subject's `Physics::lastPosition`.
 */
class PhysicsSystem : public System
{
public:
    /**
     * Stores the ECS and registers the collision listener.
     *
     * @param ecs The ECS; must outlive this object.
     */
    void Init(EngineECS& ecs);

    /// Sets the terrain used for `Physics::forceGround`.
    void SetTerrain(std::shared_ptr<Terrain>& t);


    /**
     * Moves all entities and publishes their positions.
     *
     * TODO: Position updates are published on 9 of every 10 calls (`mUpdateControl % 10` is non-zero); probably meant to be 1 of 10.
     * `Instance::new_position` is also set at creation and never reset, so every entity publishes, moving or not.
     *
     * @param dt Time step in seconds.
     * @return Elapsed time in microseconds.
     */
    int Update(float dt);

private:
    /**
     * Moves an instance in the ECS spatial hash map.
     *
     * @param handle Instance to move.
     * @param translation Its new world position.
     */
    void UpdateHashmap(InstanceHandle handle, tg::vec3& translation);

    /**
     * `EventType::StaticCollision` listener: sets the subject's `lastPosition` to its current position.
     *
     * @param e Collision event; the subject is the dynamic collider.
     */
    void CollisionListener(Event& e);

private:
    int mUpdateControl = 0; ///< Update counter used to throttle position updates.

private:
    const float mGravity = -9.81f; ///< Gravity in world units per second squared.
    const float mDamping = 0.02f;  ///< Unused.
    float last_dt = 0.0;           ///< Unused.

private:
    EngineECS* mECS = nullptr;         ///< ECS (not owned).
    std::shared_ptr<Terrain> mTerrain; ///< Terrain for height queries.
};
}
