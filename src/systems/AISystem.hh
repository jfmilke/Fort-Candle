#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"

/* AI System
 * Holds handles of all objects, which qualify for this system.
 * Qualification is defined by Components in Game.cc and whenever a qualifying Component is added or removed
 * from an entity, the handle gets either added or removed from the system.
 *
 * To define a qualifying component just do something like this (in Game.cc):
 *    Signature signature;
 *    signature.set(mECS->GetComponentType<gamedev::SpecialComponent>());
 *    mECS->SetSystemSignature<gamedev::AISystem>(signature);
 *
 * Don't change the function signature from the AddEntity or RemoveEntity methods.
 * They are being called by the SystemManager class.
 * But you may change the function body if you want special behavior on insertion.
 * I.e., if you don't want to use the mEntities set defined in ecs/System.hh but your own container,
 * you can do so by changing the AddEntity and RemoveEntity method.
 * Be sure that an entity is always removed properly if the corresponding method is called.
 *
 * You can check & retrieve an entity for a certain component by doing any of the following things:
 *    1. Call mECS->TestSignature:
 *
 *       gamedev::SpecialComponent* comp;
 *       if (mECS->TestSignature<gamedev::SpecialComponent>(instance_handle))
 *       {
 *          comp = mECS->GetComponent<gamedev::SpecialComponent>(instance_handle)
 *       }
 *       // Do something with component
 *
 *    2. Call mECS->TryGetComponent:
 *
 *       auto comp = mECS->TryGetComponent<gamedev::SpecialComponent>(instance_handle);
 *       if (comp)
 *       {
 *          // Do something with component
 *       }
 *       returns a raw pointer either holding the component or a nullptr if the entity does not have that component.
 *
 */

namespace gamedev
{
class AISystem : public System
{
public:
    // Collects references to needed (sub)systems available in Game.cc/hh
    void Init(std::shared_ptr<EngineECS>& ecs);

    // Specifies what happens, when a matching component is being added to an entity
    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    // Specifies what happens, when a matching component is being removed from an entity
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    // Specifies what happens, when a matching component is being removed from an entity
    void RemoveEntity(InstanceHandle& handle);
    // Specifies what happens, when the system should delete all its entities
    void RemoveAllEntities();

    // This represents the main update method, other parameters can be passed.
    void Update(float dt);

private:
    std::shared_ptr<EngineECS> mECS;
};
}

