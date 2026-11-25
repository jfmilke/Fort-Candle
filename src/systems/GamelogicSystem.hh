#pragma once

#include <glow/fwd.hh>
#include "AudioSystem.hh"
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "factories/ItemFactory.hh"
#include "factories/StructureFactory.hh"
#include "factories/UnitFactory.hh"

#include <array>
#include <optional>

namespace gamedev
{
class CollisionSystem;

/**
 * All gameplay: resources, building, unit orders and AI, combat, enemy waves and reinforcements, and the fire-dependent screen effects.
 *
 * Entities: all instances with a `Living` component (pioneers and monsters), sorted into people, artisans, soldiers and monsters.
 * Positions are world space (x/z ground plane), times in seconds.
 *
 * Each update:
 *  - moves units along their orders,
 *  - lets artisans produce gold,
 *  - lets attackers find, aim at and attack enemies,
 *  - sends monsters to the bonfire, where they steal firewood,
 *  - burns 0.1 firewood per second.
 *    The fire, light and sound of the bonfire, the desaturation and the vignette follow the amount of firewood.
 *
 * The player spends gold through the HUD buttons (`Assign*`, `Upgrade*`, `ConstructHome`, `AddWood`, `Heal`, ...).
 * The game is lost when `GetWood()` drops below zero and won when the supply route is complete (both checked in `Game::gameUI`).
 *
 * Collaborators:
 *  - the unit, structure and item factories create instances,
 *  - the `AudioSystem` plays sounds,
 *  - listens to `StaticCollision` (monsters attack blocking structures) and `ArrowHit` (arrow damage).
 */
class GamelogicSystem : public System
{
public:
    /**
     * Stores the ECS, the audio system and the factories, registers the event listeners and starts the ambient sounds and the music.
     *
     * @param ecs The ECS; must outlive this object.
     * @param as Audio system for all game sounds.
     * @param uf Factory for units.
     * @param sf Factory for structures.
     * @param itf Factory for items (arrows, bows).
     * @param go Asset manager (mesh bounds).
     * @param globalScale Scale of the baked positions (camp center, reinforcement path).
     */
    void Init(EngineECS& ecs,
              std::shared_ptr<AudioSystem>& as,
              std::shared_ptr<UnitFactory>& uf,
              std::shared_ptr<StructureFactory>& sf,
              std::shared_ptr<ItemFactory>& itf,
              std::shared_ptr<GameObjects>& go,
              float globalScale);

    /// Sorts a new `Living` entity into people, artisans (`Producer`), soldiers (friendly `Attacker`) or monsters.
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;
    void RemoveEntity(InstanceHandle handle) override;
    void RemoveAllEntities() override;

    /**
     * Advances the game (see class description).
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

    // ================
    // System Specifics
    // ================

    /// Sets the terrain (for waypoint heights).
    void AttachTerrain(std::shared_ptr<Terrain>& terrain);

    /// Sets the collision system, which archers ask for arrow arcs that clear structures.
    void AttachCollision(std::shared_ptr<CollisionSystem> collision);

    /**
     * Creates the camp: bonfire, two soldiers and a pioneer, the first fortification, two tents, the reinforcement path and the enemy spawn points.
     */
    void SpawnPlayer();

    /**
     * Spawns monsters near random enemy spawn points, heading for the bonfire.
     *
     * @param number Number of monsters.
     */
    void SpawnEnemyWave(int number);

    /**
     * Spawns pioneers on the reinforcement path, walking to the camp, unless the camp is full.
     *
     * @param number Number of pioneers.
     */
    void SpawnReinforcements(int number);

    // Selection/Deselection

    /// Selects the unit that receives right-click orders.
    void Select(InstanceHandle selected);

    /// Clears the selection.
    void Deselect();

    // Initiate Action

    /**
     * Orders the selected unit to move to a position.
     *
     * @param world_position Move target in world space.
     */
    void Rightclick(tg::pos3 world_position);

    /**
     * Orders the selected unit to approach an instance and, if it is a soldier and the instance an enemy, to attack it.
     *
     * Briefly outlines the target in red.
     *
     * @param handle Right-clicked instance.
     */
    void Rightclick(InstanceHandle handle);

    // Getters for UI

    /// Number of pioneers (people, artisans and soldiers).
    int GetPioneerCount();

