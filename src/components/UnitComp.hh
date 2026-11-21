#pragma once
#include "components/Component.hh"
#include "types/Properties.hh"

namespace gamedev
{
/// A unit (pioneer or monster) driven by the `GamelogicSystem`, whose signature is this component.
struct Living : public Component
{
    using Component::Component;

    int idle = 0; ///< Reset when moving; otherwise unused.

    // Friendly stuff
    bool friendly; ///< Pioneer (true) or monster (false).

    // Common stuff
    std::string name; ///< Display name (set by the factory, not shown).

    bool moveOrder = false;                ///< Moving towards `movePosition`.
    tg::pos3 movePosition= tg::pos3::zero; ///< Move target in world space.
    float moveSpeed = 1.5;                 ///< Speed in world units per second.
    bool forceMove = false;                ///< Re-apply heading and velocity on the next update (set with each new order).

    InstanceHandle focusHandle = {std::uint32_t(-1)}; ///< Instance to face (e.g. the attack target).
    bool inFocus = false;                             ///< Unused.
};

/// A pioneer that can live in a home. Unused apart from the presets.
struct Dweller : public Component
{
    using Component::Component;

    InstanceHandle home = {std::uint32_t(-1)}; ///< Home building (unused).
    bool hasHome = false;                      ///< Unused.
};

/// A unit that attacks enemies in range with its weapon (soldiers shoot arrows, monsters use claws).
struct Attacker : public Component
{
    using Component::Component;

    int idle = 0; ///< Updates without a target.

    bool forceAttack = false; ///< Set when the player orders an attack (not read).

    Weapon weapon = {1, 0.5, 1, WeaponType(-1)};       ///< Weapon properties.
    float currentCooldown = 0;                         ///< Seconds until the next attack.
    InstanceHandle weaponHandle = {std::uint32_t(-1)}; ///< Visible weapon instance (bow), created when first needed.

    InstanceHandle attackOrder = {std::uint32_t(-1)}; ///< Current target.

    int lastLookoutForEnemies = 0; ///< Updates since the last search for enemies (searches every 30 updates).
};

/// Health of a unit; at zero health the unit is killed.
struct Mortal : public Component
{
    using Component::Component;

    Hitpoints health = {1, 1}; ///< Current and maximum health.
    float armor = 0.0;         ///< Damage deflection in [0, 1]; not applied.
};

/// Health of a structure. Destroyed walls are kept (as `Prototype`) so they can be repaired; other structures are removed.
struct Destructible : public Component
{
    using Component::Component;

    Hitpoints health = {1, 1}; ///< Current and maximum health.
};

/// A pioneer that produces gold (artisan).
struct Producer : public Component
{
    using Component::Component;

    int idle = 0; ///< Unused.

    float income = 0.5;     ///< Base gold income; per second it yields `3 * income * (1 + 2 * forges)`.
    bool producing = false; ///< Unused (production is continuous).

    InstanceHandle produceOrder = {std::uint32_t(-1)}; ///< Production building (only set by the unused `LookForProduction`).
};

/// A unit that could climb walls. Unused apart from the soldier preset.
struct Climber : public Component
{
    using Component::Component;

    float speed = 1.0;     ///< Climbing speed (unused).
    bool elevated = false; ///< On a wall (unused).

    tg::vec3 climbOrder = tg::vec3::zero; ///< Climb target (unused).
};

/// A flying arrow; see `AnimationSystem` (flight, hit detection) and `GamelogicSystem::ArrowListener` (damage).
struct Arrow : public Component
{
    using Component::Component;

    float damage = 1.0;                          ///< Damage on hit.
    float lifetime = 4.0;                        ///< Seconds until the arrow is removed.
    float velocity = 0.0;                        ///< Unused (see `Physics`).
    tg::angle32 angle = 0_deg;                   ///< Unused.
    InstanceHandle shotBy = {std::uint32_t(-1)}; ///< Shooter (credited for kills).
    InstanceHandle target = {std::uint32_t(-1)}; ///< Target; the arrow hits when it gets close to it.
};

/// Marks a soldier stationed in a watch tower: it does not move and is ignored by physics and collision.
struct InTower : public Component
{
    using Component::Component;
};

/// A watch tower that can hold a soldier.
struct Tower : public Component
{
    using Component::Component;

    tg::pos3 position; ///< Soldier position relative to the tower (scaled with the tower).
};

/// Moves a unit along waypoints (reinforcements walking to the camp); removed at the last waypoint.
struct ForeignControl : public Component
{
    using Component::Component;

    std::vector<tg::pos3> waypoints;         ///< Path in world space.
    tg::pos3 currentTarget = tg::pos3::zero; ///< Current waypoint (zero before the first update).
    int currentIndex = 0;                    ///< Index of the current waypoint; starts after this one.
};
}
