#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"

namespace gamedev
{
/**
 * Steers entities with an `Input` component towards a target position or focus instance by setting their velocity.
 *
 * Unused: `Game` registers neither this system nor the `Input` component; unit movement is done by the `GamelogicSystem`.
 */
class InputSystem : public System
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;


    /**
     * Updates focus targets and points the velocity of each entity towards its `Input::targetMove` at `Input::runningSpeed`.
     *
     * TODO: The velocity only changes when its direction differs from the move direction (cross product),
     * so an entity at rest never starts moving, and nothing stops it at the target.
     *
     * @param dt Unused.
     */
    void Update(float dt);

    /// Sets the position `handle` should move to.
    void InstructMove(InstanceHandle handle, tg::pos3 targetPosition);

    /// Sets the direction `handle` should look in.
    void InstructLook(InstanceHandle handle, tg::vec3 targetDirection);

    /**
     * Lets one entity follow and/or look at another; stops focusing if both flags are false.
     *
     * TODO: Does not store `passive_handle` in `Input::focusInstance`, so `Update` does not keep tracking it.
     *
     * @param active_handle Entity with an `Input` component.
     * @param passive_handle Instance to follow or look at.
     * @param focusMove Move to the instance's current position.
     * @param focusLook Look towards the instance.
     */
    void InstructFocus(InstanceHandle active_handle, InstanceHandle passive_handle, bool focusMove, bool focusLook);

    /// Sets the move target to the current position.
    void InstructStopInput(InstanceHandle handle);

    /// Stops following/looking at the focus instance and sets the move target to the current position.
    void InstructStopFocus(InstanceHandle handle);


private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).
};
}