    /// Number of artisans.
    int GetArtisanCount();

    /// Number of soldiers.
    int GetSoldierCount();

    /// Pioneer capacity: 4 per home.
    int GetMaxPioneers();

    /// Gold, rounded down.
    int GetGold();

    /// Firewood, truncated; negative means the game is lost.
    int GetWood();

    /// Price to heal one injured pioneer.
    int GetHealPrice();

    /// Supply route progress: 0 none, 1 fences, 2 towers, 3 manned towers (victory).
    int GetSupplyLevel();

    /// Price to train an artisan.
    int GetArtisanPrice();

    /// Price to train a soldier.
    int GetSoldierPrice();

    /// Price of 5 firewood.
    int GetWoodPrice();

    /// Price of a new home (tent).
    int GetHomePrice();

    /// Price of a new production building (forge).
    int GetProductionPrice();

    /// Name of the next fortification upgrade (button label).
    std::string GetFortificationUpgrade();

    /// Price of the next fortification upgrade.
    int GetFortificationPrice();

    /// Price to repair all destroyed walls (1 per wall).
    int GetRepairFortificationsPrice();

    /// Price of the next supply route upgrade.
    int GetSupplyPrice();

    /// Number of injured pioneers.
    int GetInjured();

    /// Instances to outline in addition to the selection (new buildings, attack targets).
    const std::vector<InstanceHandle>& GetOutlinedHandles();

    /// Outline color per entry of `GetOutlinedHandles`; alpha fades out over one second (alpha <= -1: permanent).
    const std::vector<tg::vec4>& GetOutlinedColors();

    /// Adds 20 gold (the "Crazy Cheat" button).
    void CRAZY_CHEAT_YOU_WONT_BELIEVE();

    /// Adds `amount` gold without paying for it.
    void AddGold(int amount);

    /// Pays for and builds the next fortification ring: sparse fences, wooden barricades, stone walls, then spikes.
    void UpgradeFortifications();

    /// Pays for and builds a forge on a free outer slot (raises artisan income).
    void UpgradeProduction();

    /// Pays for the next supply route upgrade: fences, then towers, then soldiers manning the towers.
    void UpgradeSupply();

    /// Pays for and builds a tent on a free inner slot (+4 pioneer capacity).
    void ConstructHome();

    /// Pays for and restores all destroyed walls.
    void RepairFortifications();

    /// Pays for turning a pioneer in or near the camp into an artisan (refunds if there is none).
    void AssignArtisan();

    /// Pays for turning a pioneer in or near the camp into a soldier (refunds if there is none).
    void AssignSoldier();

    /// Pays for 5 firewood.
    void AddWood();

    /// Pays for healing all injured pioneers to full health.
    void Heal();

    /// Light of the bonfire. Unused.
    PointLight GetHomePointlight();

    /// Current vignette intensity. Unused.
    float GetVignetteIntensity();

    /// Current vignette softness. Unused.
    float GetVignetteSoftness();

    /// Current vignette radius (smaller when the fire is low).
    float GetVignetteRadius();

    /// Current vignette color. Unused.
    tg::vec3 GetVignetteColor();

    /// Current desaturation (stronger when the fire is low).
    float GetDesaturation();

private:
    /**
     * Pays a price if affordable.
     *
     * @param gold Price.
     * @return True if the gold was subtracted, false if there is not enough.
     */
    bool Pay(int gold);

    /**
     * Burns 0.1 firewood per second (on every update that crosses a full second).
     *
     * @param dt Time step in seconds.
     */
    void ConsumeWood(float dt);

    /**
     * Sets the fire level from the firewood: bonfire particles and light (when the level changes), fire sound volume,
     * and the target desaturation and vignette radius.
     */
    void UpdateHomefire();

    /// Empty.
    void UpdateVignette();

    /// Empty.
    void UpdateDesaturation();

    /**
     * Orders a unit to move; the heading and velocity are re-applied on the next update.
     *
     * @param subject `Living` component of the unit.
     * @param moveTo Move target in world space.
     */
    void SetMoveOrder(Living* subject, tg::vec3 moveTo);

    /// Makes `subject` face `focusHandle`.
    void SetFocus(Living* subject, InstanceHandle focusHandle);

