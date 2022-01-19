#pragma once
#include <glow/fwd.hh>
#include "typed-geometry/types/pos.hh"
#include <string>

namespace gamedev
{
enum WeaponType
{
    Undefined = -1,
    Bow = 0,
    Claws = 1
};

struct Hitpoints
{
    float current;
    float max;
};

struct Weapon
{
    float damage = 0.5f;
    float range = 0.5f;
    float cooldown = 2.f;
    float missileSpeed = 0.f;
    WeaponType type = WeaponType::Undefined;
};
}
