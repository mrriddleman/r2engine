//
//  AudioEngine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-20.
//

#include "AudioEngine.h"
#include "fmod.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include <cstring>

namespace
{
    const u32 MAX_NUM_SOUNDS = 512;
    const u32 MAX_NUM_CHANNELS = 128;
    const u64 SOUND_ALIGNMENT = 512;
    const f32 SILENCE = 0.0f;
    const u32 VIRTUALIZE_FADE_TIME = 3 * 1000;
    
    FMOD_VECTOR GLMToFMODVector(const glm::vec3& vec)
    {
        FMOD_VECTOR v;
        v.x = vec.x;
        v.y = vec.y;
        v.z = vec.z;
        return v;
    }
    
    void CheckFMODResult(FMOD_RESULT result)
    {
        if (result != FMOD_OK)
        {
            R2_CHECK(result == FMOD_OK, "We had an error in FMOD: %i", result);
        }
        
    }
}

namespace r2::audio
{
    r2::mem::MemoryArea::MemorySubArea::Handle AudioEngine::mSoundMemoryAreaHandle = r2::mem::MemoryArea::MemorySubArea::Invalid;
    r2::mem::LinearArena* AudioEngine::mSoundAllocator = nullptr;
    
    const AudioEngine::SoundID AudioEngine::InvalidSoundID;
    const AudioEngine::ChannelID AudioEngine::InvalidChannelID;

    AudioEngine::SoundDefinition::SoundDefinition(const AudioEngine::SoundDefinition& soundDef)
    : defaultVolume(soundDef.defaultVolume)
    , minDistance(soundDef.minDistance)
    , maxDistance(soundDef.maxDistance)
    , flags(soundDef.flags)
    {
        strcpy(soundName, soundDef.soundName);
    }
    
    AudioEngine::SoundDefinition& AudioEngine::SoundDefinition::operator=(const AudioEngine::SoundDefinition& soundDef)
    {
        if (&soundDef == this)
        {
            return *this;
        }
        
        defaultVolume = soundDef.defaultVolume;
        minDistance = soundDef.minDistance;
        maxDistance = soundDef.maxDistance;
        flags = soundDef.flags;
        strcpy(soundName, soundDef.soundName);
        
        return *this;
    }

    //----------------------------IMPLEMENTATION-------------------------------------
    struct Sound
    {
        AudioEngine::SoundDefinition mSoundDefinition;
        FMOD::Sound* fmodSound = nullptr;
    };
    
    struct Implementation
    {
        struct Channel;
        
        struct AudioFader
        {
            AudioFader(Implementation& impl, Channel& channel);
            
            void Update(u32 milliseconds);
            bool IsFinished() const {return mDurationInMilliseconds == 0;}
            void StartFade(float startVolume, float endVolume, u32 durationInMilliseconds);
            
            Channel& mChannel;
            Implementation& mImpl;
            s32 mDurationInMilliseconds = 0;
        };
        
        struct Channel
        {
            enum class State
            {INITIALIZE, TOPLAY, LOADING, PLAYING, STOPPING, STOPPED, VIRTUALIZING, VIRTUAL, DEVIRTUALIZE, PAUSED};
            
            Implementation& mImpl;
            FMOD::Channel* moptrChannel = nullptr;
            AudioEngine::SoundID mSoundID = AudioEngine::InvalidSoundID;
            AudioEngine::SoundDefinition mSoundDef;
            glm::vec3 mPos;
            float mVolume = 0.0f;
            //float mFadeInTimeInSeconds = 0.0f;
            float mPitch = 1.0f;
            State mState = State::INITIALIZE;
            AudioFader mStopFader;
            AudioFader mVirtualizeFader;
            bool mStopRequested = false;
            bool mPauseRequested = false;
            
            Channel(Implementation& impl, AudioEngine::SoundID soundID, const AudioEngine::SoundDefinition& soundDef, const glm::vec3& pos, float volume, float pitch);
            void Update();
            void UpdateChannelParameters();
            bool ShouldBeVirtual(bool allowOneShotVirtuals) const;
            bool IsOneShot(){return false;}
            bool IsPlaying() const;
            float GetVolume() const;
            void Stop(float fadeOutInSeconds = 0.0f);
        };