    /**
     * Turns the unit towards its move target and sets its velocity; stops it when it has arrived or has no move order.
     *
     * @param instance Data of the unit.
     * @param subject Its `Living` component.
     * @param dt Time step in seconds.
     */
    void UpdateMoveOrder(const Instance& instance, Living* subject, float dt);

    /**
     * Turns the unit towards its focus instance; a dead focus is cleared.
     *
     * @param instance Data of the unit.
     * @param subject Its `Living` component.
     */
    void UpdateFocus(const Instance& instance, Living* subject);

    /**
     * Monster AI: walk to the bonfire and steal firewood there unless attacking. Does nothing for pioneers.
     *
     * @param instance Data of the unit.
     * @param subject Its `Living` component.
     * @param dt Time step in seconds.
     */
    void UpdateMonster(const Instance& instance, Living* subject, float dt);

    /**
     * Removes `dt / 2` firewood (a monster at the bonfire).
     *
     * @param instance Unused.
     * @param dt Time step in seconds.
     */
    void StealFirewood(const Instance& instance, float dt);

    /**
     * Moves a unit with `ForeignControl` along its waypoints and removes the component at the end.
     *
     * @param instance Data of the unit; nothing happens without a `ForeignControl` component.
     * @param subject Its `Living` component.
     */
    void UpdateForeignControl(const Instance& instance, Living* subject);

    // Attacking

    /**
     * Every 30 updates without a target: soldiers pick a monster in weapon range, monsters pick a pioneer near them
     * (weapon range + 1) and approach it.
     *
     * @param instance Data of the unit.
     * @param lifeform Its `Living` component.
     * @param attacker Its `Attacker` component.
     */
    void LookForEnemy(const Instance& instance, Living* lifeform, Attacker* attacker);

    /**
     * Keeps a soldier stationed in a tower from moving.
     *
     * @param instance Data of the unit; nothing happens without an `InTower` component.
     * @param lifeform Its `Living` component.
     * @param attacker Its `Attacker` component.
     */
    void UpdateTowerSoldier(const Instance& instance, Living* lifeform, Attacker* attacker);

    /**
     * Sets the attack target, which is also the unit's focus.
     *
     * @param subject `Living` component of the unit.
     * @param attacker Its `Attacker` component.
     * @param target Instance to attack.
     */
    void SetAttackOrder(Living* subject, Attacker* attacker, InstanceHandle target);

    /**
     * Drops dead, destroyed (`Prototype`) or out-of-range targets;
     * otherwise draws and aims the weapon and attacks when the cooldown is over and the unit faces the target.
     *
     * @param instance Data of the unit.
     * @param subject Its `Living` component.
     * @param attacker Its `Attacker` component.
     * @param dt Time step in seconds.
     */
    void UpdateAttackOrder(const Instance& instance, Living* subject, Attacker* attacker, float dt);

    /**
     * Creates (bows only) and shows the weapon.
     *
     * @param attacker `Attacker` component of the unit.
     */
    void EquipWeapon(Attacker* attacker);

    /**
     * Hides the weapon. Unused: sheathing is disabled.
     *
     * @param attacker `Attacker` component of the unit.
     */
    void SheathWeapon(Attacker* attacker);

    /**
     * Keeps the weapon at the unit's side.
     *
     * @param instance Data of the unit.
     * @param attacker Its `Attacker` component; nothing happens without a weapon instance.
     */
    void MoveWeapon(const Instance& instance, Attacker* attacker);

    /**
     * Holds the bow in front of the unit, tilted for shooting.
     *
     * @param attacker `Attacker` component of the unit; nothing happens without a weapon instance.
     */
    void AimWeapon(Attacker* attacker);

    /// Launch of an arrow chosen by `AimArrow`.
    struct ArrowShot
    {
        tg::vec3 velocity; ///< World-space launch velocity.
        bool straight;     ///< Straight shot (sticks in structures in its way, `Arrow::hitsBlockers`) or lob (flies through them).
    };

