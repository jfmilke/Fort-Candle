#pragma once
#include "advanced/World.hh"
#include "Constants.hh"
#include <set>
#include <chrono>

/*** 
 * Short description:
 * 
 * https://austinmorlan.com/posts/entity_component_system/
 *
 * A System is any functionality that iterates upon a list of entities with a certain signature of components.
 * Any custom System must inherint from System.
 * 
 * mEntities contains all InstanceHandles, which qualify for this system by their signatur.
 * 
 * AddEntity/RemoveEntity automatically add & remove InstanceHandles from the system as their signature changes.
 * If preferred, a custom method can be implemented which overwrites this one (i.e., if they are stored in different containers).
 *
 ***/


/*
 * Long description:
 *
 * System Main Class
 * Any system implementation must inherit from this class to be managed by the SystemManager of the Entity Component System.
 * By design, a system holds handles of all objects which qualify for this system in the set mEntities.
 * If an entity qualifies for a system, the AddEntity method is called by the system manager.
 * If an entity does not qualify, the RemoveEntity method is called.
 * Both events are always triggered, independent of whether the entity was registered for the system or not.
 * If the behavior of adding or removing entities should be changed, i.e. to hold them in your own container instead of mEntities,
 * custom AddEntity and RemoveEntity methods can be implemented. Just be sure the keep the function signature the same.
 * 
 * To register a system implementation to the System Manager, do the following in Game.cc/hh:
 * 
 * // Game.hh
 * std::shared_ptr<gamedev::CoolSubSystem> mCoolSubSystem;
 * 
 * // Game.cc
 * mLightSys = mECS->RegisterSystem<gamedev::CoolSubSystem>();
 * 
 * When a system has no component prerequisites registered, any entity will be added to the system.
 * To define a component prerequisite, do the following in Game.cc/hh:
 *
 *    Signature signature;
 *    signature.set(mECS->GetComponentType<gamedev::SpecialComponent>());
 *    mECS->SetSystemSignature<gamedev::AISystem>(signature);
 *    // Attention: Component must be registered to the ECS as well
 */


namespace gamedev
{
class System
{
public:
    // If the system needs to react to changes
    virtual void AddEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.insert(handle); };
    virtual void RemoveEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.erase(handle); };
    virtual void RemoveEntity(InstanceHandle& handle) { mEntities.erase(handle); };
    virtual void RemoveAllEntities() { mEntities.clear(); }

    virtual ~System() {}

    // Reminder: A set is always ordered (here by handle._value) & only holds unique elements.
    std::set<InstanceHandle> mEntities;
};
}