        void Init(r2::mem::LinearArena& allocator);
        void Update();
        void Shutdown(r2::mem::LinearArena& allocator);
        bool SoundIsLoaded(AudioEngine::SoundID soundID);
        static u64 TotalAllocatedSize(u32 headerSize, u32 boundsCheckingSize);
        static u64 FMODMemorySize();
        
        using SoundList = r2::SArray<Sound>*;
        using ChannelList = r2::SArray<Channel*>*;
        
        FMOD::System* mSystem = nullptr;
        SoundList mSounds = nullptr;
        ChannelList mChannels = nullptr;
        byte* mFMODMemory = nullptr;
        r2::mem::PoolArena* mChannelPool = nullptr;
    };

    u64 Implementation::TotalAllocatedSize(u32 headerSize, u32 boundsCheckingSize)
    {
        u64 totalAllocationSizeNeeded = 0;
        
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(MAX_NUM_SOUNDS * sizeof(Sound) + headerSize + boundsCheckingSize + sizeof(SoundList), alignof(Sound)); //for mSounds
        
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(MAX_NUM_CHANNELS * sizeof(Channel*) + boundsCheckingSize + headerSize + sizeof(ChannelList), alignof(Channel*)); //for mChannels
        
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(MAX_NUM_CHANNELS * (sizeof(Channel) + boundsCheckingSize) + headerSize + boundsCheckingSize + sizeof(r2::mem::PoolArena), sizeof(Channel) + boundsCheckingSize ); //for mChannelPool
        
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(FMODMemorySize() + headerSize + boundsCheckingSize, 16); //for mSoundPool
        
        return totalAllocationSizeNeeded;
    }
    
    u64 Implementation::FMODMemorySize()
    {
        return Megabytes(4);
    }
    
    void Implementation::Init(r2::mem::LinearArena& allocator)
    {
        mSounds = MAKE_SARRAY(allocator, Sound, MAX_NUM_SOUNDS);
        
        R2_CHECK(mSounds != nullptr, "We should have sounds initialized!");
        
        for (u64 i = 0 ; i < MAX_NUM_SOUNDS; ++i)
        {
            Sound& sound = r2::sarr::At(*mSounds, i);
            strcpy(sound.mSoundDefinition.soundName, "");
            sound.fmodSound = nullptr;
        }
        
        mChannels = MAKE_SARRAY(allocator, Channel*, MAX_NUM_CHANNELS);
        
        R2_CHECK(mChannels != nullptr, "We should have channels initialized!");
        
        for (u64 i = 0 ; i < MAX_NUM_CHANNELS; ++i)
        {
            r2::sarr::At(*mChannels, i) = nullptr;
        }
        
        mChannelPool = MAKE_POOL_ARENA(allocator, sizeof(Channel), MAX_NUM_CHANNELS);
        mFMODMemory = ALLOC_BYTESN(allocator, FMODMemorySize(), 16);
        
        CheckFMODResult( FMOD::Memory_Initialize(mFMODMemory, FMODMemorySize(), 0, 0, 0) );
        CheckFMODResult( FMOD::System_Create(&mSystem) );
        CheckFMODResult( mSystem->init(MAX_NUM_CHANNELS, FMOD_INIT_NORMAL, nullptr));
    }
    
    void Implementation::Update()
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            Channel* channel = r2::sarr::At(*mChannels, i);
            
