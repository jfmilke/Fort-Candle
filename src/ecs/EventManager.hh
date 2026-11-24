#pragma once
#include <array>
#include <functional>
#include <utility>
#include <vector>
#include "advanced/World.hh"
#include "ecs/Event.hh"

/**
 * Expands to the `(type, listener)` arguments of `AddFunctionalListener` for a member function of the calling object.
 *
 * Use inside a member function: `AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::X, MyClass::OnX))`.
 */
#define FUNCTIONAL_LISTENER(EventType, Listener) EventType, std::bind_front(&Listener, this)

namespace gamedev
{
/**
 * Dispatches `Event`s, either synchronously to registered listeners or into a queue.
 *
 * Based on https://austinmorlan.com/posts/entity_component_system/ and RTG Assignment 2.
 * Owned by `EngineECS`, which forwards its event functions here.
 *
 * There are two ways to send events:
 *
 *  1. **Functional events** (`SendFunctionalEvent`) are handled immediately
 *     by every listener registered for their type with `AddFunctionalListener`.
 *     All game events are sent this way:
 *     @code
 *     void SomeClass::OnHit(Event& event)
 *     {
 *         // react, may change members of SomeClass
 *     }
 *
 *     mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::ArrowHit, SomeClass::OnHit));
 *     @endcode
 *
 *  2. **Plain events** (`SendEvent`) are only queued.
 *     The receiver has to fetch them with `GetPendingEvents()` and clear the queue itself.
 *     Unused: nothing sends plain events.
 */
class EventManager
{
public:
    /**
     * Registers a listener for every functional event of one type. Listeners cannot be removed.
     *
     * Register listeners during initialization: adding one from inside a listener of the same type invalidates the running dispatch.
     *
     * @param type Event type to listen to.
     * @param listener Function to call with the event.
     */
    void AddFunctionalListener(EventType type, std::function<void(Event&)> listener)
    {
        mFunctionalListeners[std::to_underlying(type)].push_back(std::move(listener));
    }

    /**
     * Calls all listeners registered for the event's type synchronously, in registration order.
     *
     * @param event Event to send; listeners may modify it.
     */
    void SendFunctionalEvent(Event& event)
    {
        for (auto const& listener : mFunctionalListeners[std::to_underlying(event.mType)])
            listener(event);
    }

    /**
     * Appends an event to the pending queue without handling it.
     *
     * @param event Event to copy into the queue.
     */
    void SendEvent(Event& event) { mPendingEvents.push_back(event); }

    /// Queue of events sent with `SendEvent`; the caller handles and clears them.
    std::vector<Event>& GetPendingEvents() { return mPendingEvents; }

private:
    std::vector<Event> mPendingEvents; ///< Events sent with `SendEvent`, not handled by the manager.

    /// Listeners per event type, indexed by `std::to_underlying(EventType)`.
    std::array<std::vector<std::function<void(Event&)>>, std::to_underlying(EventType::Count)> mFunctionalListeners;
};
}
