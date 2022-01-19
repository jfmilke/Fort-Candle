#include "factories/NatureFactory.hh"

// All nature can be rendered
void gamedev::NatureFactory::PostProcessRegistration(InstanceHandle presetHandle)
{
    auto renderable = mECS->CreateComponent<Render>(presetHandle);

    // Render
}
