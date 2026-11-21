#include "Sound.hh"

map<string, gamedev::Sound*> gamedev::Sound::sounds;

gamedev::Sound::Sound()
{
    mBitRate = 0;
    mFreqRate = 0;
    mSoundLength = 0;
    mData = NULL;
    mBuffer = 0;
    mSize = 0;
    mChannels = 0;
}

gamedev::Sound::~Sound(void)
{
    delete mData;
    alDeleteBuffers(1, &mBuffer);
}

double gamedev::Sound::GetLength() { return mSoundLength; }

void gamedev::Sound::LoadFromWAV2(string filename)
{
    const char* cstr = filename.c_str();
    glow::info() << "Loading WAV file '" << filename << "'!";

    /* Open the audio file and check that it's usable. */
    drwav_int16* sampleData = drwav_open_file_and_read_pcm_frames_s16(cstr, &mChannels, &mSampleRate, &mTotalPCMFrameCount, nullptr);
    if (sampleData == NULL)
    {
        throw("failed to load audio file");
        drwav_free(sampleData, nullptr);
    }

    if (getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max()))
    {
        throw("too much data in file for 32bit addressed vector");
        drwav_free(sampleData, nullptr);
    }

    mPcmData.resize(size_t(getTotalSamples()));
    std::memcpy(mPcmData.data(), sampleData, mPcmData.size() * /*twobytes_in_s16*/ 2);
    drwav_free(sampleData, nullptr);

    // length in seconds
    mSoundLength = (float)mPcmData.size() / (mChannels * mSampleRate);
}

void gamedev::Sound::LoadWAVChunkInfo(ifstream& file, string& name, unsigned int& size)
{
    char chunk[4];
    file.read((char*)&chunk, 4);
    file.read((char*)&size, 4);

    name = string(chunk, 4);
}

void gamedev::Sound::AddSound(string name)
{
    Sound* s = GetSound(name);

    if (!s)
    {
        string extension = name.substr(name.length() - 3, 3);
        string path = "../data/audio/";

        if (extension == "wav")
        {
            s = new Sound();
            s->LoadFromWAV2(path + name);
            alGenBuffers(1, &s->mBuffer);

            alBufferData(s->mBuffer, s->mChannels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, s->mPcmData.data(),
                         s->mPcmData.size() * 2 /*two bytes per sample*/, s->mSampleRate);
        }
        else
        {
            s = new Sound();
            glow::error() << "Incompatible file extension '" << extension << "'!";
        }

        sounds.insert(make_pair(name, s));
    }
}

gamedev::Sound* gamedev::Sound::GetSound(string name)
{
    map<string, Sound*>::iterator s = sounds.find(name);
    return (s != sounds.end() ? s->second : NULL);
}

void gamedev::Sound::DeleteSounds()
{
    for (map<string, Sound*>::iterator i = sounds.begin(); i != sounds.end(); ++i)
    {
        delete i->second;
    }
}

ALenum gamedev::Sound::GetOALFormat()
{
    if (GetBitRate() == 16)
    {
        return GetChannels() == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    }
    else if (GetBitRate() == 8)
    {
        return GetChannels() == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
    }
    return AL_FORMAT_MONO8;
}