            if (channel)
            {
                channel->Update();
                
                if (channel->mState == Channel::State::STOPPED)
                {
                    FREE(channel, *mChannelPool);
                    r2::sarr::At(*mChannels, i) = nullptr;
                }
            }
        }
        
        CheckFMODResult( mSystem->update() );
    }
    
    void Implementation::Shutdown(r2::mem::LinearArena& allocator)
    {
        CheckFMODResult( mSystem->release() );
        
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            Channel* channel = r2::sarr::At(*mChannels, i);
            if (channel)
            {
                FREE(channel, *mChannelPool);
            }
        }

        //Free in reverse order
        FREE(mFMODMemory, allocator);
        FREE(mChannelPool, allocator);
        FREE(mChannels, allocator);
        FREE(mSounds, allocator);
        
        mFMODMemory = nullptr;
        mChannels = nullptr;
        mChannelPool = nullptr;
        mSounds = nullptr;
    }
    
    //--------------------------------AUDIO FADER IMPL---------------------------------------
    Implementation::AudioFader::AudioFader(Implementation& impl, Channel& channel)
        : mImpl(impl)
        , mChannel(channel)
    {
    }
    
    void Implementation::AudioFader::Update(u32 milliseconds)
    {
        mDurationInMilliseconds -= milliseconds;
        
        if (mDurationInMilliseconds <= 0 )
        {
            mDurationInMilliseconds = 0;
        }
    }
    
    void Implementation::AudioFader::StartFade(float startVolume, float endVolume, u32 durationInMilliseconds)
    {
        if (!mChannel.moptrChannel)
        {
            return;
        }
        
        mDurationInMilliseconds = static_cast<s32>(durationInMilliseconds);
        
        unsigned long long dspClock;
        int rate;
        
        CheckFMODResult( mImpl.mSystem->getSoftwareFormat(&rate, 0, 0) );
        
        CheckFMODResult( mChannel.moptrChannel->getDSPClock(0, &dspClock) );
        
        CheckFMODResult( mChannel.moptrChannel->addFadePoint(dspClock, startVolume));
        
        CheckFMODResult( mChannel.moptrChannel->addFadePoint(dspClock + (rate * r2::util::MillisecondsToSeconds(durationInMilliseconds)), endVolume));
        
    }
    
    //----------------------------CHANNEL IMPLEMENTATION-------------------------------------
    Implementation::Channel::Channel(Implementation& impl, AudioEngine::SoundID soundID, const AudioEngine::SoundDefinition& soundDef, const glm::vec3& pos, float volume, float pitch)
        : mImpl(impl)
        , mSoundID(soundID)
        , mSoundDef(soundDef)
        , mPos(pos)
        , mVolume(volume)
        , mPitch(pitch)
        , mStopFader(impl, *this)
        , mVirtualizeFader(impl, *this)
    {
        
    }
    
    void Implementation::Channel::Update()
    {
        switch (mState)
        {
            case Implementation::Channel::State::INITIALIZE:
            case Implementation::Channel::State::DEVIRTUALIZE:
            case Implementation::Channel::State::TOPLAY:
            {
                if (mStopRequested)
                {
                    mState = State::STOPPING;
                    return;
                }
                
                if (ShouldBeVirtual(true))
                {
                    if (IsOneShot())
                    {
                        mState = State::STOPPING;
                    }
                    else
                    {
                        mState = State::VIRTUAL;
                    }
                    return;
                }
                
                R2_CHECK(mSoundID != AudioEngine::InvalidSoundID, "mSoundID is invalid!");
                
                if (!mImpl.SoundIsLoaded(mSoundID))
                {
                    AudioEngine engine;
                    engine.LoadSound(mSoundID);
                    mState = State::LOADING;
                    return;
                }
                
                moptrChannel = nullptr;
                
                Sound sound = r2::sarr::At(*mImpl.mSounds, mSoundID);
                if (sound.fmodSound)
                {
                    CheckFMODResult(mImpl.mSystem->playSound(sound.fmodSound, nullptr, true, &moptrChannel));
                }
                
                if (moptrChannel)
                {
                    if (mState == State::DEVIRTUALIZE)
                    {
                        mVirtualizeFader.StartFade(SILENCE, GetVolume(), VIRTUALIZE_FADE_TIME);
                    }
                    
                    mState = State::PLAYING;
                    UpdateChannelParameters();
                    CheckFMODResult( moptrChannel->setPaused(false) );
                }
                else
                {
                    mState = State::STOPPING;
                }
            }
            break;
            case Implementation::Channel::State::LOADING:
            {
                if (mImpl.SoundIsLoaded(mSoundID))
                {
                    mState = State::TOPLAY;
                }
            }
            break;
            case Implementation::Channel::State::PLAYING:
            {
                mVirtualizeFader.Update(CPLAT.TickRate());
                UpdateChannelParameters();
                
                if (!IsPlaying())
                {
                    mState = State::STOPPED;
                    return;
                }
                
                if (mStopRequested)
                {
                    mState = State::STOPPING;
                    return;
                }

                if (mPauseRequested)
                {
                    mPauseRequested = false;
                    mState = State::PAUSED;
                    CheckFMODResult( moptrChannel->setPaused(true) );
                    return;
                }
                
                if (ShouldBeVirtual(false))
                {
                    mVirtualizeFader.StartFade(SILENCE, GetVolume(), VIRTUALIZE_FADE_TIME);
                    mState = State::VIRTUALIZING;
                }
            }
            break;
            case Implementation::Channel::State::STOPPING:
            {
                mStopFader.Update(CPLAT.TickRate());
                UpdateChannelParameters();
                
                if (mStopFader.IsFinished())
                {
                    moptrChannel->stop();
                    mState = State::STOPPED;
                    return;
                }
            }
            break;
            case Implementation::Channel::State::STOPPED:
            break;
            case Implementation::Channel::State::VIRTUALIZING:
            {
                mVirtualizeFader.Update(CPLAT.TickRate());
                UpdateChannelParameters();
                if (!ShouldBeVirtual(false))
                {
                    mVirtualizeFader.StartFade(GetVolume(), SILENCE, VIRTUALIZE_FADE_TIME);
                    mState = State::PLAYING;
                    break;
                }
                if (mVirtualizeFader.IsFinished())
                {
                    CheckFMODResult( moptrChannel->stop() );
                    mState = State::VIRTUAL;
                }
            }
            break;
            case Implementation::Channel::State::VIRTUAL:
            {
                if (mStopRequested)
                {
                    mState = State::STOPPING;
                }
                else if(!ShouldBeVirtual(false))
                {
                    mState = State::DEVIRTUALIZE;
                }
            }
            break;
            case Implementation::Channel::State::PAUSED:
            {
                if (mPauseRequested)
                {
                    moptrChannel->setPaused(false);
                    mPauseRequested = false;
                    mState = State::PLAYING;
                }
            }
            break;
            default:
                break;
        }
    }
    
    void Implementation::Channel::UpdateChannelParameters()
    {
        if (!moptrChannel)
        {
            return;
        }
        
        FMOD_VECTOR position = GLMToFMODVector(mPos);
        
        if (mSoundDef.flags.IsSet(r2::audio::AudioEngine::IS_3D))
        {
            CheckFMODResult( moptrChannel->set3DAttributes(&position, nullptr) );
        }
        
        CheckFMODResult( moptrChannel->setVolume(GetVolume()) );
        
        CheckFMODResult( moptrChannel->setPitch(mPitch) );
    }
    
    bool Implementation::Channel::ShouldBeVirtual(bool allowOneShotVirtuals) const
    {
        //?
        return false;
    }
    
    bool Implementation::Channel::IsPlaying() const
    {
        if (!moptrChannel)
        {
            return false;
        }
        
        bool isPlaying = false;
        CheckFMODResult(moptrChannel->isPlaying(&isPlaying));
        return isPlaying;
    }
    
    float Implementation::Channel::GetVolume() const
    {
        if (!moptrChannel)
        {
            return 0.0f;
        }
        
        float volume = 0.0f;
        CheckFMODResult(moptrChannel->getVolume(&volume));
        return volume;
    }

    void Implementation::Channel::Stop(float fadeOutInSeconds)
    {
        if (!moptrChannel)
        {
            return;
        }
        
        if (fadeOutInSeconds <= 0.0f)
        {
            CheckFMODResult( moptrChannel->stop() );
            mState = Implementation::Channel::State::STOPPED;
        }
        else
        {
            mStopRequested = true;
            mStopFader.StartFade(GetVolume(), SILENCE, r2::util::SecondsToMilliseconds(fadeOutInSeconds));
        }
    }
    
    //--------------------------AUDIO ENGINE-----------------------------------
    
    static Implementation* gImpl = nullptr;
    static bool gAudioEngineInitialize = false;
    
    void AudioEngine::Init()
    {
        if (gAudioEngineInitialize)
        {
            return;
        }
        
        //do memory setup here
        {
            r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
            
            u32 boundsChecking = 0;
#ifdef R2_DEBUG
            boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
            
            u64 soundAreaSize =
            sizeof(r2::mem::LinearArena) +
            sizeof(Implementation) +
            Implementation::TotalAllocatedSize(r2::mem::LinearAllocator::HeaderSize(), boundsChecking);
            
            soundAreaSize = r2::mem::utils::GetMaxMemoryForAllocation(soundAreaSize, SOUND_ALIGNMENT);

            AudioEngine::mSoundMemoryAreaHandle = r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle)->AddSubArea(soundAreaSize);
            
            R2_CHECK(AudioEngine::mSoundMemoryAreaHandle != r2::mem::MemoryArea::MemorySubArea::Invalid, "We have an invalid sound memory area!");
            
            AudioEngine::mSoundAllocator = EMPLACE_LINEAR_ARENA(*r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle)->GetSubArea(AudioEngine::mSoundMemoryAreaHandle));
            
            R2_CHECK(AudioEngine::mSoundAllocator != nullptr, "We have an invalid linear arean for sound!");
        }

        if (gImpl == nullptr)
        {
            gImpl = ALLOC(Implementation, *AudioEngine::mSoundAllocator);
            
            R2_CHECK(gImpl != nullptr, "Something went wrong trying to allocate the implementation for sound!");
            
            gImpl->Init(*AudioEngine::mSoundAllocator);
        }
        
        gAudioEngineInitialize = true;
    }
    
    void AudioEngine::Update()
    {
        gImpl->Update();
    }
    
    void AudioEngine::Shutdown()
    {
        AudioEngine audio;
        audio.StopAllChannels();
        
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            audio.UnloadSound(i);
        }
        
        gImpl->Shutdown(*AudioEngine::mSoundAllocator);
        
        FREE(gImpl, *AudioEngine::mSoundAllocator);
        gImpl = nullptr;
        gAudioEngineInitialize = false;
    }
    
    AudioEngine::SoundID AudioEngine::RegisterSound(const SoundDefinition& soundDef, bool load)
    {
        //check to see if this was already registered
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            const Sound& sound = r2::sarr::At(*gImpl->mSounds, i);
            if (strcmp(sound.mSoundDefinition.soundName, soundDef.soundName) == 0)
            {
                //return the index that we already had for this sound definition
                R2_CHECK(false, "AudioEngine::RegisterSound - sound definition has already been registered!");
                return i;
            }
        }
        
        auto soundID = NextAvailableSoundID();
        
        if (soundID == InvalidSoundID)
        {
            R2_CHECK(false, "AudioEngine::RegisterSound - We couldn't find a new spot for the sound definition! We must be out of space!");
            return soundID;
        }
        
        //copy over the new sound definition
        Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
        sound.mSoundDefinition.flags = soundDef.flags;
        sound.mSoundDefinition.maxDistance = soundDef.maxDistance;
        sound.mSoundDefinition.minDistance = soundDef.minDistance;
        sound.mSoundDefinition.defaultVolume = soundDef.defaultVolume;
        strcpy( sound.mSoundDefinition.soundName, soundDef.soundName );
        
        if (load)
        {
            bool result = LoadSound(soundID);
            R2_CHECK(result, "AudioEngine::RegisterSound - we couldn't load the sound: %s", soundDef.soundName);
        }
    
        return soundID;
    }
    
    void AudioEngine::UnregisterSound(SoundID soundID)
    {
        if (soundID == InvalidSoundID)
        {
            R2_CHECK(false, "AudioEngine::UnregisterSound - the soundID passed in is Invalid!");
            return;
        }
        
        Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
        
        if (strcmp(sound.mSoundDefinition.soundName, "") == 0)
        {
            R2_CHECK(false, "AudioEngine::UnregisterSound - we've already unregistered this sound: %lli!", soundID);
            return;
        }
        
        if (sound.fmodSound)
        {
            UnloadSound(soundID);
        }
        
        strcpy(sound.mSoundDefinition.soundName, "");
        sound.mSoundDefinition.defaultVolume = 0.0f;
        sound.mSoundDefinition.flags.Clear();
        sound.mSoundDefinition.minDistance = 0.0f;
        sound.mSoundDefinition.maxDistance = 0.0f;
    }
    
    bool AudioEngine::LoadSound(SoundID soundID)
    {
        if (gImpl->SoundIsLoaded(soundID))
        {
            return true;
        }
        
        Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
        if (strcmp( sound.mSoundDefinition.soundName, "") == 0)
        {
            R2_CHECK(false, "AudioEngine::LoadSound - there's no definition for the sound!");
            return false;
        }
        
        FMOD_MODE mode = FMOD_DEFAULT;//FMOD_NONBLOCKING;
        mode |= sound.mSoundDefinition.flags.IsSet(IS_3D) ? (FMOD_3D | FMOD_3D_INVERSETAPEREDROLLOFF) : FMOD_2D;
        mode |= sound.mSoundDefinition.flags.IsSet(LOOP) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        mode |= sound.mSoundDefinition.flags.IsSet(STREAM) ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
        
        CheckFMODResult( gImpl->mSystem->createSound(sound.mSoundDefinition.soundName, mode, nullptr, &sound.fmodSound) );
        
        if (sound.fmodSound)
        {
            if (sound.mSoundDefinition.flags.IsSet(IS_3D))
            {
                CheckFMODResult( sound.fmodSound->set3DMinMaxDistance(sound.mSoundDefinition.minDistance, sound.mSoundDefinition.maxDistance) );
            }
            return true;
        }
        
        R2_CHECK(false, "AudioEngine::LoadSound - Failed to create sound: %s", sound.mSoundDefinition.soundName);
        
        return false;
    }
    
    void AudioEngine::UnloadSound(SoundID soundID)
    {
        if (soundID == InvalidSoundID)
        {
            R2_CHECK(false, "AudioEngine::UnloadSound - you passed in an invalid soundID!");
            return;
        }
        
        Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
        
        if (!sound.fmodSound)
        {
            return;
        }
        
        CheckFMODResult(sound.fmodSound->release());
        sound.fmodSound = nullptr;
    }
    
    void AudioEngine::Set3DListenerAndOrientation(const glm::vec3& position, const glm::vec3& look, const glm::vec3& up)
    {
        FMOD_VECTOR pos = GLMToFMODVector(position);
        FMOD_VECTOR forward = GLMToFMODVector(look);
        FMOD_VECTOR upVec = GLMToFMODVector(up);
        
        CheckFMODResult( gImpl->mSystem->set3DListenerAttributes(0, &pos, nullptr, &forward, &upVec) );
    }
    
    AudioEngine::ChannelID AudioEngine::PlaySound(AudioEngine::SoundID soundID, const glm::vec3& pos, float volume, float pitch)
    {
        if (soundID == InvalidSoundID)
        {
            R2_CHECK(false, "AudioEngine::PlaySound - passed in invalid SoundID");
            return InvalidChannelID;
        }
        
        if (!gImpl->SoundIsLoaded(soundID))
        {
            R2_CHECK(false, "AudioEngine::PlaySound - tried to play a sound that isn't loaded");
            return InvalidChannelID;
        }

        const Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
        if (strcmp( sound.mSoundDefinition.soundName, "") == 0)
        {
            R2_CHECK(false, "AudioEngine::PlaySound - we don't have a proper definition for soundID: %lli", soundID);
            return InvalidChannelID;
        }
        
        ChannelID nextChannelID = NextAvailableChannelID();
        
        if (nextChannelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::PlaySound - we don't have anymore channel slots available to play: %s", sound.mSoundDefinition.soundName);
            return InvalidChannelID;
        }
        
        r2::sarr::At( *gImpl->mChannels, nextChannelID ) = ALLOC_PARAMS(Implementation::Channel, *gImpl->mChannelPool, *gImpl, soundID, sound.mSoundDefinition, pos, volume, pitch);
        
        return nextChannelID;
    }
    
    void AudioEngine::StopChannel(AudioEngine::ChannelID channelID, float fadeoutInSeconds)
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::StopChannel - channelID is Invalid!");
            return;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::StopChannel - channel is null?");
            return;
        }
        
        channel->Stop(fadeoutInSeconds);
    }
    
    void AudioEngine::PauseChannel(AudioEngine::ChannelID channelID)
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::PauseChannel - channelID is Invalid!");
            return;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::PauseChannel - channel is null?");
            return;
        }
        
        channel->mPauseRequested = true;
    }
    
    void AudioEngine::PauseAll()
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
            
            if (channel)
            {
                PauseChannel(i);
            }
        }
    }
    
    void AudioEngine::StopAllChannels(float fadeOutInSeconds)
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
            
            if (channel)
            {
                StopChannel(i, fadeOutInSeconds);
            }
        }
    }
    
    void AudioEngine::SetChannel3DPosition(AudioEngine::ChannelID channelID, const glm::vec3& position)
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::SetChannel3DPosition - channelID is Invalid!");
            return;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::SetChannel3DPosition - channel is null?");
            return;
        }
        
        channel->mPos = position;
    }
    
    void AudioEngine::SetChannelVolume(AudioEngine::ChannelID channelID, float volume)
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::SetChannelVolume - channelID is Invalid!");
            return;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::SetChannelVolume - channel is null?");
            return;
        }
        
        channel->mVolume = volume;
    }
    
    bool AudioEngine::IsChannelPlaying(AudioEngine::ChannelID channelID) const
    {
        
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::IsChannelPlaying - channelID is Invalid!");
            return false;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::IsChannelPlaying - channel is null?");
            return false;
        }
        
        return channel->IsPlaying();
    }
    
    float AudioEngine::GetChannelPitch(AudioEngine::ChannelID channelID) const
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::GetChannelPitch - channelID is Invalid!");
            return false;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::GetChannelPitch - channel is null?");
            return false;
        }
        
        return channel->mPitch;
    }
    
    void AudioEngine::SetChannelPitch(AudioEngine::ChannelID channelID, float pitch)
    {
        if (channelID == InvalidChannelID)
        {
            R2_CHECK(false, "AudioEngine::SetChannelPitch - channelID is Invalid!");
            return;
        }
        
        Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (channel == nullptr)
        {
            R2_CHECK(false, "AudioEngine::SetChannelPitch - channel is null?");
            return;
        }
        
        channel->mPitch = pitch;
    }
 
    AudioEngine::SoundID AudioEngine::NextAvailableSoundID()
    {
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            const Sound& sound = r2::sarr::At(*gImpl->mSounds, i);
            if (strcmp(sound.mSoundDefinition.soundName, "") == 0)
            {
                return i;
            }
        }
        
        return AudioEngine::InvalidSoundID;
    }
                         
    AudioEngine::ChannelID AudioEngine::NextAvailableChannelID()
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            if (r2::sarr::At(*gImpl->mChannels, i) == nullptr)
            {
                return i;
            }
        }
        
        return AudioEngine::InvalidChannelID;
    }
    
    bool Implementation::SoundIsLoaded(AudioEngine::SoundID soundID)
    {
        if (soundID == AudioEngine::InvalidSoundID)
        {
            R2_CHECK(false, "Implementation::SoundIsLoaded - you're passing an invalid soundID!");
            return false;
        }
        
        const Sound& sound = r2::sarr::At(*mSounds, soundID);
        return sound.fmodSound != nullptr;
    }
}
