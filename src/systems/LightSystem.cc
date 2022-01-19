#include "systems/LightSystem.hh"
#include "utility/Random.hh"

void gamedev::LightSystem::Init(std::shared_ptr<EngineECS>& ecs)
{
    mECS = ecs;
}

int gamedev::LightSystem::AddFreeLight(PointLight& pl)
{
    mFreeLights.push_back(pl);
    int internID = mFreeLights.size() - 1;

    mExternToInternID.insert({mFreeLightIndex, internID});
    mInternToExternID.insert({internID, mFreeLightIndex});

    mFreeLightIndex++;

    return (mFreeLightIndex - 1);
}

void gamedev::LightSystem::RemoveFreeLight(int light_id)
{
    // Swap'n'Pop
    std::swap(mFreeLights[light_id], mFreeLights.back());
    mFreeLights.pop_back();

    // Update ID mappings
    int extern_id = mInternToExternID[mFreeLights.size() - 1];  // Outside ID of last element
    mExternToInternID[extern_id] = mExternToInternID[light_id]; // Reassign outside ID of last element to removed element (because of swap)
    mInternToExternID.erase(mFreeLights.size() - 1);            // Remove intern ID of last element (because of pop_back)
}

int gamedev::LightSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    BuildLights(dt);

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

const std::vector<gamedev::PointLight>& gamedev::LightSystem::GetLights() { return mLights; }
int gamedev::LightSystem::GetLightCount() { return mLights.size(); }

const gamedev::PointLight& gamedev::LightSystem::GetShadowingPointlight() { return mShadowPL; }

void gamedev::LightSystem::BuildLights(float dt)
{
    mLights.clear();

    // Add all independent lights
    for (const auto& light : mFreeLights)
    {
        mLights.push_back(light);
    }

    // Add all instance-bound lights
    for (const auto& handle : mEntities)
    {
        auto instance = mECS->GetInstance(handle);
        auto light = mECS->GetComponent<gamedev::PointLightEmitter>(handle);

        if (!light->shadowing)
        {
            mLights.push_back(light->pl);

            if (light->flicker)
            {
                while (light->flicker_smoothingQueue.size() >= light->flicker_smoothing)
                {
                    light->flicker_sum -= light->flicker_smoothingQueue.front();
                    light->flicker_smoothingQueue.pop();
                }

                light->flicker_smoothingQueue.push(RandomFloat(light->flicker_min, light->flicker_max));
                light->flicker_sum += light->flicker_smoothingQueue.back();

                mLights.back().intensity *= light->flicker_sum / light->flicker_smoothingQueue.size();
            }


            // Lights bound to an instance move with that instance
            mLights.back().position = tg::mat4(instance.xform.transform_mat()) * mLights.back().position;
        }
        else
        {
            mShadowPL = light->pl;

            if (light->flicker)
            {
                while (light->flicker_smoothingQueue.size() >= light->flicker_smoothing)
                {
                    light->flicker_sum -= light->flicker_smoothingQueue.front();
                    light->flicker_smoothingQueue.pop();
                }

                light->flicker_smoothingQueue.push(RandomFloat(light->flicker_min, light->flicker_max));
                light->flicker_sum += light->flicker_smoothingQueue.back();

                mShadowPL.intensity *= light->flicker_sum / light->flicker_smoothingQueue.size();
            }

            mShadowPL.position = tg::mat4(instance.xform.transform_mat()) * mShadowPL.position;
        }
    }
}


