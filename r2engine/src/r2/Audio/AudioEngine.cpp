//
//  AudioEngine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-20.
//

#include "r2pch.h"
#include "AudioEngine.h"
#include "fmod.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

#include <cstring>

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#endif

namespace
{
    const u32 MAX_NUM_SOUNDS = 512;
    const u32 MAX_NUM_CHANNELS = 128;
    const u64 SOUND_ALIGNMENT = 512;
    const u32 MAX_NUM_DEFINITIONS = MAX_NUM_SOUNDS*2;
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
#ifdef R2_ASSET_PIPELINE
    r2::asset::pln::AssetThreadSafeQueue<std::vector<std::string>> s_soundDefsBuiltQueue;
#endif
    
}

namespace r2::audio
{
    r2::mem::MemoryArea::MemorySubArea::Handle AudioEngine::mSoundMemoryAreaHandle = r2::mem::MemoryArea::MemorySubArea::Invalid;
    r2::mem::LinearArena* AudioEngine::mSoundAllocator = nullptr;
    
    const AudioEngine::SoundID AudioEngine::InvalidSoundID;
    const AudioEngine::ChannelID AudioEngine::InvalidChannelID;

    AudioEngine::SoundDefinition::SoundDefinition(const AudioEngine::SoundDefinition& soundDef)
    : defaultVolume(soundDef.defaultVolume)
    , defaultPitch(soundDef.defaultPitch)
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
        defaultPitch = soundDef.defaultPitch;
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
        using DefinitionMap = r2::SHashMap<AudioEngine::SoundID>*;
        
        FMOD::System* mSystem = nullptr;
        SoundList mSounds = nullptr;
        DefinitionMap mDefinitions = nullptr;
        ChannelList mChannels = nullptr;
        byte* mFMODMemory = nullptr;
        r2::mem::PoolArena* mChannelPool = nullptr;
    };

    u64 Implementation::TotalAllocatedSize(u32 headerSize, u32 boundsCheckingSize)
    {
        u64 totalAllocationSizeNeeded = 0;
        
        u64 sizeForSystem = r2::mem::utils::GetMaxMemoryForAllocation( sizeof(FMOD::System) + sizeof(FMOD::System*), SOUND_ALIGNMENT); //for mSystem

        totalAllocationSizeNeeded += sizeForSystem;
        
        u64 sizeForSounds = r2::mem::utils::GetMaxMemoryForAllocation(SArray<Sound>::MemorySize(MAX_NUM_SOUNDS) + headerSize + boundsCheckingSize + sizeof(SoundList), SOUND_ALIGNMENT); //for mSounds

        totalAllocationSizeNeeded += sizeForSounds;
        
        u64 sizeForDefinitions = r2::mem::utils::GetMaxMemoryForAllocation(SHashMap<AudioEngine::SoundID>::MemorySize(MAX_NUM_DEFINITIONS) + sizeof(DefinitionMap) + headerSize + boundsCheckingSize, SOUND_ALIGNMENT); //for mDefinitions

        totalAllocationSizeNeeded += sizeForDefinitions;
        
        u64 sizeForChannelList = r2::mem::utils::GetMaxMemoryForAllocation( SArray<Channel*>::MemorySize(MAX_NUM_CHANNELS) + boundsCheckingSize + headerSize + sizeof(ChannelList), SOUND_ALIGNMENT); //for mChannels

        totalAllocationSizeNeeded += sizeForChannelList;
        
        u64 sizeForChannelPool = r2::mem::utils::GetMaxMemoryForAllocation(MAX_NUM_CHANNELS * (sizeof(Channel) + boundsCheckingSize) + headerSize + boundsCheckingSize + sizeof(r2::mem::PoolArena), SOUND_ALIGNMENT); //for mChannelPool

        totalAllocationSizeNeeded += sizeForChannelPool;
        
        u64 sizeForFMODMemory = r2::mem::utils::GetMaxMemoryForAllocation(FMODMemorySize() + sizeof(byte*) + headerSize + boundsCheckingSize, SOUND_ALIGNMENT); //for mFMODMemory

        totalAllocationSizeNeeded += sizeForFMODMemory;
        
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
        
        mDefinitions = MAKE_SHASHMAP(allocator, AudioEngine::SoundID, MAX_NUM_DEFINITIONS);
        
        R2_CHECK(mDefinitions != nullptr, "Failed to allocate mDefinitions");
        
        mChannels = MAKE_SARRAY(allocator, Channel*, MAX_NUM_CHANNELS);
        
        R2_CHECK(mChannels != nullptr, "We should have channels initialized!");
        
        for (u64 i = 0 ; i < MAX_NUM_CHANNELS; ++i)
        {
            r2::sarr::At(*mChannels, i) = nullptr;
        }
        
        mChannelPool = MAKE_POOL_ARENA(allocator, sizeof(Channel), MAX_NUM_CHANNELS);
 
        mFMODMemory = ALLOC_BYTESN(allocator, FMODMemorySize(), SOUND_ALIGNMENT);
        
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
                r2::sarr::At(*mChannels, i) = nullptr;
            }
            
            
        }

        //Free in reverse order
        FREE(mFMODMemory, allocator);
        FREE(mChannelPool, allocator);
        FREE(mChannels, allocator);
        FREE(mDefinitions, allocator);
        FREE(mSounds, allocator);
        
        mFMODMemory = nullptr;
        mChannels = nullptr;
        mChannelPool = nullptr;
        mDefinitions = nullptr;
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
        
        CheckFMODResult( moptrChannel->setVolume(mVolume) );
        
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
        
        return mVolume;
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
            R2_LOGI("sound area size: %llu\n", soundAreaSize);
            
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
#ifdef R2_ASSET_PIPELINE
        std::vector<std::string> paths;
        
        if(gAudioEngineInitialize && s_soundDefsBuiltQueue.TryPop(paths))
        {
            //TODO(Serge): could be smarter and only unload the sounds that were actually modified
            AudioEngine audio;
            audio.StopAllChannels();

            for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
            {
                audio.UnloadSound(i);
            }
            
            for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
            {
                r2::audio::Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
                if (channel)
                {
                    FREE(channel, *gImpl->mChannelPool);
                    r2::sarr::At(*gImpl->mChannels, i) = nullptr;
                }
            }

            ReloadSoundDefinitions(CPLAT.SoundDefinitionsPath().c_str());
        }
