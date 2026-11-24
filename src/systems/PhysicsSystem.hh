#pragma once

#include <glow/fwd.hh>
#include "Terrain.hh"
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

#include <array>

namespace gamedev
{
/**
 * Integrates the `Physics` components: velocity, gravity, arrow arcs and snapping to the terrain height.
 *
 * Entities: all instances with a `Physics` component; units inside a tower (`InTower`) are skipped.
 *
 *  - It publishes the positions of instances with `Instance::new_position`: it sends `EventType::NewPosition`
 *    (so the `CollisionSystem` updates its hash maps) and updates the ECS spatial hash map.
 */
class PhysicsSystem : public System
{
public:
    /**
     * Stores the ECS.
     *
     * @param ecs The ECS; must outlive this object.
     */
    void Init(EngineECS& ecs);

    /// Sets the terrain used for `Physics::forceGround`.
    void SetTerrain(std::shared_ptr<Terrain>& t);


    /**
     * Moves all entities and publishes the positions that changed (`Instance::new_position`), then resets the flag.
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

private:
    /**
     * Moves an instance in the ECS spatial hash map.
     *
     * @param handle Instance to move.
     * @param translation Its new world position.
     */
    void UpdateHashmap(InstanceHandle handle, tg::vec3& translation);

private:
    const float mGravity = -9.81f; ///< Gravity in world units per second squared.

private:
    EngineECS* mECS = nullptr;         ///< ECS (not owned).
    std::shared_ptr<Terrain> mTerrain; ///< Terrain for height queries.
};
}
