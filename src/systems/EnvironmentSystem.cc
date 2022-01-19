#include "systems/EnvironmentSystem.hh"

void gamedev::EnvironmentSystem::Init(std::shared_ptr<EngineECS>& ecs)
{
    mECS = ecs;
    mSunLight.color = tg::vec3(0.985, 1.0, 0.876);

    mSunTheta = 0.f; // 70.0f;
    mSunPhi = 0.f; //-64.0f;
    mSunRadius = 1.f;
    mSunIntensity = 0.5;
    mAmbientLight = tg::vec3(0.75, 1.0, 0.9) * 0.15f;
    SetMinutesPerDay(0.2f);
    SetSunriseHour(6.f);
    SetSunsetHour(21.f);

    mSunrise.position = {60, -90};
    mMidday.position = {0, 0};
    mSunset.position = {60, 90};

    mSunrise.color = {1.f, 0.66f, 0.2f};
    mMidday.color = {1.f, 1.f, 0.8f};
    mSunset.color = {1.f, 0.3f, 0.2f};
    mMidnight.color = {0.f, 0.2f, 1.f};

    mDayCount = 1;
}

int gamedev::EnvironmentSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    mGameTime += dt * mSecondToGameSecond;

    mNewDay = false;

    if (mGameTime > secondsPerDay)
    {
        mGameTime = mGameTime - secondsPerDay;
        mNewDay = true;
        mDayCount += 1;
    }

    TimeOfDay oldTime;
    TimeOfDay newTime;

    if (mGameTime < mSunrise.time)
    {
        oldTime = mMidnight;
        oldTime.time = 0.f;
        oldTime.position = {90, -100};
        newTime = mSunrise;
    }
    else if (mGameTime < mMidday.time)
    {
        oldTime = mSunrise;
        newTime = mMidday;
    }
    else if (mGameTime < mSunset.time)
    {
        oldTime = mMidday;
        newTime = mSunset;
    }
    else if (mGameTime > mSunset.time)
    {
        oldTime = mSunset;
        newTime = mMidnight;
        newTime.time = 86400.f;
        newTime.position = {90, 100};
    }

    float currTime = (mGameTime - oldTime.time) / (newTime.time - oldTime.time);

    mSunLight.color = tg::mix(oldTime.color, newTime.color, currTime);

    mSunTheta = tg::mix(oldTime.position.x, newTime.position.x, currTime);
    mSunPhi = tg::mix(oldTime.position.y, newTime.position.y, currTime);

    if (!mLocked)
      CalculateSun();

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::EnvironmentSystem::SetMinutesPerDay(float minutes)
{
    mSecondToGameSecond = secondsPerDay / (minutes * 60.f);
}

float gamedev::EnvironmentSystem::getGameTime() { return mGameTime; }

int gamedev::EnvironmentSystem::getDayCount() { return mDayCount; }

void gamedev::EnvironmentSystem::SetSunriseHour(float realHour)
{
    mSunrise.time = realHour * 3600.f;
    mMidday.time = (mSunrise.time + mSunset.time) / 2.f;
}
void gamedev::EnvironmentSystem::SetSunsetHour(float realHour)
{
    mSunset.time = realHour * 3600.f;
    mMidday.time = (mSunrise.time + mSunset.time) / 2.f;
}

void gamedev::EnvironmentSystem::LockSun() { mLocked = true; }
void gamedev::EnvironmentSystem::UnlockSun() { mLocked = false; }

void gamedev::EnvironmentSystem::CalculateSun()
{
    // Calculation in spherical coordinate system
    const auto theta = tg::radians(mSunTheta * tg::pi_scalar<float> / 180.f); // forward
    const auto phi = tg::radians(mSunPhi * tg::pi_scalar<float> / 180.f);   // rotation
    // const auto theta = mSunTheta * tg::pi_scalar<float> / 180.f;
    // const auto phi = mSunPhi * tg::pi_scalar<float> / 180.f;

    tg::pos3 sunPosition;

    sunPosition.x = mSunRadius * tg::sin(theta) * tg::cos(phi)+0.01; // avoid crash
    sunPosition.y = mSunRadius * tg::cos(theta);
    sunPosition.z = mSunRadius * tg::sin(theta) * tg::sin(phi);
    
    mSunPos = sunPosition;
    mSunLight.direction = sunPosition - tg::pos3::zero;
}

gamedev::DirectionalLight gamedev::EnvironmentSystem::GenerateSunlight(float theta, float phi, tg::vec3 lightColor, float lightIntensity)
{
    auto rad_theta = tg::radians(theta * tg::pi_scalar<float> / 180.f);
    auto rad_phi = tg::radians(phi * tg::pi_scalar<float> / 180.f);

    tg::pos3 sunPosition;

    sunPosition.x = mSunRadius * tg::sin(rad_theta) * tg::cos(rad_phi);
    sunPosition.y = mSunRadius * tg::cos(rad_theta);
    sunPosition.z = mSunRadius * tg::sin(rad_theta) * tg::sin(rad_phi);

    return {sunPosition - tg::pos3::zero, lightColor * lightIntensity};
}


gamedev::AmbientLight gamedev::EnvironmentSystem::GenerateAmbientlight(tg::vec3 lightColor, float lightIntensity)
{
    return {lightColor * lightIntensity};
}

gamedev::PointLight gamedev::EnvironmentSystem::GeneratePointlight(tg::pos3 lightPosition, tg::vec3 lightColor, float lightIntensity, float lightRadius)
{
    return {lightPosition, lightColor, lightIntensity, lightRadius};
}