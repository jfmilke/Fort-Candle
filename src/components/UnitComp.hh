#pragma once
#include "components/Component.hh"
#include "types/Properties.hh"

namespace gamedev
{
struct Living : public Component
{
    using Component::Component;

    int idle = 0;

    // Friendly stuff
    bool friendly;

    // Common stuff
    std::string name;

    bool moveOrder = false;
    tg::pos3 movePosition= tg::pos3::zero;
    float moveSpeed = 1.5;
    bool forceMove = false;

    InstanceHandle focusHandle = {std::uint32_t(-1)};
    bool inFocus = false;
};

struct Dweller : public Component
{
    using Component::Component;

    InstanceHandle home = {std::uint32_t(-1)};
    bool hasHome = false;
};

struct Attacker : public Component
{
    using Component::Component;

    int idle = 0;

    bool forceAttack = false;

    Weapon weapon = {1, 0.5, 1, WeaponType(-1)};
    float currentCooldown = 0;
    InstanceHandle weaponHandle = {std::uint32_t(-1)};

    InstanceHandle attackOrder = {std::uint32_t(-1)};

    int lastLookoutForEnemies = 0;
};

struct Mortal : public Component
{
    using Component::Component;

    Hitpoints health = {1, 1};
    float armor = 0.0;  // damage deflection in percent (range: [0, 1])
};

struct Destructible : public Component
{
    using Component::Component;

    Hitpoints health = {1, 1};
};

struct Producer : public Component
{
    using Component::Component;

    int idle = 0;

    float income = 0.5;
    bool producing = false;

    InstanceHandle produceOrder = {std::uint32_t(-1)};
};

struct Climber : public Component
{
    using Component::Component;

    float speed = 1.0;
    bool elevated = false;

    tg::vec3 climbOrder = tg::vec3::zero;
};

struct Arrow : public Component
{
    using Component::Component;

    float damage = 1.0;
    float lifetime = 4.0;
    float velocity = 0.0;
    tg::angle32 angle = 0_deg;
    InstanceHandle shotBy = {std::uint32_t(-1)};
    InstanceHandle target = {std::uint32_t(-1)};
};

struct InTower : public Component
{
    using Component::Component;
};

struct Tower : public Component
{
    using Component::Component;

    tg::pos3 position;
};

struct ForeignControl : public Component
{
    using Component::Component;

    std::vector<tg::pos3> waypoints;
    tg::pos3 currentTarget = tg::pos3::zero;
    int currentIndex = 0;
};
}