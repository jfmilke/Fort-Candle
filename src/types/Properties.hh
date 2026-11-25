#pragma once
#include <glow/fwd.hh>
#include <string>
#include "typed-geometry/types/pos.hh"

namespace gamedev
{
/// Weapon kind; decides between ranged (arrows) and melee attacks.
enum WeaponType
{
    Undefined = -1, ///< No weapon.
    Bow = 0,        ///< Ranged: shoots arrows (pioneer soldiers).
    Claws = 1       ///< Melee (monsters).
};

/// Current and maximum health.
struct Hitpoints
{
    float current; ///< Current health; <= 0 means dead or destroyed.
    float max;     ///< Maximum health.
};

/// Attack properties of an `Attacker`.
struct Weapon
{
    float damage = 0.5f;                     ///< Damage per hit.
    float range = 0.5f;                      ///< Attack range in world units.
    float cooldown = 2.f;                    ///< Seconds between attacks.
    float missileSpeed = 0.f;                ///< Highest launch speed of arrows in world units per second (bows only).
    WeaponType type = WeaponType::Undefined; ///< Weapon kind.
};
}
