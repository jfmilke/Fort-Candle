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
/**
 * 3D audio with OpenAL Soft.
 *
 * Entities: all instances with a `SoundEmitter`. Volumes are in [0, 1], positions in world space.
 *
 *  - `Init` opens the default device, creates up to 32 OpenAL sources ("channels") and loads all sounds.
 *  - Every update, the emitters within `maxRadius` of the listener that are still playing get one of the limited
 *    sources; the others lose theirs.
 *  - The `Play*Sound` functions create emitters on the fly. Emitters at a position are destroyed when they finish.
 *
 * TODO: Global emitters (and their instances) are never removed, so every `PlayGlobalSound` call uses up one of the
 * `MAX_INSTANCES` slots for good.
 */
class AudioSystem : public System
{
public:
    /// Opens the default OpenAL device and context, creates the sources and loads all sounds (`LoadSounds`).
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /// Advances all emitters by `dt` seconds, removes finished positional emitters and (re)assigns sources.
    void Update(float dt);

    /// Places the listener (usually the camera); `forward` is the view direction, up is +y.
    void UpdateListener(tg::pos3 position, tg::vec3 forward, tg::vec3 velocity = {0, 0, 0});

    /// Sets the listener gain, clamped to [0, 1].
    void SetMasterVolume(float value);

    /// Sets the volume of a `SoundType` (index), clamped to [0, 1].
    void SetVolume(int type, float value);

    /**
     * Plays sound `name` (file name in `data/audio/`) at instance `handle`, following it. Adds a `SoundEmitter` to the
     * instance or restarts its existing one. The emitter stays on the instance after the sound ends.
     */
    void PlayLocalSound(InstanceHandle handle,
                        std::string name,
                        SoundType type = effect,
                        float volume = 1.0f,
                        bool looping = false,
                        float minRadius = 1.f,
                        float maxRadius = 50.f,
                        SoundPriority priority = low);

    /**
     * Plays sound `name` at a fixed `position` using a new emitter instance, which is destroyed when the sound ends
     * (unless `looping`).
     */
    void PlayLocalSound(tg::pos3 position,
                        std::string name,
                        SoundType type = effect,
                        float volume = 1.0f,
                        bool looping = false,
                        float minRadius = 1.f,
                        float maxRadius = 50.f,
                        SoundPriority priority = low);

    /**
     * Plays sound `name` without attenuation (it follows the listener) using a new emitter instance.
     *
     * TODO: The emitter instance is never destroyed (see class description).
     */
    void PlayGlobalSound(std::string name, SoundType type = effect, float volume = 1.0f, bool looping = false, SoundPriority priority = low);

    /// Loads all WAV files used by the game from `data/audio/`.
    void LoadSounds();

    /// Deletes the sources and closes the OpenAL context and device.
    void destroy();

private:

    /**
     * Advances the emitter's remaining time (looping or stopping it) and pushes its gain, pitch, range and position
     * to its source, if it has one.
     */
    void UpdateSoundState(InstanceHandle handle, float dt);

    /// Detaches the source of an emitter that is out of range or finished; otherwise queues it in `emitters`.
    void CullEmitters(InstanceHandle handle);

    /// Destroys finished, non-looping emitters created by `PlayLocalSound(position, ...)`.
    void CleanupLocalEmitters();

    /// Binds source `s` (may be nullptr) to the emitter and starts playing its sound.
    void AttachSource(InstanceHandle handle, OALSource* s);

    /// Stops the emitter's source and releases it.
    void DetachSource(InstanceHandle handle);

    /// `DetachSource` for every handle in [from, to).
    void DetachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);

    /// `AttachSource` with a free source for every handle in [from, to) that has none.
    void AttachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);

    /// Returns a source that is not in use, or nullptr.
    OALSource* GetSource();

    /// True if emitter 1 has a higher priority than emitter 2. Unused (the sort by priority is disabled).
    bool CompareNodesByPriority(InstanceHandle handle1, InstanceHandle handle2);

    std::shared_ptr<EngineECS> mECS; ///< Shared ECS.

    unsigned int channels; ///< Number of OpenAL sources to create.

    float masterVolume;         ///< Listener gain.
    std::vector<float> volumes; ///< Volume per `SoundType`.

    std::vector<OALSource*> sources; ///< All OpenAL sources (owned).

    tg::pos3 mListenerPosition; ///< Listener position from the last `UpdateListener`.

    vector<InstanceHandle> emitters;       ///< Emitters that may play this update (rebuilt every update).
    vector<InstanceHandle> mLocalEmitters; ///< Emitters created by `PlayLocalSound(position, ...)`.

    ALCcontext* context; ///< OpenAL context.
    ALCdevice* device;   ///< OpenAL device.
};
}
