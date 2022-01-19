#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Light.hh"

namespace gamedev
{

struct TimeOfDay
{
    float time;
    tg::vec2 position;
    tg::vec3 color;
};

class EnvironmentSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);
    int Update(float dt);
    void SetMinutesPerDay(float gameMinutes);
    void SetSunriseHour(float realHour);
    void SetSunsetHour(float realHour);
    void LockSun();
    void UnlockSun();

    AmbientLight GenerateAmbientlight(tg::vec3 lightColor = {0.75, 1.0, 0.9}, float lightIntensity = 0.5);
    DirectionalLight GenerateSunlight(float theta = 70, float phi = -64, tg::vec3 lightColor = {0.985, 1.0, 0.876}, float lightIntensity = 1.0);
    PointLight GeneratePointlight(tg::pos3 lightPosition = {0.0, 0.0, 0.0}, tg::vec3 lightColor = {1.0, 1.0, 1.0}, float lightIntensity = 1.0, float lightRadius = 50.0);

    float getGameTime();
    int getDayCount();

public:
    DirectionalLight mSunLight;
    tg::vec3 mAmbientLight;
    float mSunRadius;
    float mSunTheta;
    float mSunPhi;
    float mSunIntensity;
    tg::pos3 mSunPos;

    bool mNewDay = false;

private:
    void CalculateSun();

private:
    std::shared_ptr<EngineECS> mECS;

    const float secondsPerDay = 86400.f;  // 60 * 60 * 24
    float mSecondToGameSecond;
    float mGameTime;                      // in seconds: [0, 86400]
    int mDayCount;

    // in seconds
    // todo: implement day/night cycle
    TimeOfDay mSunrise;
    TimeOfDay mSunset;
    TimeOfDay mMidday;
    TimeOfDay mMidnight;

    bool mLocked = false;
};
}
