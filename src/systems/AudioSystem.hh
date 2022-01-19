#pragma once

#include <algorithm>
#include <glow/fwd.hh>
#include "glow/common/log.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "AL/al.h"
#include "AL/alc.h"
#include "typed-geometry/tg.hh"
#include "utility/Sound.hh"

namespace gamedev
{
class AudioSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    void Update(float dt);

    void UpdateListener(tg::pos3 position, tg::vec3 forward, tg::vec3 velocity = {0, 0, 0});

    void SetMasterVolume(float value);
    void SetVolume(int type, float value);

    void PlayLocalSound(InstanceHandle handle,
                        std::string name,
                        SoundType type = effect,
                        float volume = 1.0f,
                        bool looping = false,
                        float minRadius = 1.f,
                        float maxRadius = 50.f,
                        SoundPriority priority = low);

    void PlayLocalSound(tg::pos3 position,
                        std::string name,
                        SoundType type = effect,
                        float volume = 1.0f,
                        bool looping = false,
                        float minRadius = 1.f,
                        float maxRadius = 50.f,
                        SoundPriority priority = low);

    void PlayGlobalSound(std::string name, SoundType type = effect, float volume = 1.0f, bool looping = false, SoundPriority priority = low);

    void LoadSounds();

    void destroy();

private:

    void UpdateSoundState(InstanceHandle handle, float dt);

    void CullEmitters(InstanceHandle handle);
    void CleanupLocalEmitters();

    void AttachSource(InstanceHandle handle, OALSource* s);
    void DetachSource(InstanceHandle handle);

    void DetachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);
    void AttachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);

    OALSource* GetSource();

    bool CompareNodesByPriority(InstanceHandle handle1, InstanceHandle handle2);

    std::shared_ptr<EngineECS> mECS;

    unsigned int channels;

    float masterVolume;
    std::vector<float> volumes;

    std::vector<OALSource*> sources;

    tg::pos3 mListenerPosition;

    vector<InstanceHandle> emitters;
    vector<InstanceHandle> mLocalEmitters;

    ALCcontext* context;
    ALCdevice* device;
};
}
