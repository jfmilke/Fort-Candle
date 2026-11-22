#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

namespace gamedev
{
/**
 * Steers entities with an `Input` component towards a target position or focus instance by setting their velocity.
 *
 * Unused: `Game` registers neither this system nor the `Input` component.
 * Unit movement is done by the `GamelogicSystem`.
 */
class InputSystem : public System
{
public:
    /// Stores the ECS.
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();


    /**
     * Updates focus targets and points the velocity of each entity towards its `Input::targetMove` at
     * `Input::runningSpeed`. Returns the elapsed time in microseconds.
     */
    int Update(float dt);

    /// Sets the position `handle` should move to.
    void InstructMove(InstanceHandle handle, tg::pos3 targetPosition);

    /// Sets the direction `handle` should look in.
    void InstructLook(InstanceHandle handle, tg::vec3 targetDirection);

    /**
     * Lets `active_handle` follow (`focusMove`) and/or look at (`focusLook`) `passive_handle`; stops focusing if both
     * are false.
     */
    void InstructFocus(InstanceHandle active_handle, InstanceHandle passive_handle, bool focusMove, bool focusLook);

    /// Sets the move target to the current position.
    void InstructStopInput(InstanceHandle handle);

    /// Stops following/looking at the focus instance and sets the move target to the current position.
    void InstructStopFocus(InstanceHandle handle);



private:
    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.
};
}
