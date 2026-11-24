#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Light.hh"

namespace gamedev
{

/// Key frame of the sun's day cycle.
struct TimeOfDay
{
    float time;        ///< Game time of day in seconds [0, 86400].
    tg::vec2 position; ///< Sun angles in degrees: x = polar angle from the zenith (theta), y = azimuth (phi).
    tg::vec3 color;    ///< Sun color.
};

/**
 * Game clock (time of day and day counter) and the sun position derived from it.
 *
 * Not an ECS system with entities: `Game` creates it directly and calls `Update` every tick.
 * The game time runs `secondsPerDay / (minutes per day * 60)` times faster than real time; `Game::init()` sets a day length of 0.5 real minutes.
 *
 * The sun computed here (`mSunLight`) is not used for rendering; `Game` passes its own fixed sun parameters to `GenerateSunlight`
 * (see the day/night TODO below).
 */
class EnvironmentSystem : public System
{
public:
    /**
     * Sets the default sun, ambient light, a day length of 0.2 real minutes, sunrise at 6:00 and sunset at 21:00.
     *
     * @param ecs The ECS; must outlive this object. Unused: only stored.
     */
    void Init(EngineECS& ecs);

    /**
     * Advances the game time, counts days (`mNewDay`) and interpolates the sun.
     *
     * @param dt Time step in real seconds.
     */
    void Update(float dt);

    /// Sets how many real minutes a game day lasts.
    void SetMinutesPerDay(float gameMinutes);

    /// Sets the hour of game time at which the sun rises (midday is halfway between sunrise and sunset).
    void SetSunriseHour(float realHour);

    /// Sets the hour of game time at which the sun sets (midday is halfway between sunrise and sunset).
    void SetSunsetHour(float realHour);

    /// Freezes `mSunLight.direction` and `mSunPos` (the game clock keeps running).
    void LockSun();

    /// Undoes `LockSun`.
    void UnlockSun();

    /**
     * Creates an ambient light.
     *
     * @param lightColor Color.
     * @param lightIntensity Factor for `lightColor`.
     * @return Ambient light of `lightColor * lightIntensity`.
     */
    AmbientLight GenerateAmbientlight(tg::vec3 lightColor = {0.75, 1.0, 0.9}, float lightIntensity = 0.5);

    /**
     * Creates a sun light from spherical angles.
     *
     * @param theta Polar angle from the zenith in degrees.
     * @param phi Azimuth in degrees.
     * @param lightColor Color.
     * @param lightIntensity Factor for `lightColor`.
     * @return Directional light whose direction points from the scene to the sun (length `mSunRadius`).
     */
    DirectionalLight GenerateSunlight(float theta = 70, float phi = -64, tg::vec3 lightColor = {0.985, 1.0, 0.876}, float lightIntensity = 1.0);

    /**
     * Creates a point light.
     *
     * @param lightPosition Position.
     * @param lightColor Color.
     * @param lightIntensity Intensity.
     * @param lightRadius Radius of influence.
     * @return The point light.
     */
    PointLight GeneratePointlight(tg::pos3 lightPosition = {0.0, 0.0, 0.0}, tg::vec3 lightColor = {1.0, 1.0, 1.0}, float lightIntensity = 1.0, float lightRadius = 50.0);

    /// Current time of day in game seconds [0, 86400].
    float getGameTime();

    /// Current day, starting at 1.
    int getDayCount();

public:
    DirectionalLight mSunLight; ///< Sun interpolated over the day; `direction` points to the sun. Unused: `Game` passes its own sun.
    tg::vec3 mAmbientLight;     ///< Ambient color set by `Init`. Unused.
    float mSunRadius;           ///< Distance of the sun position from the origin.
    float mSunTheta;            ///< Current sun polar angle in degrees.
    float mSunPhi;              ///< Current sun azimuth in degrees.
    float mSunIntensity;        ///< Sun intensity set by `Init`. Unused.
    tg::pos3 mSunPos;           ///< Current sun position.

    bool mNewDay = false; ///< True during the update in which a new day started.

private:
    /// Computes `mSunPos` and `mSunLight.direction` from `mSunTheta`/`mSunPhi`.
    void CalculateSun();

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned). Unused.

    const float secondsPerDay = 86400.f; ///< Game seconds per day (60 * 60 * 24).
    float mSecondToGameSecond;           ///< Game seconds per real second.
    /**
     * Time of day in game seconds: [0, 86400].
     *
     * TODO: Not set by `Init`; it only starts at 0 because `Game` value-initializes the system with `make_shared`.
     */
    float mGameTime;
    int mDayCount; ///< Current day, starting at 1.

    // in seconds
    // todo: implement day/night cycle
    TimeOfDay mSunrise;  ///< Sun at sunrise.
    TimeOfDay mSunset;   ///< Sun at sunset.
    TimeOfDay mMidday;   ///< Sun at midday.
    TimeOfDay mMidnight; ///< Sun color at midnight (time and position are set in `Update`).

    bool mLocked = false; ///< See `LockSun`.
};
}
