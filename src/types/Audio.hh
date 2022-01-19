#pragma once
#include "AL/al.h"
#include "AL/alc.h"

namespace gamedev
{
enum SoundPriority
{
    low,
    medium,
    high,
    always
};

enum SoundType
{
    effect,
    ambient,
    music,
    ui
};

struct OALSource
{
    ALuint source;
    bool inUse;

    OALSource(ALuint src)
    {
        source = src;
        inUse = false;
    }
};

}
