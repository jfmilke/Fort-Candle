#include "advanced/transform.hh"
#include "glow/common/log.hh"
#include "systems/AudioSystem.hh"

void gamedev::AudioSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{ mEntities.insert(handle); }

void gamedev::AudioSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) { mEntities.erase(handle); }

void gamedev::AudioSystem::RemoveEntity(InstanceHandle& handle) { mEntities.erase(handle); }

void gamedev::AudioSystem::RemoveAllEntities() { mEntities.clear(); }

void gamedev::AudioSystem::Init(std::shared_ptr<EngineECS>& ecs) 
{ 
    mECS = ecs; 

    channels = 32;
    masterVolume = 1.0f;
    volumes = {1.0f, 1.0f, 1.0f, 1.0f};

    glow::info() << "Found the following devices: " << alcGetString(NULL, ALC_DEVICE_SPECIFIER);

    device = alcOpenDevice(NULL); // Open the 'best' device

    if (!device)
    {
        glow::info() << "Failed to create SoundSystem! (No valid device!)";
    }

    glow::info() << "SoundSystem created with device: " << alcGetString(device, ALC_DEVICE_SPECIFIER);

    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED); // exponential sound decay

    for (unsigned int i = 0; i < channels; ++i)
    {
        ALuint source;

        alGenSources(1, &source);
        ALenum error = alGetError();

        if (error == AL_NO_ERROR)
        {
            sources.push_back(new OALSource(source));
        }
        else
        {
            break;
        }
    }

    glow::info() << "SoundSystem has " << sources.size() << " channels available!";

    LoadSounds();
}

void gamedev::AudioSystem::Update(float dt)
{
    CleanupLocalEmitters();

    for (const auto& handle : mEntities)
    {
        auto emitter = mECS->GetComponent<SoundEmitter>(handle);

        if (emitter)
        {
            // Update values for every emitter, whether in range or not
            UpdateSoundState(handle, dt);

            CullEmitters(handle); // Remove emitters that are too far away
        }
    }

    if (emitters.size() > sources.size())
    {
        // std::sort(emitters.begin(), emitters.end(), gamedev::AudioSystem::CompareNodesByPriority); // Then sort by priority

        DetachSources(emitters.begin() + (sources.size() + 1), emitters.end()); // Detach sources from emitters that won't be covered this frame
        
        AttachSources(emitters.begin(), emitters.begin() + (sources.size()));   // And attach sources to emitters that WILL be covered this frame
    }
    else
    {
        AttachSources(emitters.begin(), emitters.end());
    }

    emitters.clear(); // Empty the emitters list
}

void gamedev::AudioSystem::UpdateListener(tg::pos3 position, tg::vec3 forward, tg::vec3 velocity)
{
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALfloat forwardAndUp[] = {forward.x, forward.y, forward.z, 0.f, 1.f, 0.f};
    alListenerfv(AL_ORIENTATION, forwardAndUp);
    
    mListenerPosition = position;
}

void gamedev::AudioSystem::UpdateSoundState(InstanceHandle handle, float dt)
{
    auto emitter = mECS->GetComponent<SoundEmitter>(handle);

    emitter->timeLeft -= (dt * emitter->pitch);

    if (emitter->isLooping)
    {
        while (emitter->timeLeft < 0)
        {
            emitter->timeLeft += emitter->sound->GetLength();
            // streamPos += sound->GetLength();
        }
    }
    else if (emitter->timeLeft < 0)
    {
        DetachSource(handle);
        // mECS->RemoveComponent<SoundEmitter>(handle);
    }

    if (emitter->oalSource)
    {
        auto source = emitter->oalSource->source;
        alSourcef(source, AL_GAIN, emitter->volume * volumes[emitter->type]);
        alSourcef(source, AL_PITCH, emitter->pitch);
        alSourcef(source, AL_MAX_DISTANCE, emitter->maxRadius);
        alSourcef(source, AL_REFERENCE_DISTANCE, emitter->minRadius);

        tg::vec3 position;

        if (emitter->isGlobal)
        {
            position = tg::vec3(mListenerPosition);
        }
        else
        {
            auto& instance = mECS->GetInstance(handle);
            position = instance.xform.translation;
        }

        alSourcefv(emitter->oalSource->source, AL_POSITION, (float*)&position);

        //if (sound->IsStreaming())
        //{
        //    int numProcessed;
        //    alGetSourcei(oalSource->source, AL_BUFFERS_PROCESSED, &numProcessed);
        //    alSourcei(oalSource->source, AL_LOOPING, 0);

        //    while (numProcessed-- /* && streamPos > 0*/)
        //    { // The && prevents clipping at the end of sounds!
        //        ALuint freeBuffer;

        //        alSourceUnqueueBuffers(oalSource->source, 1, &freeBuffer);

        //        streamPos -= sound->StreamData(freeBuffer, streamPos);
        //        alSourceQueueBuffers(oalSource->source, 1, &freeBuffer);

        //        if (streamPos < 0 && isLooping)
        //        {
        //            streamPos += sound->GetLength();
        //        }
        //    }
        //}
        //else
        //{
        alSourcei(emitter->oalSource->source, AL_LOOPING, emitter->isLooping ? 1 : 0);
        //}
    }
}

