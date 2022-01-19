#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"
#include "AudioSystem.hh"
#include "factories/StructureFactory.hh"
#include "factories/UnitFactory.hh"
#include "factories/ItemFactory.hh"

#include <array>

/* Gamelogic System
 * Holds handles of all objects, which qualify for this system.
 * Qualification is defined by Components in Game.cc and whenever a qualifying Component is added or removed
 * from an entity, the handle gets either added or removed from the system.
 * 
 * To define a qualifying component just do something like this (in Game.cc):
 *    Signature signature;
 *    signature.set(mECS->GetComponentType<gamedev::SpecialComponent>());
 *    mECS->SetSystemSignature<gamedev::GamelogicSystem>(signature);
 *
 * Don't change the function signature from the AddEntity or RemoveEntity methods.
 * They are being called by the SystemManager class.
 * But you may change the function body if you want special behavior on insertion.
 * I.e., if you don't want to use the mEntities set defined in ecs/System.hh but your own container,
 * you can do so by changing the AddEntity and RemoveEntity method.
 * Be sure that an entity is always removed properly if the corresponding method is called.
 * 
 * You can check & retrieve an entity for a certain component by doing any of the following things:
 *    1. Call mECS->TestSignature:
 * 
 *       gamedev::SpecialComponent* comp;
 *       if (mECS->TestSignature<gamedev::SpecialComponent>(instance_handle))
 *       {
 *          comp = mECS->GetComponent<gamedev::SpecialComponent>(instance_handle)
 *       }
 *       // Do something with component
 * 
 *    2. Call mECS->TryGetComponent:
 * 
 *       auto comp = mECS->TryGetComponent<gamedev::SpecialComponent>(instance_handle);
 *       if (comp)
 *       {
 *          // Do something with component
 *       }
 *       returns a raw pointer either holding the component or a nullptr if the entity does not have that component.
 *       
 */

namespace gamedev
{
class GamelogicSystem : public System
{
public:
    // Collects references to needed (sub)systems available in Game.cc/hh
    void Init(std::shared_ptr<EngineECS>& ecs,
              std::shared_ptr<AudioSystem>& as,
              std::shared_ptr<UnitFactory>& uf,
              std::shared_ptr<StructureFactory>& sf,
              std::shared_ptr<ItemFactory>& itf,
              std::shared_ptr<GameObjects>& go,
              float globalScale);

    // Specifies what happens, when a matching component is being added to an entity
    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    // Specifies what happens, when a matching component is being removed from an entity
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    // Specifies what happens, when a matching component is being removed from an entity
    void RemoveEntity(InstanceHandle& handle);
    // Specifies what happens, when the system should delete all its entities
    void RemoveAllEntities();

     // Update
    int Update(float dt);

    // ================
    // System Specifics
    // ================

    void AttachTerrain(std::shared_ptr<Terrain>& terrain);
    void SpawnPlayer();

    void SpawnEnemyWave(int number);
    void SpawnReinforcements(int number);

    // Selection/Deselection
    void Select(InstanceHandle selected);
    void Deselect();

    // Initiate Action
    void Rightclick(tg::pos3 world_position);
    void Rightclick(InstanceHandle handle);

    // Getters for UI
    int GetPioneerCount();
    int GetArtisanCount();
    int GetSoldierCount();
    int GetMaxPioneers();
    int GetGold();
    int GetWood();
    int GetHealPrice();
    int GetSupplyLevel();
    int GetArtisanPrice();
    int GetSoldierPrice();
    int GetWoodPrice();
    int GetHomePrice();
    int GetProductionPrice();
    std::string GetFortificationUpgrade();
    int GetFortificationPrice();
    int GetRepairFortificationsPrice();
    int GetSupplyPrice();
    int GetInjured();

    const std::vector<InstanceHandle>& GetOutlinedHandles();
    const std::vector<tg::vec4>& GetOutlinedColors();

    void CRAZY_CHEAT_YOU_WONT_BELIEVE();

    void UpgradeFortifications();
    void UpgradeProduction();
    void UpgradeSupply();
    void ConstructHome();
    void RepairFortifications();
    void AssignArtisan();
    void AssignSoldier();
    void AddWood();
    void Heal();

    PointLight GetHomePointlight();
    float GetVignetteIntensity();
    float GetVignetteSoftness();
    float GetVignetteRadius();
    tg::vec3 GetVignetteColor();
    float GetDesaturation();

private:
    bool Pay(int gold);
    void ConsumeWood(float dt);
    void UpdateHomefire();
    void UpdateVignette();
    void UpdateDesaturation();

    void SetMoveOrder(Living* subject, tg::vec3 moveTo);
    void SetFocus(Living* subject, InstanceHandle focusHandle);
    void UpdateMoveOrder(const Instance& instance, Living* subject, float dt);
    void UpdateFocus(const Instance& instance, Living* subject);
    void UpdateMonster(const Instance& instance, Living* subject, float dt);
    void StealFirewood(const Instance& instance, float dt);
    void UpdateForeignControl(const Instance& instance, Living* subject);

