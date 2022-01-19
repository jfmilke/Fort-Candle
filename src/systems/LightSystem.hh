#pragma once

#include <glow/fwd.hh>
#include <glow/objects/UniformBuffer.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Light.hh"

namespace gamedev
{
class LightSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);
    int AddFreeLight(PointLight& pl);
    void RemoveFreeLight(int light_id);
    int Update(float dt);
    const std::vector<PointLight>& GetLights();
    const PointLight& GetShadowingPointlight();
    int GetLightCount();

private:
    void BuildLights(float dt);

private:
    std::shared_ptr<EngineECS> mECS;
    std::vector<PointLight> mFreeLights;
    std::vector<PointLight> mLights;
    PointLight mShadowPL;

private:
    int mFreeLightIndex = 0;
    std::unordered_map<int, int> mExternToInternID;
    std::unordered_map<int, int> mInternToExternID;
};
}
