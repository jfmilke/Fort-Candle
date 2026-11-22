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

/**
 * Layout of a WAV "fmt " chunk.
 *
 * Unused: only meant for the legacy loader, which is not implemented.
 */
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
/**
 * A sound loaded into an OpenAL buffer, plus a static registry of all loaded sounds by file name.
 *
 *  - Sounds are loaded once by `AudioSystem::LoadSounds` via `AddSound`.
 *  - `SoundEmitter`s look them up with `GetSound`.
 *  - Only WAV files are supported (decoded to 16-bit samples with dr_wav).
 */
class Sound
{
public:
    /// Raw data of the legacy loader; always nullptr.
    char* GetData() { return mData; }

    /// Bits per sample of the legacy loader; always 0.
    int GetBitRate() { return mBitRate; }

    /// Sample rate of the legacy loader; always 0.
    float GetFrequency() { return mFreqRate; }

    /// Number of channels (1 or 2).
    int GetChannels() { return mChannels; }

    /// Data size of the legacy loader; always 0.
    int GetSize() { return mSize; }

    /// OpenAL buffer with the samples.
    ALuint GetBuffer() { return mBuffer; }

    /// OpenAL format derived from the legacy bit rate. Unused.
    ALenum GetOALFormat();

    /// Duration in seconds.
    double GetLength();

    /**
     * Loads a sound into a new OpenAL buffer and registers it under its file name, unless already registered.
     *
     * Files without `.wav` extension are registered as empty sounds and log an error.
     * A WAV file that cannot be read throws (see `LoadFromWAV2`).
     *
     * @param n File name in `data/audio/`.
     */
    static void AddSound(string n);

    /// Returns the sound registered under `name`, or nullptr.
    static Sound* GetSound(string name);

    /**
     * Deletes all registered sounds and their buffers. Unused.
     *
     * TODO: The registry keeps the dangling pointers.
     */
    static void DeleteSounds();

protected:
    Sound();
    virtual ~Sound(void);

    /**
     * Legacy WAV loader; declared but not implemented.
     *
     * @param filename Path of the WAV file.
     */
    void LoadFromWAV(string filename);

    /**
     * Decodes a WAV file to 16-bit samples with dr_wav into `mPcmData` and sets channels, sample rate and length.
     *
     * Throws a `const char*` if the file cannot be read.
     *
     * @param filename Path of the WAV file, relative to the working directory.
     */
    void LoadFromWAV2(string filename);

    /**
     * Reads a RIFF chunk header from a file. Unused: only meant for the legacy loader, which is not implemented.
     *
     * @param file Open WAV file, positioned at a chunk header.
     * @param[out] name 4-character chunk name.
     * @param[out] size Chunk size in bytes.
     */
    void LoadWAVChunkInfo(ifstream& file, string& name, unsigned int& size);

    char* mData;    ///< Legacy loader data, always nullptr. Unused.
    ALuint mBuffer; ///< OpenAL buffer.

    float mFreqRate;        ///< Legacy loader sample rate. Unused.
    double mSoundLength;    ///< Duration in seconds.
    unsigned int mBitRate;  ///< Legacy loader bits per sample. Unused.
    unsigned int mSize;     ///< Legacy loader data size. Unused.
    unsigned int mChannels; ///< Number of channels.

    unsigned int mSampleRate = 0;         ///< Samples per second and channel.
    drwav_uint64 mTotalPCMFrameCount = 0; ///< Number of frames (one sample per channel).
    std::vector<uint16_t> mPcmData;       ///< Interleaved 16-bit samples.

    /// Number of samples over all channels.
    drwav_uint64 getTotalSamples() { return mTotalPCMFrameCount * mChannels; }

    static map<string, Sound*> sounds; ///< Registry: file name -> sound (owned).
};
}
