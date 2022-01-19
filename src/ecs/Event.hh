#pragma once
#include <unordered_map>
#include <array>
#include <assert.h>

namespace gamedev
{
struct Component;

/***
* https://austinmorlan.com/posts/entity_component_system/
* 
***/
enum class EventType
{
  // Send if two instances collide
  StaticCollision,
  NewPosition,
  ArrowHit,
  MonsterHit,
  PioneerHit,
  BuildingHit,
  MonsterDeath,
  PioneerDeath,
  BuildingDestroyed,
  Supply1,
  Supply2,
  MoreWood
};

struct Event
{
    Event() = delete;
    explicit Event(EventType type, InstanceHandle sender, InstanceHandle subject) : mType(type), mSender(sender), mSubject(subject) {}

    EventType mType;
    InstanceHandle mSender;
    InstanceHandle mSubject;
};


}