void gamedev::AudioSystem::CullEmitters(InstanceHandle handle) 
{
    auto emitter = mECS->GetComponent<SoundEmitter>(handle);
    auto instance = mECS->GetInstance(handle);

    float length;

    if (emitter->isGlobal)
    {
        length = 0.0f;
    }
    else
    {
        length = tg::length(mListenerPosition - instance.xform.translation);
    }

    if (length > emitter->maxRadius || !emitter->sound || emitter->timeLeft < 0)
    {
        DetachSource(handle);
    }
    else
    {
        emitters.push_back(handle);
    }
}

void gamedev::AudioSystem::AttachSource(InstanceHandle handle, OALSource* s)
{
    auto emitter = mECS->GetComponent<SoundEmitter>(handle);
    
    emitter->oalSource = s;

    if (!emitter->oalSource)
    {
        return;
    }

    emitter->oalSource->inUse = true;

    alSourceStop(emitter->oalSource->source);
    alSourcef(emitter->oalSource->source, AL_MAX_DISTANCE, emitter->maxRadius);
    alSourcef(emitter->oalSource->source, AL_REFERENCE_DISTANCE, emitter->minRadius);

    //// if(timeLeft > 0) {
    //if (sound->IsStreaming())
    //{ 
    //    streamPos = timeLeft;
    //    int numBuffered = 0;
    //    while (numBuffered < NUM_STREAM_BUFFERS)
    //    {
    //        double streamed = sound->StreamData(streamBuffers[numBuffered], streamPos);

    //        if (streamed)
    //        {
    //            streamPos -= streamed;
    //            ++numBuffered;
    //        }
    //        else
    //        {
    //            break;
    //        }
    //    }
    //    alSourceQueueBuffers(oalSource->source, numBuffered, &streamBuffers[0]);
    //}
    //else
    //{
    alSourcei(emitter->oalSource->source, AL_BUFFER, emitter->sound->GetBuffer());
    alSourcef(emitter->oalSource->source, AL_SEC_OFFSET, (emitter->sound->GetLength() / 1000.0) - (emitter->timeLeft / 1000.0));

    glow::info() << "Attaching Audio Source! Timeleft: " << (emitter->sound->GetLength() / 1000.0) - (emitter->timeLeft / 1000.0);
    alSourcePlay(emitter->oalSource->source);
    // }
    alSourcePlay(emitter->oalSource->source);
}

void gamedev::AudioSystem::DetachSource(InstanceHandle handle) 
{
    auto emitter = mECS->GetComponent<SoundEmitter>(handle);

    if (!emitter->oalSource)
    {
        return;
    }

    emitter->oalSource->inUse = false;

    alSourcef(emitter->oalSource->source, AL_GAIN, 0.0f);
    alSourceStop(emitter->oalSource->source);
    alSourcei(emitter->oalSource->source, AL_BUFFER, 0);

    //if (emitter->sound && emitter->sound->IsStreaming())
    //{
    //    int numProcessed = 0;
    //    ALuint tempBuffer;
    //    alGetSourcei(emitter->oalSource->source, AL_BUFFERS_PROCESSED, &numProcessed);
    //    while (numProcessed--)
    //    {
    //        alSourceUnqueueBuffers(emitter->oalSource->source, 1, &tempBuffer);
    //    }
    //}

    emitter->oalSource = NULL;
    glow::info() << "Source detached!";
    
}

void gamedev::AudioSystem::DetachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to)
{
    for (vector<InstanceHandle>::iterator i = from; i != to; ++i)
    {
        DetachSource(*i);
    }
}

void gamedev::AudioSystem::AttachSources(vector<InstanceHandle>::iterator from, vector<InstanceHandle>::iterator to)
{
    for (vector<InstanceHandle>::iterator i = from; i != to; ++i)
    {
        auto emitter = mECS->GetComponent<SoundEmitter>(*i);

        if (!emitter->oalSource) // Don't attach a new source if we already have one!
        { 
            AttachSource(*i, GetSource());
        }
    }
}

gamedev::OALSource* gamedev::AudioSystem::GetSource()
{
    for (vector<OALSource*>::iterator i = sources.begin(); i != sources.end(); ++i)
    {
        OALSource* s = *i;
        if (!s->inUse)
        {
            return s;
        }
    }
    return NULL;
}

bool gamedev::AudioSystem::CompareNodesByPriority(InstanceHandle handle1, InstanceHandle handle2)
{
    auto a = mECS->GetComponent<SoundEmitter>(handle1);
    auto b = mECS->GetComponent<SoundEmitter>(handle2);

    return (a->priority > b->priority) ? true : false;
}

void gamedev::AudioSystem::SetMasterVolume(float value)
{
    value = tg::clamp(value, 0.0f, 1.0f);
    masterVolume = value;
    alListenerf(AL_GAIN, masterVolume);
}

