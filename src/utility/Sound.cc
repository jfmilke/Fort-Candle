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

//void gamedev::Sound::LoadFromWAV(string filename)
//{
//    ifstream file(filename.c_str(), ios::in | ios::binary);
//
//    if (!file)
//    {
//        glow::error() << "Failed to load WAV file '" << filename << "'!";
//        return;
//    }
//
//    glow::info() << "Loading WAV file '" << filename << "'!";
//
//    string chunkName;
//    unsigned int chunkSize;
//
//    while (!file.eof())
//    {
//        LoadWAVChunkInfo(file, chunkName, chunkSize);
//
//        if (chunkName == "RIFF")
//        {
//            file.seekg(4, ios_base::cur);
//            // char waveString[4];
//            // file.read((char*)&waveString,4);
//        }
//        else if (chunkName == "fmt ")
//        {
//            FMTCHUNK fmt;
//
//            file.read((char*)&fmt, sizeof(FMTCHUNK));
//
//            mBitRate = fmt.samp;
//            mFreqRate = (float)fmt.srate;
//            mChannels = fmt.channels;
//        }
//        else if (chunkName == "data")
//        {
//            mSize = chunkSize;
//            mData = new char[mSize];
//            file.read((char*)mData, chunkSize);
//            break;
//            /*
//            In release mode, ifstream and / or something else were combining
//            to make this function see another 'data' chunk, filled with
//            nonsense data, breaking WAV loading. Easiest way to get around it
//            was to simply break after loading the data chunk. This *should*
//            be fine for any WAV file you find / use. Not fun to debug.
//            */
//        }
//        else
//        {
//            file.seekg(chunkSize, ios_base::cur);
//        }
//    }
//
//    mLength = (float)mSize / (mChannels * mFreqRate * (mBitRate / 8.0f)) * 1000.0f;
//
//    file.close();
//}

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

    // length
    mSoundLength = (float)mPcmData.size() / (mChannels * mSampleRate);

    //ALuint soundBuffer;
    //alGenBuffers(1, &soundBuffer);
    //alBufferData(soundBuffer, mChannels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, mPcmData.data(),
    //             mPcmData.size() * 2 /*two bytes per sample*/, mSampleRate);

    ///* Check if an error occured, and clean up if so. */
    //auto err = alGetError();
    //if (err != AL_NO_ERROR)
    //{
    //    fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
    //    if (soundBuffer && alIsBuffer(soundBuffer))
    //        alDeleteBuffers(1, &soundBuffer);
    //    // return 0;
    //}

    // mSoundEffectBuffers.push_back(soundBuffer);

    // return soundBuffer;

    //alGenBuffers(1, &s->mBuffer);
    //alBufferData(s->mBuffer, s->GetOALFormat(), s->GetData(), s->GetSize(), (ALsizei)s->GetFrequency());
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
            //s->LoadFromWAV(path + name);
            s->LoadFromWAV2(path + name);
            alGenBuffers(1, &s->mBuffer);
            //alBufferData(s->mBuffer, s->GetOALFormat(), s->GetData(), s->GetSize(), (ALsizei)s->GetFrequency());

            alBufferData(s->mBuffer, s->mChannels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, s->mPcmData.data(),
                         s->mPcmData.size() * 2 /*two bytes per sample*/, s->mSampleRate);
        }
        /*else if (extension == "ogg")
        {
            OggSound* ogg = new OggSound();
            ogg->LoadFromOgg(name);

            s = ogg;
        }*/
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