#endif
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
        
        FREE_EMPLACED_ARENA(AudioEngine::mSoundAllocator);
    }
    
#ifdef R2_ASSET_PIPELINE
    void AudioEngine::PushNewlyBuiltSoundDefinitions(std::vector<std::string> paths)
    {
        s_soundDefsBuiltQueue.Push(paths);
    }
#endif
    
    void AudioEngine::ReloadSoundDefinitions(const char* path)
    {
        if (gAudioEngineInitialize)
        {
            r2::fs::File* file = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), path, r2::fs::Read | r2::fs::Binary);
            
      //      R2_CHECK(file != nullptr, "AudioEngine::ReloadSoundDefinitions - We couldn't open the file: %s\n", path);
            if(!file)
            {
                R2_LOGE("AudioEngine::ReloadSoundDefinitions - We couldn't open the file: %s\n", path);
                return;
            }
            
            
            if (file)
            {
                u64 fileSize = file->Size();
                
                byte* fileBuf = (byte*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize, 4);
                
                R2_CHECK(fileBuf != nullptr, "Failed to allocate bytes for sound definitions");
                
                if (!fileBuf)
                {
                    return;
                }
                
                bool succeeded = file->ReadAll(fileBuf);
                
                R2_CHECK(succeeded, "Failed to read the whole file!");
                
                if (!succeeded)
                {
                    FREE(fileBuf, *MEM_ENG_SCRATCH_PTR);
                    return;
                }
                
                auto soundDefinitions = r2::GetSoundDefinitions(fileBuf);
                
                const auto size = soundDefinitions->definitions()->Length();
                
                for (u32 i = 0; i < size; ++i)
                {
                    const r2::SoundDefinition* def = soundDefinitions->definitions()->Get(i);
                    
                    SoundDefinition soundDef;
                    strcpy(soundDef.soundName, def->soundName()->c_str());
                    soundDef.minDistance = def->minDistance();
                    soundDef.maxDistance = def->maxDistance();
                    soundDef.defaultVolume = def->defaultVolume();
                    soundDef.defaultPitch = def->pitch();
                    soundDef.loadOnRegister = def->loadOnRegister();
                    
                    if (def->loop())
                    {
                        soundDef.flags |= LOOP;
                    }
                    
                    if (def->is3D())
                    {
                        soundDef.flags |= IS_3D;
                    }
                    
                    if (def->stream())
                    {
                        soundDef.flags |= STREAM;
                    }
                    AudioEngine audio;
                    char fileName[r2::fs::FILE_PATH_LENGTH];
                    r2::fs::utils::CopyFileNameWithExtension(soundDef.soundName, fileName);
                    
                    soundDef.soundKey = STRING_ID(fileName);
                    SoundID soundID = audio.RegisterSound(soundDef);
                    
                    r2::shashmap::Set(*gImpl->mDefinitions, soundDef.soundKey, soundID);
                }
                
                FREE(fileBuf, *MEM_ENG_SCRATCH_PTR);
                
                r2::fs::FileSystem::Close(file);
            }
        }
    }
    
    AudioEngine::SoundID AudioEngine::RegisterSound(const SoundDefinition& soundDef)
    {
        //check to see if this was already registered
        SoundID soundID = InvalidSoundID;
        
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            const Sound& sound = r2::sarr::At(*gImpl->mSounds, i);
            if (strcmp(sound.mSoundDefinition.soundName, soundDef.soundName) == 0)
            {
                //return the index that we already had for this sound definition
                soundID = i;
                break;
            }
        }
        
        if (soundID == InvalidSoundID)
        {
            soundID = NextAvailableSoundID();
        }
        
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
        sound.mSoundDefinition.defaultPitch = soundDef.defaultPitch;
        strcpy( sound.mSoundDefinition.soundName, soundDef.soundName );
        
        if (soundDef.loadOnRegister)
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
        sound.mSoundDefinition.defaultPitch = 0.0f;
        sound.mSoundDefinition.flags.Clear();
        sound.mSoundDefinition.minDistance = 0.0f;
        sound.mSoundDefinition.maxDistance = 0.0f;
        sound.mSoundDefinition.flags = NONE;
        sound.mSoundDefinition.loadOnRegister = false;
    }
    
    bool AudioEngine::LoadSound(const char* soundName)
    {
        SoundID defaultID = AudioEngine::InvalidSoundID;
        SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, STRING_ID(soundName), defaultID);
        
        if (theSoundID == defaultID)
        {
            R2_CHECK(false, "AudioEngine::LoadSound - Couldn't find sound: %s", soundName);
            return false;
        }
        
        return LoadSound(theSoundID);
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
    
    AudioEngine::ChannelID AudioEngine::PlaySound(const char* soundName, const glm::vec3& pos, float volume, float pitch)
    {
        SoundID defaultID = AudioEngine::InvalidSoundID;
        SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, STRING_ID(soundName), defaultID);
        
        if (theSoundID == defaultID)
        {
            R2_CHECK(false, "AudioEngine::PlaySound - Couldn't find sound: %s", soundName);
            return InvalidChannelID;
        }
        
        return PlaySound(theSoundID, pos, volume, pitch);
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
        
        float volumeToPlay = volume > 0.0f ? volume : sound.mSoundDefinition.defaultVolume;
        float pitchToPlay = pitch > 0.0f ? pitch : sound.mSoundDefinition.defaultPitch;
        
        r2::sarr::At( *gImpl->mChannels, nextChannelID ) = ALLOC_PARAMS(Implementation::Channel, *gImpl->mChannelPool, *gImpl, soundID, sound.mSoundDefinition, pos, volumeToPlay, pitchToPlay);
        
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
    
    int AudioEngine::GetSampleRate() const
    {
        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return sampleRate;
    }
    
    AudioEngine::SpeakerMode AudioEngine::GetSpeakerMode() const
    {
        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return static_cast<SpeakerMode>(speakerMode);
    }
    
    u32 AudioEngine::GetNumberOfDrivers() const
    {
        int numDrivers = 0;
        CheckFMODResult(gImpl->mSystem->getNumDrivers(&numDrivers));
        return numDrivers;
    }
    
    u32 AudioEngine::GetCurrentDriver() const
    {
        int driverId;
        CheckFMODResult(gImpl->mSystem->getDriver(&driverId));
        return driverId;
    }
    
    void AudioEngine::SetDriver(int driverId)
    {
        u32 numDrivers = GetNumberOfDrivers();
        
        R2_CHECK(driverId >= 0 && driverId < numDrivers, "Passed in a driver id that isn't in range");
        
        CheckFMODResult(gImpl->mSystem->setDriver(driverId));
    }
    
    void AudioEngine::GetDriverInfo(int driverId, char* driverName, u32 driverNameLength, u32& systemRate, SpeakerMode& mode, u32& speakerModeChannels)
    {
        u32 numDrivers = GetNumberOfDrivers();
        
        R2_CHECK(driverId >= 0 && driverId < numDrivers, "Passed in a driver id that isn't in range");
        int rate, channels;
        FMOD_SPEAKERMODE fmodSpeakerMode;
        CheckFMODResult(gImpl->mSystem->getDriverInfo(driverId, driverName, driverNameLength, nullptr, &rate, &fmodSpeakerMode, &channels));
        
        systemRate = rate;
        mode = static_cast<SpeakerMode>(fmodSpeakerMode);
        speakerModeChannels = channels;
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