void gamedev::AudioSystem::SetVolume(int type, float value) 
{ 
    value = tg::clamp(value, 0.0f, 1.0f);
    volumes[type] = value;
}

void gamedev::AudioSystem::PlayLocalSound(InstanceHandle handle, std::string name, SoundType type, float volume, bool looping, float minRadius, float maxRadius, SoundPriority priority)
{
    auto sound = mECS->TryGetComponent<SoundEmitter>(handle);
    if (!sound)
        sound = mECS->CreateComponent<gamedev::SoundEmitter>(handle);
    sound->sound = Sound::GetSound(name);
    DetachSource(handle);
    if (sound->sound)
        sound->timeLeft = sound->sound->GetLength();
    sound->isLooping = looping;
    sound->isGlobal = false;
    sound->minRadius = minRadius;
    sound->maxRadius = maxRadius;
    sound->priority = priority;
    sound->volume = volume;
    sound->type = type;
}

void gamedev::AudioSystem::PlayLocalSound(tg::pos3 position, std::string name, SoundType type, float volume, bool looping, float minRadius, float maxRadius, SoundPriority priority)
{
    auto emitter = mECS->CreateInstance(NULL, NULL, NULL, NULL);
    auto& instance = mECS->GetInstance(emitter);
    instance.xform.translation = tg::vec3(position);

    auto sound = mECS->CreateComponent<gamedev::SoundEmitter>(emitter);
    sound->sound = Sound::GetSound(name);
    DetachSource(emitter);
    if (sound->sound)
        sound->timeLeft = sound->sound->GetLength();
    sound->isLooping = looping;
    sound->isGlobal = false;
    sound->minRadius = minRadius;
    sound->maxRadius = maxRadius;
    sound->priority = priority;
    sound->volume = volume;
    sound->type = type;

    mLocalEmitters.push_back(emitter);
    glow::info() << "Local emitter created!";
}

void gamedev::AudioSystem::PlayGlobalSound(std::string name, SoundType type, float volume, bool looping, SoundPriority priority)
{
    auto emitter = mECS->CreateInstance(NULL, NULL, NULL, NULL);
    auto sound = mECS->CreateComponent<gamedev::SoundEmitter>(emitter);
    sound->sound = Sound::GetSound(name);
    DetachSource(emitter);
    if (sound->sound)
        sound->timeLeft = sound->sound->GetLength();
    sound->isLooping = looping;
    sound->isGlobal = true;
    sound->priority = priority;
    sound->volume = volume;
    sound->type = type;
}

void gamedev::AudioSystem::CleanupLocalEmitters()
{
    for (vector<InstanceHandle>::iterator i = mLocalEmitters.begin(); i != mLocalEmitters.end();)
    {
        auto emitter = mECS->TryGetComponent<SoundEmitter>(*i);

        if (emitter->timeLeft <= 0 && !emitter->isLooping)
        {
            DetachSource(*i);
            // mECS->RemoveComponent<SoundEmitter>(*i);
            mECS->DestroyInstance(*i);
            i = mLocalEmitters.erase(i);
            glow::info() << "Local emitter erased!";
        }
        else
        {
            ++i;
        }
    }
}

void gamedev::AudioSystem::LoadSounds()
{
    /*Sound::AddSound("test_mono.wav");
    Sound::AddSound("test_stereo.wav");*/

    Sound::AddSound("arrow1.wav");
    Sound::AddSound("arrow2.wav");
    Sound::AddSound("arrow3.wav");
    Sound::AddSound("arrow4.wav");

    Sound::AddSound("claw1.wav");
    Sound::AddSound("claw2.wav");
    Sound::AddSound("claw3.wav");
    Sound::AddSound("claw4.wav");

    Sound::AddSound("wilhelm.wav");
    Sound::AddSound("pioneerDeathAlternative.wav");
    Sound::AddSound("monsterDeath.wav");

    Sound::AddSound("fire.wav");
    Sound::AddSound("wood.wav");
    Sound::AddSound("heal.wav");

    Sound::AddSound("interfaceClick.wav");
    Sound::AddSound("interfaceNegative.wav");

    Sound::AddSound("construction1.wav");
    Sound::AddSound("construction2.wav");
    Sound::AddSound("construction3.wav");
    Sound::AddSound("tent.wav");
    Sound::AddSound("supplyUpgrade.wav");

    Sound::AddSound("waveDrum1.wav");
    Sound::AddSound("explosion1.wav");

    Sound::AddSound("water.wav");
    Sound::AddSound("wind.wav");
    Sound::AddSound("crickets.wav");

    Sound::AddSound("music.wav");

    Sound::AddSound("victory.wav");
    Sound::AddSound("lost.wav");
}

void gamedev::AudioSystem::destroy()
{
    for (vector<OALSource*>::iterator i = sources.begin(); i != sources.end(); ++i)
    {
        alDeleteSources(1, &(*i)->source);
        delete (*i);
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}