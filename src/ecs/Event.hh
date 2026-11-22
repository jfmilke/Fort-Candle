#pragma once
#include "advanced/World.hh"

namespace gamedev
{
struct Component;

/**
 * Kinds of events sent through the `EventManager`.
 *
 * The meaning of `Event::mSender` and `Event::mSubject` depends on the type and is given per enumerator.
 * Based on https://austinmorlan.com/posts/entity_component_system/.
 */
enum class EventType
{
  StaticCollision,   ///< `CollisionSystem`: a dynamic entity (subject) overlaps a static one (sender).
  NewPosition,       ///< The sender (= subject) moved or was created; `CollisionSystem` updates its hash maps.
  ArrowHit,          ///< `AnimationSystem`: arrow (sender) reached its target (subject).
  MonsterHit,        ///< An arrow (sender) damaged a monster (subject).
  PioneerHit,        ///< A monster (sender) damaged a pioneer (subject).
  BuildingHit,       ///< A monster (sender) damaged a building (subject).
  MonsterDeath,      ///< A monster (subject) was killed by the sender.
  PioneerDeath,      ///< A pioneer (subject) was killed by the sender.
  BuildingDestroyed, ///< A building (subject) was destroyed by the sender.
  Supply1,           ///< First supply route upgrade bought (both handles invalid). No listeners.
  Supply2,           ///< Second supply route upgrade bought (both handles invalid). No listeners.
  MoreWood,          ///< Wood was added to the bonfire (sender = subject = bonfire). No listeners.

  Count ///< Number of event types; not an event.
};

/// A typed message between two instances; see `EventType` for what sender and subject mean.
struct Event
{
    Event() = delete;

    /// Creates an event of `type` sent by `sender` about `subject`.
    explicit Event(EventType type, InstanceHandle sender, InstanceHandle subject) : mType(type), mSender(sender), mSubject(subject) {}

    EventType mType;         ///< Kind of event.
    InstanceHandle mSender;  ///< Instance that caused the event (may be invalid).
    InstanceHandle mSubject; ///< Instance the event is about (may be invalid).
};
}