    // Attacking
    void LookForEnemy(const Instance& instance, Living* lifeform, Attacker* attacker);
    void UpdateTowerSoldier(const Instance& instance, Living* lifeform, Attacker* attacker);
    void SetAttackOrder(Living* subject, Attacker* attacker, InstanceHandle target);
    void UpdateAttackOrder(const Instance& instance, Living* subject, Attacker* attacker, float dt);
    void EquipWeapon(Attacker* attacker);
    void SheathWeapon(Attacker* attacker);
    void MoveWeapon(const Instance& instance, Attacker* attacker);
    void AimWeapon(Attacker* attacker);
    arcEq GetArrowArc(Attacker* attacker, float dt);
    void ShootArrow(Attacker* attacker, float dt);
    void AttackClaw(Attacker* attacker, float dt);
    void Kill(InstanceHandle killedHandle, bool killedPioneer, InstanceHandle killerHandle);
    void Destroy(InstanceHandle destroyedHandle, InstanceHandle destroyerHandle);
    // void UpdateAttack(InstanceHandle selected, float dt);

    // Production
    bool LookForProduction(InstanceHandle handle);
    void StartProduction(InstanceHandle handle);
    void StopProduction(InstanceHandle handle);
    void Produce(Producer* p, float dt);

    // Buildings
    void SpawnForge();
    void SpawnHouse(bool highlighted = true);

    // Fortifications
    void SpawnStoneWalls();
    void SpawnWoodWalls();
    void SpawnWoodStartingFences();
    void SpawnSpikes();
    void DeleteFortifications();

    void UpdateEffects(float dt);

    // Misc
    bool WithinFort(InstanceHandle handle);
    void CalculateFreeSlots();
    void SetReinforcementWay();
    void SetEnemySpawns();
    void TestFreeSlots();
    void TestReinforcementWay();
    InstanceHandle TryConstructHome();
    InstanceHandle SpawnObjectRandomly(tg::pos3 position, std::string object_id);
    InstanceHandle TryConstructObject(std::string object_id);
    InstanceHandle TryConstructRandomObject(std::vector<std::string> object_ids);
    void UpdateOutlines(float dt);

private:
    void CollisionListener(Event& e);
    void ArrowListener(Event& e);

public:
    float mGlobalScale = 1.0;                   // Affects baked positions
    tg::pos3 mPlayerStart = {0.2, 0.0, 7.0};    // Baked player starting position
    tg::angle32 fort_rotation = 45_deg;
    tg::pos3 mReinforcementsStart;
    InstanceHandle mSelected;                   // Current selection
    unsigned int mUpdateControl = 0;            // To not update orders every frame

    tg::circle3 mHome;                          // Spans the area of home
    tg::pos3 mHomeEntry;
    InstanceHandle mBonfire;
    InstanceHandle mGate;
    
private:
    std::vector<InstanceHandle> mWalls;
    std::vector<InstanceHandle> mProductionBuildings;
    std::vector<InstanceHandle> mDwellings;
    std::vector<InstanceHandle> mTowers;

    std::vector<InstanceHandle> mUnits;
    std::vector<InstanceHandle> mCreatures;

    std::vector<InstanceHandle> mOutlinedHandles;
    std::vector<tg::vec4> mOutlinedColors;

    std::vector<tg::pos3> mReinforcementWay;
    std::vector<tg::pos3> mEnemySpawns;

    std::set<InstanceHandle> mInjuredPioneers;

    float mTime = 0.0;

    float mGold = 10;
    float mFireWood = 25;
    int mFortificationLevel = 0;
    int mFireLevel = 0;
    int mMaxPioneers = 0;
    int mForges = 0;
    int mHomes = 0;
    int mSupplyLevel = 0;

    float mTimeUntilReinforcements;
    float mTimeBetweenReinforcements;
    float mTimeBetweenAttacks;

    int mDestroyedWalls = 0;
    float slotSize = 2.0;
    std::vector<tg::pos3> mFreeInnerSlots;
    std::vector<tg::pos3> mFreeOuterSlots;


private:
    float mVignetteIntensity = 0.5;
    float mVignetteRadius = 0.75;
    float mVignetteSoftness = 0.4;
    tg::vec3 mVignetteColor = tg::vec3::zero;

    float mDesaturationIntensity = 0.3;

    float mVignetteIntensityTarget = 0.5;
    float mVignetteRadiusTarget = 0.75;
    float mVignetteSoftnessTarget = 0.4;
    tg::vec3 mVignetteColorTarget = tg::vec3::zero;

    float mDesaturationIntensityTarget = 0.3;

private:

    std::shared_ptr<EngineECS> mECS;
    std::shared_ptr<AudioSystem> mAS;
    std::shared_ptr<UnitFactory> mUF;
    std::shared_ptr<StructureFactory> mSF;
    std::shared_ptr<ItemFactory> mIF;
    std::shared_ptr<GameObjects> mGO;
    std::shared_ptr<Terrain> mTerrain;

    std::set<InstanceHandle> mPeople;
    std::set<InstanceHandle> mArtisans;
    std::set<InstanceHandle> mSoldiers;
    std::set<InstanceHandle> mMonsters;
};
}

