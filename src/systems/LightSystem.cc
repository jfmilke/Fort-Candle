#include "systems/LightSystem.hh"
#include "utility/Random.hh"

void gamedev::LightSystem::Init(EngineECS& ecs) { mECS = &ecs; }

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
    auto const it = mExternToInternID.find(light_id);
    if (it == mExternToInternID.end())
        return;

    int const index = it->second;
    int const last = int(mFreeLights.size()) - 1;

    // Swap'n'Pop: the last light moves into the freed index
    if (index != last)
    {
        mFreeLights[index] = mFreeLights[last];

        int const moved_id = mInternToExternID[last];
        mExternToInternID[moved_id] = index;
        mInternToExternID[index] = moved_id;
    }

    mFreeLights.pop_back();
    mExternToInternID.erase(light_id);
    mInternToExternID.erase(last);
}

void gamedev::LightSystem::Update(float dt) { BuildLights(dt); }

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
