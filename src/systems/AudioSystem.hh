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
 *  - Every update, the emitters within `maxRadius` of the listener that are still playing get one of the limited sources; the others lose theirs.
 *  - The `Play*Sound` functions create emitters on the fly. Emitters at a position are destroyed when they finish.
 *
 * TODO: Global emitters (and their instances) are never removed, so every `PlayGlobalSound` call uses up one of the `MAX_INSTANCES` slots for good.
 */
class AudioSystem : public System
{
public:
    /**
     * Opens the default OpenAL device and context, creates the sources and loads all sounds (`LoadSounds`).
     *
     * @param ecs Shared ECS.
     */
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    /**
     * Advances all emitters, removes finished positional emitters and (re)assigns sources.
     *
     * @param dt Time step in seconds.
     */
    void Update(float dt);

    /**
     * Places the listener (usually the camera); up is +y.
     *
     * @param position Listener position in world space.
     * @param forward View direction.
     * @param velocity Listener velocity (for the Doppler effect).
     */
    void UpdateListener(tg::pos3 position, tg::vec3 forward, tg::vec3 velocity = {0, 0, 0});

    /// Sets the listener gain, clamped to [0, 1].
    void SetMasterVolume(float value);

    /// Sets the volume of a `SoundType` (index), clamped to [0, 1].
    void SetVolume(int type, float value);

    /**
     * Plays a sound at an instance, following it.
     *
     * Adds a `SoundEmitter` to the instance or restarts its existing one.
     * The emitter stays on the instance after the sound ends.
     *
     * @param handle Live instance to play the sound at.
     * @param name File name in `data/audio/`, loaded by `LoadSounds`.
     * @param type Volume group.
     * @param volume Gain in [0, 1].
     * @param looping Restart when finished.
     * @param minRadius Distance up to which the sound is not attenuated.
     * @param maxRadius Distance beyond which the sound is not played.
     * @param priority Unused: only stored in the emitter.
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
     * Plays a sound at a fixed position using a new emitter instance, which is destroyed when the sound ends.
     *
     * @param position World position of the sound.
     * @param name File name in `data/audio/`, loaded by `LoadSounds`.
     * @param type Volume group.
     * @param volume Gain in [0, 1].
     * @param looping Restart when finished; the emitter is then never destroyed.
     * @param minRadius Distance up to which the sound is not attenuated.
     * @param maxRadius Distance beyond which the sound is not played.
     * @param priority Unused: only stored in the emitter.
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
     * Plays a sound without attenuation (it follows the listener) using a new emitter instance.
     *
     * TODO: The emitter instance is never destroyed (see class description).
     *
     * @param name File name in `data/audio/`, loaded by `LoadSounds`.
     * @param type Volume group.
     * @param volume Gain in [0, 1].
     * @param looping Restart when finished.
     * @param priority Unused: only stored in the emitter.
     */
    void PlayGlobalSound(std::string name, SoundType type = effect, float volume = 1.0f, bool looping = false, SoundPriority priority = low);

    /// Loads all WAV files used by the game from `data/audio/`.
    void LoadSounds();

    /// Deletes the sources and closes the OpenAL context and device.
    void destroy();

private:

    /**
     * Advances the emitter's remaining time (looping or stopping it) and pushes its gain, pitch, range and position to its source, if it has one.
     *
     * @param handle Instance with a `SoundEmitter`.
     * @param dt Time step in seconds (scaled by the pitch).
     */
    void UpdateSoundState(InstanceHandle handle, float dt);

    /**
     * Detaches the source of an emitter that is out of range or finished; otherwise queues it in `emitters`.
     *
     * @param handle Instance with a `SoundEmitter`.
     */
    void CullEmitters(InstanceHandle handle);

    /// Destroys finished, non-looping emitters created by `PlayLocalSound(position, ...)`.
    void CleanupLocalEmitters();

    /**
     * Binds a source to an emitter and starts playing its sound.
     *
     * TODO: The start offset is divided by 1000, so a sound that gets a source again restarts from (almost) the beginning
     * instead of where it would be.
     *
     * @param handle Instance with a `SoundEmitter`.
     * @param s Free source; nullptr does nothing.
     */
    void AttachSource(InstanceHandle handle, OALSource* s);

    /**
     * Stops an emitter's source and releases it.
     *
     * @param handle Instance with a `SoundEmitter`; nothing happens if it has no source.
     */
    void DetachSource(InstanceHandle handle);

    /**
     * Calls `DetachSource` for a range of emitters.
     *
     * @param from First emitter.
     * @param to End of the range (exclusive).
     */
    void DetachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);

    /**
     * Calls `AttachSource` with a free source for every emitter of a range that has none.
     *
     * @param from First emitter.
     * @param to End of the range (exclusive).
     */
    void AttachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to);

    /// Returns a source that is not in use, or nullptr.
    OALSource* GetSource();

    /**
     * Compares two emitters by priority. Unused: the sort by priority is disabled.
     *
     * @param handle1 Instance with a `SoundEmitter`.
     * @param handle2 Instance with a `SoundEmitter`.
     * @return True if emitter 1 has a higher priority than emitter 2.
     */
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