    /**
     * Chooses the launch of an arrow that hits the target if the target keeps its course.
     *
     * The target is assumed to move on at its `Physics::observedVelocity`;
     * the arrow flies a ballistic arc (`PhysicsSystem::kGravity`) to `Arrow::aimHeight` above the ground at the predicted target position.
     * Every flight time gives one such arc: short ones are flat and fast, long ones are high lobs.
     * The flight times from `kMinArrowFlightTime` to `kMaxArrowFlightTime` are tried in steps of `kArrowFlightTimeStep`,
     * skipping arcs that need more than the bow's `missileSpeed`:
     *  1. The flattest arc is a straight shot if no structure is in its way up to the aim point (`IsArrowArcClear`).
     *  2. Otherwise the archer lobs: the lowest arc that passes over the structures except in the last `kArrowFreeApproach`,
     *     or the highest one if none does. Lobs fly through structures.
     *
     * @param attacker `Attacker` component of the soldier, with a bow and an attack target.
     * @param start Launch position of the arrow (world space).
     * @return The launch, or nothing if no arc reaches the target at the bow's speed.
     */
    std::optional<ArrowShot> AimArrow(Attacker* attacker, tg::pos3 start);

    /**
     * Tests whether an arrow arc passes a static collider (`CollisionSystem::IsSegmentBlocked`) before it gets within `freeApproach` of the aim point.
     *
     * @param start Launch position (world space); the collider the archer stands in does not count.
     * @param velocity Launch velocity.
     * @param flightTime Time until the arc reaches the aim point, in seconds.
     * @param distance Horizontal distance from `start` to the aim point.
     * @param freeApproach Horizontal distance before the aim point in which obstacles are ignored.
     * @return True if no static collider is in the way, or if there is no collision system.
     */
    bool IsArrowArcClear(tg::pos3 start, tg::vec3 velocity, float flightTime, float distance, float freeApproach);

    /**
     * Launches an arrow with a smoke trail from the center of the bow, aimed with `AimArrow`.
     *
     * @param attacker `Attacker` component of the soldier, with a bow and an attack target.
     */
    void ShootArrow(Attacker* attacker);

    /**
     * Deals claw damage to the target pioneer or structure; kills or destroys it at zero health.
     *
     * @param attacker `Attacker` component of the monster, with an attack target.
     * @param dt Unused.
     */
    void AttackClaw(Attacker* attacker, float dt);

    /**
     * Sends the death event, plays the death sound and marks the unit for destruction.
     *
     * @param killedHandle Killed unit.
     * @param killedPioneer True for a pioneer (`PioneerDeath`), false for a monster (`MonsterDeath`).
     * @param killerHandle Attacker; the sender of the death event.
     */
    void Kill(InstanceHandle killedHandle, bool killedPioneer, InstanceHandle killerHandle);

    /**
     * Sends the destruction event; walls become `Prototype` (hidden, repairable), other structures are removed.
     *
     * Does nothing if the structure is already destroyed (marked for destruction or a `Prototype`).
     *
     * @param destroyedHandle Destroyed structure.
     * @param destroyerHandle Attacker; its attack target is cleared.
     */
    void Destroy(InstanceHandle destroyedHandle, InstanceHandle destroyerHandle);

    // Production

    /**
     * Picks a random production building for an artisan.
     *
     * Unused: `mProductionBuildings` is never filled, so it would always fail.
     *
     * @param handle Artisan.
     * @return False if there is no production building.
     */
    bool LookForProduction(InstanceHandle handle);

    /**
     * Adds a work particle emitter to an idle artisan. Unused.
     *
     * @param handle Artisan.
     */
    void StartProduction(InstanceHandle handle);

    /**
     * Stops the artisan's production: clears `Producer::producing` and the work particles. Unused.
     *
     * @param handle Artisan.
     */
    void StopProduction(InstanceHandle handle);

    /**
     * Adds the artisan's gold income: `3 * income * (1 + 2 * forges)` per second.
     *
     * @param p `Producer` component of the artisan.
     * @param dt Time step in seconds.
     */
    void Produce(Producer* p, float dt);

    // Buildings

    /// Builds a forge on a random free outer slot, facing the camp center.
    void SpawnForge();

    /**
     * Builds a tent on a random free inner slot.
     *
     * @param highlighted Outline the new tent for a moment.
     */
    void SpawnHouse(bool highlighted = true);

    // Fortifications

    /// Replaces the fortification with a ring of stone walls, pillars and a gate.
    void SpawnStoneWalls();

