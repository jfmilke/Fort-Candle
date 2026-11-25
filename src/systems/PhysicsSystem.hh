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
 * Integrates the `Physics` components: velocity, gravity (ballistic arcs of arrows) and snapping to the terrain height.
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
     * Moves all entities and flags moved instances (`Instance::new_position`).
     *
     * Before the move it updates `Physics::observedVelocity` and sets `Physics::lastPosition`.
     * Gravity is integrated exactly (constant acceleration), so a launched body follows its parabola regardless of the time step.
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

    /// Gravity in world units per second squared: 9.81 m/s² at about 3.3 m per world unit (a 0.55 units tall person is about 1.8 m).
    static constexpr float kGravity = 9.81f / 3.3f;

    /// Time constant of the smoothing of `Physics::observedVelocity`, in seconds.
    static constexpr float kObservedVelocityTime = 0.1f;

private:
    EngineECS* mECS = nullptr;         ///< ECS (not owned).
    std::shared_ptr<Terrain> mTerrain; ///< Terrain for height queries.
};
}
