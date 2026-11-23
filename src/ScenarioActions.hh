#pragma once

namespace gamedev
{
/**
 * Gameplay commands for scripted play, such as benchmark scenarios.
 *
 * `Game` implements it on top of the `GamelogicSystem` and hands it to `GameHooks::OnTickEnd`.
 * The paid actions behave like the HUD buttons: they spend resources, as the player would.
 * Not owned through this interface (protected destructor).
 */
class ScenarioActions
{
public:
    /**
     * Spawns monsters near random enemy spawn points, as a daily wave does.
     *
     * @param monsters Number of monsters.
     */
    virtual void SpawnEnemyWave(int monsters) = 0;

    /**
     * Spawns pioneers that walk to the camp (unless it is full), as the daily reinforcements do.
     *
     * @param pioneers Number of pioneers.
     */
    virtual void SpawnReinforcements(int pioneers) = 0;

    /// Turns the daily enemy waves and reinforcements on or off.
    virtual void SetDailyWaves(bool enabled) = 0;

    /// Adds gold without paying for it.
    virtual void AddGold(int amount) = 0;

    /// Builds a tent on a free inner slot (+4 pioneer capacity).
    virtual void ConstructHome() = 0;

    /// Builds a forge on a free outer slot (raises artisan income).
    virtual void UpgradeProduction() = 0;

    /// Buys 5 firewood.
    virtual void BuyWood() = 0;

    /// Turns a pioneer in or near the camp into an artisan.
    virtual void AssignArtisan() = 0;

    /// Turns a pioneer in or near the camp into a soldier.
    virtual void AssignSoldier() = 0;

    /// Builds the next fortification ring.
    virtual void UpgradeFortifications() = 0;

    /// Restores all destroyed walls.
    virtual void RepairFortifications() = 0;

    /// Builds the next supply route upgrade.
    virtual void UpgradeSupply() = 0;

protected:
    ~ScenarioActions() = default;
};
}
