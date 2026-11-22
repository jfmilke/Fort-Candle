#pragma once
#include "AL/al.h"
#include "AL/alc.h"

namespace gamedev
{
/// Priority of a sound emitter. Stored but unused (sorting by priority is disabled in `AudioSystem::Update`).
enum SoundPriority
{
    low,
    medium,
    high,
    always
};

/// Volume group of a sound; index into the per-type volumes of the `AudioSystem`.
enum SoundType
{
    effect,  ///< Game effects (combat, building, ...).
    ambient, ///< Ambient loops (water, wind, ...).
    music,   ///< Music.
    ui       ///< Interface clicks.
};

/// An OpenAL source and whether an emitter currently uses it.
struct OALSource
{
    ALuint source; ///< OpenAL source name.
    bool inUse;    ///< Attached to an emitter.

    /// Wraps the existing source `src` (not in use).
    OALSource(ALuint src)
    {
        source = src;
        inUse = false;
    }
};

}
