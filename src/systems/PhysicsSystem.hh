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
 *  - After moving an instance it sends `EventType::NewPosition` (so the `CollisionSystem` updates its hash maps) and
 *    updates the ECS spatial hash map.
 *  - On `EventType::StaticCollision` it resets the subject's `Physics::lastPosition`.
 */
class PhysicsSystem : public System
{
public:
    /// Stores the ECS and registers the collision listener.
    void Init(std::shared_ptr<EngineECS>& ecs);

    /// Sets the terrain used for `Physics::forceGround`.
    void SetTerrain(std::shared_ptr<Terrain>& t);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /**
     * Moves all entities by `dt` seconds. Returns the elapsed time in microseconds.
     *
     * TODO: Position updates are published on 9 of every 10 calls (`mUpdateControl % 10` is non-zero); probably meant
     * to be 1 of 10. `Instance::new_position` is also never reset, so every instance that moved once keeps publishing.
     */
    int Update(float dt);

private:
    /// Moves `handle` to `translation` in the ECS spatial hash map.
    void UpdateHashmap(InstanceHandle handle, tg::vec3& translation);

    /// `EventType::StaticCollision` listener: sets the subject's `lastPosition` to its current position.
    void CollisionListener(Event& e);

private:
    int mUpdateControl = 0; ///< Update counter used to throttle position updates.

private:
    const float mGravity = -9.81f; ///< Gravity in world units per second squared.
    const float mDamping = 0.02f;  ///< Unused.
    float last_dt = 0.0;           ///< Unused.

private:
    std::shared_ptr<EngineECS> mECS;   ///< Shared ECS.
    std::shared_ptr<Terrain> mTerrain; ///< Terrain for height queries.
};
}
