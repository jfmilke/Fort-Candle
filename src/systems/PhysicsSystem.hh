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
 * Moved instances get `Instance::new_position`; `EngineECS::PublishPositions` publishes them after the collision step.
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
     * Moves all entities, sets `Physics::lastPosition` before the move and flags moved instances (`Instance::new_position`).
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

private:
    const float mGravity = -9.81f; ///< Gravity in world units per second squared.

private:
    EngineECS* mECS = nullptr;         ///< ECS (not owned).
    std::shared_ptr<Terrain> mTerrain; ///< Terrain for height queries.
};
}
