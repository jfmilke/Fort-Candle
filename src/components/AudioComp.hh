#pragma once
#include "components/Component.hh"
#include "utility/Sound.hh"
#include "Types.hh"

namespace gamedev
{
struct SoundEmitter : public Component
{
    using Component::Component;

    Sound* sound = NULL;
    OALSource* oalSource = NULL;
    SoundPriority priority = low;
    SoundType type = effect;
    float volume = 1.f;
    float minRadius = 1.f;
    float maxRadius = 50.f;
    float pitch = 1.f;
    bool isLooping = true;
    bool isGlobal = false;
    double timeLeft = 0.f;
};
}