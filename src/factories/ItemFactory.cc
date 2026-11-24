#include "factories/ItemFactory.hh"
#include <glow/common/log.hh>

// All items can be rendered
void gamedev::ItemFactory::PostProcessRegistration(InstanceHandle presetHandle)
{
    auto renderable = mECS->CreateComponent<Render>(presetHandle);
}

void gamedev::ItemFactory::InitResources()
{
    Init_Arrow();
    Init_Bow();
}

void gamedev::ItemFactory::Init_Arrow()
{
    auto handle = mIDToPreset[mObjectNameToID["arrow"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto arrow = mECS->CreateComponent<gamedev::Arrow>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);

    instance.xform.translation = {0, 0, 0};

    // Render
    renderable->selectable = false;

    // physics
    physics->gravity = true;
}

void gamedev::ItemFactory::Init_Bow()
{
    auto handle = mIDToPreset[mObjectNameToID["bow"]];
    auto& instance = mECS->GetInstance(handle);

    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);
}
