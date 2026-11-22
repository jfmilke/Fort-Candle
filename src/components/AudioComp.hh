#pragma once
#include "components/Component.hh"
#include "utility/Sound.hh"
#include "Types.hh"

namespace gamedev
{
/// A sound played at an instance; managed by the `AudioSystem` (usually created through `AudioSystem::Play*Sound`).
struct SoundEmitter : public Component
{
    using Component::Component;

    Sound* sound = NULL;          ///< Sound to play (owned by `Sound`'s registry).
    OALSource* oalSource = NULL;  ///< OpenAL source while the emitter is audible, else nullptr.
    SoundPriority priority = low; ///< Unused: see `SoundPriority`.
    SoundType type = effect;      ///< Volume group.
    float volume = 1.f;           ///< Gain in [0, 1], multiplied with the volume of `type`.
    float minRadius = 1.f;        ///< Distance up to which the sound is not attenuated (OpenAL reference distance).
    float maxRadius = 50.f;       ///< Distance beyond which the emitter gets no source.
    float pitch = 1.f;            ///< Playback speed factor.
    bool isLooping = true;        ///< Restart when finished.
    bool isGlobal = false;        ///< Play at the listener position (no attenuation).
    double timeLeft = 0.f;        ///< Remaining play time in seconds.
};
}
