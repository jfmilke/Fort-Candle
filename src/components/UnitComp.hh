#pragma once
#include <string>
#include <typed-geometry/tg-lean.hh>
#include <vector>
#include "advanced/World.hh"
#include "components/Component.hh"
#include "types/Properties.hh"

namespace gamedev
{
/// A unit (pioneer or monster) driven by the `GamelogicSystem`, whose signature is this component.
struct Living : public Component
{
    using Component::Component;

    int idle = 0; ///< Unused: only reset when moving.

    // Friendly stuff
    bool friendly; ///< Pioneer (true) or monster (false).

    // Common stuff
    std::string name; ///< Display name (set by the factory, not shown).

    bool moveOrder = false;                 ///< Moving towards `movePosition`.
    tg::pos3 movePosition = tg::pos3::zero; ///< Move target in world space.
    float moveSpeed = 1.5;                  ///< Speed in world units per second.
    bool forceMove = false;                 ///< Re-apply heading and velocity on the next update (set with each new order).

    InstanceHandle focusHandle = {std::uint32_t(-1)}; ///< Instance to face (e.g. the attack target).
    bool inFocus = false;                             ///< Unused.
};

/**
 * A pioneer that can live in a home.
 *
 * Unused: the unit presets attach it, but nothing reads it.
 */
struct Dweller : public Component
{
    using Component::Component;

    InstanceHandle home = {std::uint32_t(-1)}; ///< Home building. Unused.
    bool hasHome = false;                      ///< Unused.
};

/// A unit that attacks enemies in range with its weapon (soldiers shoot arrows, monsters use claws).
struct Attacker : public Component
{
    using Component::Component;

    int idle = 0; ///< Updates without a target.

    bool forceAttack = false; ///< Set when the player orders an attack (not read).

    Weapon weapon = {1, 0.5, 1, 0, WeaponType::Undefined}; ///< Weapon properties; the unit presets overwrite them.

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
    bool producing = false; ///< Unused: production is continuous.

    InstanceHandle produceOrder = {std::uint32_t(-1)}; ///< Production building. Unused: only the unused `LookForProduction` sets it.
};

/**
 * A unit that could climb walls.
 *
 * Unused: the soldier preset attaches it, but nothing reads it.
 */
struct Climber : public Component
{
    using Component::Component;

    float speed = 1.0;     ///< Climbing speed. Unused.
    bool elevated = false; ///< On a wall. Unused.

    tg::vec3 climbOrder = tg::vec3::zero; ///< Climb target. Unused.
};

/// Flight state of an `Arrow`.
enum class ArrowState
{
    Flying,   ///< Moved ballistically by the `PhysicsSystem`.
    InGround, ///< Stuck in the terrain for good (`AnimationSystem` keeps the newest `AnimationSystem::kMaxGroundArrows`).
    InTarget  ///< Stuck in the unit it hit; follows it and is removed with it.
};

/**
 * An arrow: flies along a ballistic arc, then sticks in its target or in the ground.
 *
 * `GamelogicSystem::ShootArrow` launches it, the `PhysicsSystem` moves it,
 * the `AnimationSystem` aligns it to its flight direction, detects hits and landings and keeps stuck arrows in place,
 * and `GamelogicSystem::ArrowListener` deals the damage.
 */
struct Arrow : public Component
{
    using Component::Component;

    float damage = 1.0;                          ///< Damage on hit.
    float lifetime = 4.0;                        ///< Seconds of flight until the arrow is removed (if it never lands).
    float velocity = 0.0;                        ///< Unused: `Physics::velocity` moves the arrow.
    tg::angle32 angle = 0_deg;                   ///< Unused.
    InstanceHandle shotBy = {std::uint32_t(-1)}; ///< Shooter (credited for kills).
    InstanceHandle target = {std::uint32_t(-1)}; ///< Target; the arrow hits when its tip passes within `hitRadius` of its aim point.

    ArrowState state = ArrowState::Flying;       ///< Flying or stuck.
    tg::vec3 stuckOffset = tg::vec3::zero;       ///< `InTarget`: position in the target's local frame (before the target's scale).
    tg::quat stuckRotation = tg::quat::identity; ///< `InTarget`: rotation relative to the target.

    static constexpr float aimHeight = 0.25f;   ///< Aim point above the target's position: about the body center of the 0.25-scaled units.
    static constexpr float hitRadius = 0.32f;   ///< Hit when the path of the tip in one update passes this close to the aim point.
    static constexpr float tipOffset = 0.33f;   ///< Distance from the arrow's origin to its tip, along its local +z axis (at the item scale 0.8).
    static constexpr float embedDepth = 0.1f;   ///< How deep the tip sinks into the ground.
    static constexpr float stuckRadius = 0.08f; ///< The tip of an arrow stuck in a unit is at most this far from the aim point.
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
    int currentIndex = 0;                    ///< Index of `currentTarget`; the first target is the waypoint after the initial value.
};
}