    /// Replaces the fortification with a closed ring of wooden barricades.
    void SpawnWoodWalls();

    /// Replaces the fortification with sparse wooden fences.
    void SpawnWoodStartingFences();

    /// Adds a ring of spike barriers outside the stone walls.
    void SpawnSpikes();

    /// Destroys all fortification instances immediately.
    void DeleteFortifications();

    /**
     * Moves the vignette and desaturation towards their targets (by half the remaining difference per second).
     *
     * @param dt Time step in seconds.
     */
    void UpdateEffects(float dt);

    // Misc

    /**
     * Tests whether an instance is inside the camp circle. Unused.
     *
     * @param handle Live instance.
     * @return True if its position is inside `mHome`.
     */
    bool WithinFort(InstanceHandle handle);

    /// Fills the free inner (tents) and outer (forges) building slots on rings around the camp center.
    void CalculateFreeSlots();

    /// Sets the path of the reinforcements (baked positions from Blender).
    void SetReinforcementWay();

    /// Sets the fixed enemy spawn points.
    void SetEnemySpawns();

    /// Debug: places a tent on every free slot.
    void TestFreeSlots();

    /// Debug: places a tent on every reinforcement waypoint.
    void TestReinforcementWay();

    /// Builds a tent at a random free spot in the camp. Unused.
    InstanceHandle TryConstructHome();

    /**
     * Creates a structure with random rotation about the y axis and a random uniform scale factor (1.0-1.4).
     *
     * @param position World position.
     * @param object_id Structure preset name.
     * @return The new instance.
     */
    InstanceHandle SpawnObjectRandomly(tg::pos3 position, std::string object_id);

    /**
     * Tries 20 random spots in the camp for a structure, until its bounds overlap no other instance's. Unused.
     *
     * @param object_id Structure preset name.
     * @return The new instance, or an invalid handle if no spot was free.
     */
    InstanceHandle TryConstructObject(std::string object_id);

    /**
     * Like `TryConstructObject` with a random preset. Unused: only the unused `TryConstructHome` calls it.
     *
     * @param object_ids Structure preset names to choose from.
     * @return The new instance, or an invalid handle if no spot was free.
     */
    InstanceHandle TryConstructRandomObject(std::vector<std::string> object_ids);

    /**
     * Fades out temporary outlines and drops outlines of dead instances.
     *
     * @param dt Time step in seconds.
     */
    void UpdateOutlines(float dt);

private:
    /**
     * `StaticCollision` listener: stops units that reached their target; monsters attack the structure they hit.
     *
     * @param e Collision event; the sender is the structure, the subject the unit.
     */
    void CollisionListener(Event& e);

    /**
     * `ArrowHit` listener: damages the target and kills it at zero health (the arrow stays in it, see `AnimationSystem`).
     *
     * @param e Hit event; the sender is the arrow, the subject its target.
     */
    void ArrowListener(Event& e);

public:
    float mGlobalScale = 1.0;                ///< Scale of the baked positions.
    tg::pos3 mPlayerStart = {0.2, 0.0, 7.0}; ///< Baked camp center (before scaling).
    tg::angle32 fort_rotation = 45_deg;      ///< Unused.
    tg::pos3 mReinforcementsStart;           ///< Where reinforcements appear (a waypoint of the path).
    InstanceHandle mSelected;                ///< Unit receiving right-click orders.

    /// Update counter; `Update` refreshes the home fire every 30 updates.
    unsigned int mUpdateControl = 0;

    tg::circle3 mHome;       ///< Camp circle (center = bonfire, radius 10).
    tg::pos3 mHomeEntry;     ///< Unused.
    InstanceHandle mBonfire; ///< Home bonfire.
    InstanceHandle mGate;    ///< Unused.

private:
    std::vector<InstanceHandle> mWalls;               ///< Current fortification instances.
    std::vector<InstanceHandle> mProductionBuildings; ///< Never filled.
    std::vector<InstanceHandle> mDwellings;           ///< Tents built by `TryConstructHome`. Unused.
    std::vector<InstanceHandle> mTowers;              ///< Supply route towers (level 2).

    std::vector<InstanceHandle> mUnits;     ///< Starting units (not updated).
    std::vector<InstanceHandle> mCreatures; ///< All spawned monsters (not cleaned up).

