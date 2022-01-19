#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "glow/common/log.hh"
#include <vector>

#include "AL/al.h"
#include "AL/alc.h"
#include "dr_wav.hh"

#include <limits>
#include <cstring>

using namespace std;

typedef unsigned long DWORD;

struct FMTCHUNK
{
    short format;
    short channels;
    DWORD srate;
    DWORD bps;
    short balign;
    short samp;
};

namespace gamedev
{
class Sound
{
public:
    char* GetData() { return mData; }
    int GetBitRate() { return mBitRate; }
    float GetFrequency() { return mFreqRate; }
    int GetChannels() { return mChannels; }
    int GetSize() { return mSize; }
    ALuint GetBuffer() { return mBuffer; }
    // bool IsStreaming() { return streaming; }
    // virtual double StreamData(ALuint buffer, double timeLeft) { return 0.0f; }

    ALenum GetOALFormat();
    double GetLength();

    static void AddSound(string n);
    static Sound* GetSound(string name);

    static void DeleteSounds();

protected:
    Sound();
    virtual ~Sound(void);

    void LoadFromWAV(string filename);
    void LoadFromWAV2(string filename);
    void LoadWAVChunkInfo(ifstream& file, string& name, unsigned int& size);

    char* mData;
    ALuint mBuffer;

    float mFreqRate;
    double mSoundLength;
    unsigned int mBitRate;
    unsigned int mSize;
    unsigned int mChannels;

    unsigned int mSampleRate = 0;
    drwav_uint64 mTotalPCMFrameCount = 0;
    std::vector<uint16_t> mPcmData;
    drwav_uint64 getTotalSamples() { return mTotalPCMFrameCount * mChannels; }

    static map<string, Sound*> sounds;
};
}