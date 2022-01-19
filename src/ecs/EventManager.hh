#pragma once
#include "ecs/Event.hh"
#include "advanced/World.hh"
#include <unordered_map>
#include <assert.h>
#include <functional>
#include <list>

#define FUNCTIONAL_LISTENER(EventType, Listener) EventType, std::bind(&Listener, this, std::placeholders::_1)

namespace gamedev
{
/***
* Thanks to
* https://austinmorlan.com/posts/entity_component_system/
* & RTG Assignment 2
* 
* The EventManager is used to generate & optionally also directly resolve simple Events.
* You can do 2 things:
*     1. Send functional events via SendFunctionalEvent(..).
*        These events are resolved the moment, they are issued.
*        But a call to AddFunctionalListener(..) needs  to be done before, which registers a certain method to this event.
*     
*     2. Send plain events via SendEvent(..).
*        These events are NOT handled by the EventManager.
*        You have to manually get mPendingEvents via GetPendingEvents(), loop through all events and resolve them yourself (i.e. in Game.cc).
* 
* How to do functional Events:
*     1. Inside class SomeClass define a function.
*        void SomeClass::InputListener(Event& event) { // Some Code, maybe even altering variables of SomeClass depending on event }
* 
*     2. Add this function as a listener at some point of execution.
*        mEventManager->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::Banana, SomeClass::InputListener));
*        Done.
* 
***/
class EventManager
{
public:
    // If something should directly react to an event, simply do:
    void AddFunctionalListener(EventType type, std::function<void(Event&)> const& listener)
    {
        mFunctionalListeners[type].push_back(listener);
    }

    // Call this method for events, which should be handled directly by some registered method
    void SendFunctionalEvent(Event& event)
    {
        for (auto const& listener : mFunctionalListeners[event.mType])
        {
            listener(event);
        }
    }

    // Call this method for events, which you want handle yourself somewhere
    void SendEvent(Event& event) { mPendingEvents.push_back(event); }

    // Call this method to get all unresolved events and then handle them yourself
    std::vector<Event>& GetPendingEvents() { return mPendingEvents; }

private:
    // Holds all unresolved events that are NOT directly handled by some method
    std::vector<Event> mPendingEvents;
    // Holds all mappings from an EventType to a list of methods which all get executed on a particular EventType
    std::unordered_map<EventType, std::list<std::function<void(Event&)>>> mFunctionalListeners;
};
}