    std::vector<InstanceHandle> mOutlinedHandles; ///< See `GetOutlinedHandles`.
    std::vector<tg::vec4> mOutlinedColors;        ///< See `GetOutlinedColors`.

    std::vector<tg::pos3> mReinforcementWay; ///< Reinforcement path.
    std::vector<tg::pos3> mEnemySpawns;      ///< Enemy spawn points.

    std::set<InstanceHandle> mInjuredPioneers; ///< Pioneers hit by monsters since the last `Heal`.

    float mTime = 0.0; ///< Game time in seconds.

    float mGold = 10;            ///< Gold.
    float mFireWood = 25;        ///< Firewood.
    int mFortificationLevel = 0; ///< Number of fortification upgrades built.
    int mFireLevel = 0;          ///< Fire level 0-6 derived from the firewood.
    int mMaxPioneers = 0;        ///< Unused: `GetMaxPioneers` computes the capacity from `mHomes`.
    int mForges = 0;             ///< Number of forges.
    int mHomes = 0;              ///< Number of tents.
    int mSupplyLevel = 0;        ///< See `GetSupplyLevel`.

    float mTimeUntilReinforcements;   ///< Unused.
    float mTimeBetweenReinforcements; ///< Unused.
    float mTimeBetweenAttacks;        ///< Unused.

    int mDestroyedWalls = 0;               ///< Walls waiting for repair.
    float slotSize = 2.0;                  ///< Distance between building slots.
    std::vector<tg::pos3> mFreeInnerSlots; ///< Free tent slots.
    std::vector<tg::pos3> mFreeOuterSlots; ///< Free forge slots.


private:
    float mVignetteIntensity = 0.5;           ///< Current vignette intensity.
    float mVignetteRadius = 0.75;             ///< Current vignette radius.
    float mVignetteSoftness = 0.4;            ///< Current vignette softness.
    tg::vec3 mVignetteColor = tg::vec3::zero; ///< Current vignette color.

    float mDesaturationIntensity = 0.3; ///< Current desaturation.

    float mVignetteIntensityTarget = 0.5;           ///< Target of `mVignetteIntensity`.
    float mVignetteRadiusTarget = 0.75;             ///< Target of `mVignetteRadius` (set by the fire level).
    float mVignetteSoftnessTarget = 0.4;            ///< Target of `mVignetteSoftness`.
    tg::vec3 mVignetteColorTarget = tg::vec3::zero; ///< Target of `mVignetteColor`.

    float mDesaturationIntensityTarget = 0.3; ///< Target of `mDesaturationIntensity` (set by the fire level).

private:
    static constexpr float kMinArrowFlightTime = 0.1f;  ///< Shortest flight time `AimArrow` tries, in seconds.
    static constexpr float kMaxArrowFlightTime = 3.5f;  ///< Longest flight time `AimArrow` tries (the highest lob), in seconds.
    static constexpr float kArrowFlightTimeStep = 1.1f; ///< Factor between the flight times `AimArrow` tries.
    static constexpr int kArrowArcChords = 12;          ///< Straight pieces an arc is split into for the obstacle test.
    static constexpr float kArrowFreeApproach = 0.5f; ///< Horizontal distance before the aim point in which lobs ignore obstacles (monsters at a wall).

    EngineECS* mECS = nullptr;                   ///< ECS (not owned).
    std::shared_ptr<AudioSystem> mAS;            ///< Audio.
    std::shared_ptr<UnitFactory> mUF;            ///< Unit factory.
    std::shared_ptr<StructureFactory> mSF;       ///< Structure factory.
    std::shared_ptr<ItemFactory> mIF;            ///< Item factory (arrows, bows).
    std::shared_ptr<GameObjects> mGO;            ///< Asset manager (mesh bounds).
    std::shared_ptr<Terrain> mTerrain;           ///< Terrain.
    std::shared_ptr<CollisionSystem> mCollision; ///< Obstacle test for arrow arcs; without it, arrows fly the flattest arc.

    std::set<InstanceHandle> mPeople;   ///< Pioneers without a job.
    std::set<InstanceHandle> mArtisans; ///< Artisans.
    std::set<InstanceHandle> mSoldiers; ///< Soldiers.
    std::set<InstanceHandle> mMonsters; ///< Monsters.
};
}
