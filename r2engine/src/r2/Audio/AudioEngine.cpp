//
//  AudioEngine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-20.
//

#include "r2pch.h"
#include "AudioEngine.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetLib.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#endif

namespace
{
    const u32 MAX_NUM_SOUNDS = 512;
    const u32 MAX_NUM_CHANNELS = 128;
    const u64 SOUND_ALIGNMENT = 16;
    const u32 MAX_NUM_DEFINITIONS = MAX_NUM_SOUNDS*2;
    const f32 SILENCE = 0.0f;
    const u32 VIRTUALIZE_FADE_TIME = 3 * 1000;
    const u32 MAX_NUM_BANKS = 100;
    const u32 MAX_NUM_EVENT_INSTANCES = 5000;

    FMOD_VECTOR GLMToFMODVector(const glm::vec3& vec)
    {
        FMOD_VECTOR v;
        v.x = vec.x;
        v.y = vec.y;
        v.z = vec.z;
        return v;
    }

    void Attributes3DToFMOD3DAttributes(const r2::audio::AudioEngine::Attributes3D& attributes, FMOD_3D_ATTRIBUTES& fmodAttributes)
    {
        fmodAttributes.forward = GLMToFMODVector(attributes.look);
        fmodAttributes.position = GLMToFMODVector(attributes.position);
        fmodAttributes.up = GLMToFMODVector(attributes.up);
        fmodAttributes.velocity = GLMToFMODVector(attributes.velocity);
    }
    
    void CheckFMODResult(FMOD_RESULT result)
    {
        if (result != FMOD_OK && result != FMOD_ERR_INVALID_HANDLE)
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
    r2::mem::MemoryArea::SubArea::Handle AudioEngine::mSoundMemoryAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
   // r2::mem::LinearArena* AudioEngine::mSoundAllocator = nullptr;
    
   // const AudioEngine::SoundID AudioEngine::InvalidSoundID;
   // const AudioEngine::ChannelID AudioEngine::InvalidChannelID;

   /* AudioEngine::SoundDefinition::SoundDefinition(const AudioEngine::SoundDefinition& soundDef)
    : defaultVolume(soundDef.defaultVolume)
    , defaultPitch(soundDef.defaultPitch)
    , minDistance(soundDef.minDistance)
    , maxDistance(soundDef.maxDistance)
    , flags(soundDef.flags)
    {
        r2::util::PathCpy(soundName, soundDef.soundName);
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
        r2::util::PathCpy(soundName, soundDef.soundName);
        
        return *this;
    }*/

    //----------------------------IMPLEMENTATION-------------------------------------
    //struct Sound
    //{
    //    AudioEngine::SoundDefinition mSoundDefinition;
    //    FMOD::Sound* fmodSound = nullptr;
    //};
    
    struct Implementation
    {
        //struct Channel;
        //
        //struct AudioFader
        //{
        //    AudioFader(Implementation& impl, Channel& channel);
        //    
        //    void Update(u32 milliseconds);
        //    bool IsFinished() const {return mDurationInMilliseconds == 0;}
        //    void StartFade(float startVolume, float endVolume, u32 durationInMilliseconds);
        //    
        //    Channel& mChannel;
        //    Implementation& mImpl;
        //    s32 mDurationInMilliseconds = 0;
        //};
        //
        //struct Channel
        //{
        //    enum class State
        //    {INITIALIZE, TOPLAY, LOADING, PLAYING, STOPPING, STOPPED, VIRTUALIZING, VIRTUAL, DEVIRTUALIZE, PAUSED};
        //    
        //    Implementation& mImpl;
        //    FMOD::Channel* moptrChannel = nullptr;
        //    
        //    AudioEngine::SoundID mSoundID = AudioEngine::InvalidSoundID;
        //    AudioEngine::SoundDefinition mSoundDef;
        //    glm::vec3 mPos;
        //    float mVolume = 0.0f;
        //    //float mFadeInTimeInSeconds = 0.0f;
        //    float mPitch = 1.0f;
        //    State mState = State::INITIALIZE;
        //    AudioFader mStopFader;
        //    AudioFader mVirtualizeFader;
        //    bool mStopRequested = false;
        //    bool mPauseRequested = false;
        //    
        //    Channel(Implementation& impl, AudioEngine::SoundID soundID, const AudioEngine::SoundDefinition& soundDef, const glm::vec3& pos, float volume, float pitch);
        //    void Update();
        //    void UpdateChannelParameters();
        //    bool ShouldBeVirtual(bool allowOneShotVirtuals) const;
        //    bool IsOneShot(){return false;}
        //    bool IsPlaying() const;
        //    float GetVolume() const;
        //    void Stop(float fadeOutInSeconds = 0.0f);
        //};

        void Init(const r2::mem::utils::MemBoundary& memoryBoundary, u32 boundsChecking);
        void Update();
        void Shutdown();
     //   bool SoundIsLoaded(AudioEngine::SoundID soundID);
        static u64 MemorySize(u32 maxNumBanks, u32 maxNumEvents, u32 headerSize, u32 boundsCheckingSize);
        
      //  using SoundList = r2::SArray<Sound>*;
      //  using ChannelList = r2::SArray<Channel*>*;
      //  using DefinitionMap = r2::SHashMap<AudioEngine::SoundID>*;
        
        
       
      //  SoundList mSounds = nullptr;
      //  DefinitionMap mDefinitions = nullptr;
      //  ChannelList mChannels = nullptr;
      //  r2::mem::PoolArena* mChannelPool = nullptr;

        //new data proposal
        r2::mem::utils::MemBoundary mSystemBoundary;
        r2::mem::StackArena* mStackArena;

		using BankList = r2::SArray<FMOD::Studio::Bank*>*;
		using EventInstanceList = r2::SArray<FMOD::Studio::EventInstance*>*;
        using EventInstanceHandleList = r2::SArray<AudioEngine::EventInstanceHandle>*;

        FMOD::Studio::System* mStudioSystem = nullptr;
        FMOD::System* mSystem = nullptr;

        u64 mEventInstanceCount = 0;

        BankList mLoadedBanks = nullptr;
        EventInstanceList mLiveEventInstances = nullptr;
        EventInstanceHandleList mEventInstanceHandles = nullptr;

        r2::audio::AudioEngine::BankHandle mMasterBank = r2::audio::AudioEngine::InvalidBank;
        r2::audio::AudioEngine::BankHandle mMasterStringsBank = r2::audio::AudioEngine::InvalidBank;
    };

    u64 Implementation::MemorySize(u32 maxNumBanks, u32 maxNumEvents, u32 headerSize, u32 boundsCheckingSize)
    {
        u64 totalAllocationSizeNeeded = 0;

        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Implementation), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<FMOD::Studio::Bank*>::MemorySize(maxNumBanks), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<FMOD::Studio::EventInstance*>::MemorySize(maxNumEvents), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);
        totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AudioEngine::EventInstanceHandle>::MemorySize(maxNumEvents), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);
        //totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(SArray<Sound>::MemorySize(MAX_NUM_SOUNDS), SOUND_ALIGNMENT, headerSize, boundsCheckingSize); //for mSounds
    //    totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(SHashMap<AudioEngine::SoundID>::MemorySize(MAX_NUM_DEFINITIONS) * SHashMap<u32>::LoadFactorMultiplier(), SOUND_ALIGNMENT, headerSize, boundsCheckingSize); //for mDefinitions
       // totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation( SArray<Channel*>::MemorySize(MAX_NUM_CHANNELS), SOUND_ALIGNMENT, headerSize, boundsCheckingSize); //for mChannels
       // totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Channel), SOUND_ALIGNMENT, headerSize, boundsCheckingSize) * MAX_NUM_CHANNELS; //for mChannelPool
     //   totalAllocationSizeNeeded += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), SOUND_ALIGNMENT, headerSize, boundsCheckingSize);

        return totalAllocationSizeNeeded;
    }
    
    void Implementation::Init(const r2::mem::utils::MemBoundary& memoryBoundary, u32 boundsChecking)
    {
        u64 memorySize = MemorySize(MAX_NUM_BANKS, MAX_NUM_EVENT_INSTANCES, r2::mem::StackAllocator::HeaderSize(), boundsChecking);

        R2_CHECK(memoryBoundary.location != nullptr && memoryBoundary.size >= memorySize, "We don't have a valid memory boundary!");

        mSystemBoundary = memoryBoundary;

        mStackArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(mSystemBoundary);
        
        R2_CHECK(mStackArena != nullptr, "We couldn't emplace the stack arena?");

        mLoadedBanks = MAKE_SARRAY(*mStackArena, FMOD::Studio::Bank*, MAX_NUM_BANKS);

        R2_CHECK(mLoadedBanks != nullptr, "Couldn't create the mLoadedBanks array");

        mLiveEventInstances = MAKE_SARRAY(*mStackArena, FMOD::Studio::EventInstance*, MAX_NUM_EVENT_INSTANCES);

        R2_CHECK(mLiveEventInstances != nullptr, "Couldn't create the live instances");

        mEventInstanceHandles = MAKE_SARRAY(*mStackArena, AudioEngine::EventInstanceHandle, MAX_NUM_EVENT_INSTANCES);

        r2::sarr::Fill(*mEventInstanceHandles, AudioEngine::InvalidEventInstanceHandle);

        mEventInstanceCount = 0;

        FMOD::Studio::Bank* emptyBank = nullptr;

        r2::sarr::Fill(*mLoadedBanks, emptyBank);

      //  mSounds = MAKE_SARRAY(allocator, Sound, MAX_NUM_SOUNDS);
        
      //  R2_CHECK(mSounds != nullptr, "We should have sounds initialized!");
        
        /*for (u64 i = 0 ; i < MAX_NUM_SOUNDS; ++i)
        {
            Sound& sound = r2::sarr::At(*mSounds, i);
            r2::util::PathCpy(sound.mSoundDefinition.soundName, "");
            sound.fmodSound = nullptr;
        }
        
        mDefinitions = MAKE_SHASHMAP(allocator, AudioEngine::SoundID, MAX_NUM_DEFINITIONS);*/
        
      //  R2_CHECK(mDefinitions != nullptr, "Failed to allocate mDefinitions");
        
      //  mChannels = MAKE_SARRAY(allocator, Channel*, MAX_NUM_CHANNELS);
        
     //   R2_CHECK(mChannels != nullptr, "We should have channels initialized!");
        
      //  for (u64 i = 0 ; i < MAX_NUM_CHANNELS; ++i)
        {
      //      r2::sarr::At(*mChannels, i) = nullptr;
        }
        
     //   mChannelPool = MAKE_POOL_ARENA(allocator, sizeof(Channel), alignof(Channel), MAX_NUM_CHANNELS);

        CheckFMODResult( FMOD::Studio::System::create(&mStudioSystem) );
        CheckFMODResult(mStudioSystem->initialize(MAX_NUM_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr));
        CheckFMODResult(mStudioSystem->getCoreSystem(&mSystem));

    }
    
    void Implementation::Update()
    {
     //   TODO;

        /*for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
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
        }*/
        
        CheckFMODResult(mStudioSystem->update());
    }
    
    void Implementation::Shutdown()
    {
        CheckFMODResult(mStudioSystem->unloadAll());
        CheckFMODResult(mStudioSystem->release());
        mSystem = nullptr;

        r2::sarr::Clear(*mLiveEventInstances);
        r2::sarr::Clear(*mLoadedBanks);

        FREE(mEventInstanceHandles, *mStackArena);
        FREE(mLiveEventInstances, *mStackArena);
        FREE(mLoadedBanks, *mStackArena);

        FREE_EMPLACED_ARENA(mStackArena);

        mLiveEventInstances = nullptr;
        mLoadedBanks = nullptr;
        mStudioSystem = nullptr;
        mStackArena = nullptr;

       // CheckFMODResult(mStudioSystem->release() );
        
       // for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
       // {
       //     Channel* channel = r2::sarr::At(*mChannels, i);
       //     if (channel)
       //     {
       //         FREE(channel, *mChannelPool);
       //         r2::sarr::At(*mChannels, i) = nullptr;
       //     }
       //     
       //     
       // }

       // //Free in reverse order
       //// FREE(mFMODMemory, allocator);
       // FREE(mChannelPool, allocator);
       // FREE(mChannels, allocator);
       // FREE(mDefinitions, allocator);
       // FREE(mSounds, allocator);
       // 
       // //mFMODMemory = nullptr;
       // mChannels = nullptr;
       // mChannelPool = nullptr;
       // mDefinitions = nullptr;
       // mSounds = nullptr;
    }
    
    //--------------------------------AUDIO FADER IMPL---------------------------------------
   /* Implementation::AudioFader::AudioFader(Implementation& impl, Channel& channel)
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

		CheckFMODResult( mChannel.moptrChannel->addFadePoint(dspClock + (rate * static_cast<u64>(r2::util::MillisecondsToSeconds(durationInMilliseconds))), endVolume));

	}*/
    
    //----------------------------CHANNEL IMPLEMENTATION-------------------------------------
    //Implementation::Channel::Channel(Implementation& impl, AudioEngine::SoundID soundID, const AudioEngine::SoundDefinition& soundDef, const glm::vec3& pos, float volume, float pitch)
    //    : mImpl(impl)
    //    , mSoundID(soundID)
    //    , mSoundDef(soundDef)
    //    , mPos(pos)
    //    , mVolume(volume)
    //    , mPitch(pitch)
    //    , mStopFader(impl, *this)
    //    , mVirtualizeFader(impl, *this)
    //{
    //    
    //}
    //
    //void Implementation::Channel::Update()
    //{
    //    switch (mState)
    //    {
    //        case Implementation::Channel::State::INITIALIZE:
    //        case Implementation::Channel::State::DEVIRTUALIZE:
    //        case Implementation::Channel::State::TOPLAY:
    //        {
    //            if (mStopRequested)
    //            {
    //                mState = State::STOPPING;
    //                return;
    //            }
    //            
    //            if (ShouldBeVirtual(true))
    //            {
    //                if (IsOneShot())
    //                {
    //                    mState = State::STOPPING;
    //                }
    //                else
    //                {
    //                    mState = State::VIRTUAL;
    //                }
    //                return;
    //            }
    //            
    //            R2_CHECK(mSoundID != AudioEngine::InvalidSoundID, "mSoundID is invalid!");
    //            
    //            if (!mImpl.SoundIsLoaded(mSoundID))
    //            {
    //                AudioEngine engine;
    //                engine.LoadSound(mSoundID);
    //                mState = State::LOADING;
    //                return;
    //            }
    //            
    //            moptrChannel = nullptr;
    //            
    //            Sound sound = r2::sarr::At(*mImpl.mSounds, mSoundID);
    //            if (sound.fmodSound)
    //            {
    //                CheckFMODResult(mImpl.mSystem->playSound(sound.fmodSound, nullptr, true, &moptrChannel));
    //            }
    //            
    //            if (moptrChannel)
    //            {
    //                if (mState == State::DEVIRTUALIZE)
    //                {
    //                    mVirtualizeFader.StartFade(SILENCE, GetVolume(), VIRTUALIZE_FADE_TIME);
    //                }
    //                
    //                mState = State::PLAYING;
    //                UpdateChannelParameters();
    //                CheckFMODResult( moptrChannel->setPaused(false) );
    //            }
    //            else
    //            {
    //                mState = State::STOPPING;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::LOADING:
    //        {
    //            if (mImpl.SoundIsLoaded(mSoundID))
    //            {
    //                mState = State::TOPLAY;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::PLAYING:
    //        {
    //            mVirtualizeFader.Update(CPLAT.TickRate());
				//UpdateChannelParameters();
    //            
    //            if (!IsPlaying())
    //            {
    //                mState = State::STOPPED;
    //                return;
    //            }
				//
    //            if (mStopRequested)
    //            {
    //                mState = State::STOPPING;
    //                return;
    //            }

    //            if (mPauseRequested)
    //            {
    //                mPauseRequested = false;
    //                mState = State::PAUSED;
    //                CheckFMODResult( moptrChannel->setPaused(true) );
    //                return;
    //            }
    //            
    //            if (ShouldBeVirtual(false))
    //            {
    //                mVirtualizeFader.StartFade(SILENCE, GetVolume(), VIRTUALIZE_FADE_TIME);
    //                mState = State::VIRTUALIZING;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::STOPPING:
    //        {
    //            mStopFader.Update(CPLAT.TickRate());
    //            UpdateChannelParameters();
    //            
    //            if (mStopFader.IsFinished())
    //            {
    //                moptrChannel->stop();
    //                mState = State::STOPPED;
    //                return;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::STOPPED:
    //        break;
    //        case Implementation::Channel::State::VIRTUALIZING:
    //        {
    //            mVirtualizeFader.Update(CPLAT.TickRate());
    //            UpdateChannelParameters();
    //            if (!ShouldBeVirtual(false))
    //            {
    //                mVirtualizeFader.StartFade(GetVolume(), SILENCE, VIRTUALIZE_FADE_TIME);
    //                mState = State::PLAYING;
    //                break;
    //            }
    //            if (mVirtualizeFader.IsFinished())
    //            {
    //                CheckFMODResult( moptrChannel->stop() );
    //                mState = State::VIRTUAL;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::VIRTUAL:
    //        {
    //            if (mStopRequested)
    //            {
    //                mState = State::STOPPING;
    //            }
    //            else if(!ShouldBeVirtual(false))
    //            {
    //                mState = State::DEVIRTUALIZE;
    //            }
    //        }
    //        break;
    //        case Implementation::Channel::State::PAUSED:
    //        {
    //            if (mPauseRequested)
    //            {
    //                moptrChannel->setPaused(false);
    //                mPauseRequested = false;
    //                mState = State::PLAYING;
    //            }
    //        }
    //        break;
    //        default:
    //            break;
    //    }
    //}
    //
    //void Implementation::Channel::UpdateChannelParameters()
    //{
    //    if (!moptrChannel)
    //    {
    //        return;
    //    }
    //    
    //    FMOD_VECTOR position = GLMToFMODVector(mPos);
    //    
    //    if (mSoundDef.flags.IsSet(r2::audio::AudioEngine::IS_3D))
    //    {
    //        CheckFMODResult( moptrChannel->set3DAttributes(&position, nullptr) );
    //    }
    //    
    //    CheckFMODResult( moptrChannel->setVolume(mVolume) );
    //    
    //    CheckFMODResult( moptrChannel->setPitch(mPitch) );
    //}
    //
    //bool Implementation::Channel::ShouldBeVirtual(bool allowOneShotVirtuals) const
    //{
    //    //?
    //    return false;
    //}
    //
    //bool Implementation::Channel::IsPlaying() const
    //{
    //    if (!moptrChannel)
    //    {
    //        return false;
    //    }
    //    
    //    bool isPlaying = false;
    //    CheckFMODResult(moptrChannel->isPlaying(&isPlaying));
    //    return isPlaying;
    //}
    //
    //float Implementation::Channel::GetVolume() const
    //{
    //    if (!moptrChannel)
    //    {
    //        return 0.0f;
    //    }
    //    
    //    return mVolume;
    //}

    //void Implementation::Channel::Stop(float fadeOutInSeconds)
    //{
    //    if (!moptrChannel)
    //    {
    //        return;
    //    }
    //    
    //    if (fadeOutInSeconds <= 0.0f)
    //    {
    //        CheckFMODResult( moptrChannel->stop() );
    //        mState = Implementation::Channel::State::STOPPED;
    //    }
    //    else
    //    {
    //        mStopRequested = true;
    //        mStopFader.StartFade(GetVolume(), SILENCE, r2::util::SecondsToMilliseconds(fadeOutInSeconds));
    //    }
    //}
    
    //--------------------------AUDIO ENGINE-----------------------------------
    
    static Implementation* gImpl = nullptr;
    static bool gAudioEngineInitialize = false;
    
    void AudioEngine::Init()
    {
        if (gAudioEngineInitialize)
        {
            return;
        }
        
        R2_CHECK(gImpl == nullptr, "Should always be the case");
        //do memory setup here
        {
            r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
            
            u32 boundsChecking = 0;
#if defined(R2_DEBUG)|| defined(R2_RELEASE)
            boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
            u64 memorySizeForImplementation = Implementation::MemorySize(MAX_NUM_BANKS, MAX_NUM_EVENT_INSTANCES, r2::mem::StackAllocator::HeaderSize(), boundsChecking);
            
            
            //u64 soundAreaSize =
            //sizeof(r2::mem::LinearArena) +
            //Implementation::MemorySize(r2::mem::LinearAllocator::HeaderSize(), boundsChecking);
            
         //   soundAreaSize = r2::mem::utils::GetMaxMemoryForAllocation(soundAreaSize, SOUND_ALIGNMENT);
          //  R2_LOGI("sound area size: %llu\n", soundAreaSize);
            
            r2::mem::MemoryArea* memArea = r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle);
            AudioEngine::mSoundMemoryAreaHandle = memArea->AddSubArea(memorySizeForImplementation);
            R2_CHECK(AudioEngine::mSoundMemoryAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "We have an invalid sound memory area!");
            
            r2::mem::MemoryArea::SubArea* soundMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle)->GetSubArea(AudioEngine::mSoundMemoryAreaHandle);
            
            gImpl = ALLOC(Implementation, *MEM_ENG_PERMANENT_PTR);

			R2_CHECK(gImpl != nullptr, "Something went wrong trying to allocate the implementation for sound!");

            gImpl->Init(soundMemoryArea->mBoundary, boundsChecking);
       //     AudioEngine::mSoundAllocator = EMPLACE_LINEAR_ARENA(*r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle)->GetSubArea(AudioEngine::mSoundMemoryAreaHandle));
            
       //     R2_CHECK(AudioEngine::mSoundAllocator != nullptr, "We have an invalid linear arena for sound!");
        }

       // if (gImpl == nullptr)
        {

         //   gImpl = ALLOC(Implementation, *AudioEngine::mSoundAllocator);
            
         //   R2_CHECK(gImpl != nullptr, "Something went wrong trying to allocate the implementation for sound!");
            
         //   gImpl->Init(*AudioEngine::mSoundAllocator);
        }
        
        gAudioEngineInitialize = true;

        ReloadSoundDefinitions();
    }
    
    void AudioEngine::Update()
    {
#ifdef R2_ASSET_PIPELINE
        std::vector<std::string> paths;
        
        if(gAudioEngineInitialize && s_soundDefsBuiltQueue.TryPop(paths))
        {
            //TODO(Serge): could be smarter and only unload the sounds that were actually modified
            AudioEngine audio;

          //  audio.StopAllChannels();

     //       for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
            {
             //   audio.UnloadSound(i);
            }
            
            //for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
            //{
            //    r2::audio::Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
            //    if (channel)
            //    {
            //        FREE(channel, *gImpl->mChannelPool);
            //        r2::sarr::At(*gImpl->mChannels, i) = nullptr;
            //    }
            //}

            ReloadSoundDefinitions();
        }
#endif
        gImpl->Update();
    }
    
    void AudioEngine::Shutdown()
    {
        AudioEngine audio;
      //  audio.StopAllChannels();
        audio.StopAllEvents(false);
        audio.UnloadAllBanks();

        gImpl->Shutdown();

        FREE(gImpl, *MEM_ENG_PERMANENT_PTR);
        gImpl = nullptr;
        gAudioEngineInitialize = false;
       // for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
       // {
      //      audio.UnloadSound(i);
       // }
        
        //gImpl->Shutdown(*AudioEngine::mSoundAllocator);
        //
        //FREE(gImpl, *AudioEngine::mSoundAllocator);
        //gImpl = nullptr;
        //gAudioEngineInitialize = false;
        //
        //FREE_EMPLACED_ARENA(AudioEngine::mSoundAllocator);
    }
    
#ifdef R2_ASSET_PIPELINE
    void AudioEngine::PushNewlyBuiltSoundDefinitions(std::vector<std::string> paths)
    {
        s_soundDefsBuiltQueue.Push(paths);
    }
#endif
    
    void AudioEngine::ReloadSoundDefinitions()
    {
        if (gAudioEngineInitialize)
        {
            StopAllEvents(false);

            ReleaseAllEventInstances();
            
            UnloadAllBanks();

            r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

            r2::SArray<const byte*>* manifestDataArray = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const byte*, 1);

            r2::asset::lib::GetManifestDataForType(assetLib, asset::SOUND_DEFINTION, manifestDataArray);
    
            const auto* soundDefinitions = flat::GetSoundDefinitions(r2::sarr::At(*manifestDataArray, 0));
                
            const auto size = soundDefinitions->banks()->size();
            
			char directoryPath[r2::fs::FILE_PATH_LENGTH];

			r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SOUNDS, soundDefinitions->masterBank()->path()->c_str(), directoryPath); //@NOTE(Serge): maybe should be SOUND_DEFINITIONS?

            gImpl->mMasterBank = LoadBank(directoryPath, FMOD_STUDIO_LOAD_BANK_NORMAL);

			r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SOUNDS, soundDefinitions->masterBankStrings()->path()->c_str(), directoryPath); //@NOTE(Serge): maybe should be SOUND_DEFINITIONS?

            gImpl->mMasterStringsBank = LoadBank(directoryPath, FMOD_STUDIO_LOAD_BANK_NORMAL);
            
            /*FMOD::Studio::Bank* stringsBank = r2::sarr::At(*gImpl->mLoadedBanks, gImpl->mMasterStringsBank);

            R2_CHECK(stringsBank != nullptr, "?");

            int stringsCount = 0;
            CheckFMODResult(stringsBank->getStringCount(&stringsCount));

            for (int i = 0; i < stringsCount; ++i)
            {
                FMOD_GUID guid;
                char path[fs::FILE_PATH_LENGTH];
                int retrieved;
                CheckFMODResult(stringsBank->getStringInfo(i, &guid, path, fs::FILE_PATH_LENGTH, &retrieved));

            }*/
            //@TODO(Serge): now list all of the strings from the master strings bank
            
            //for (u32 i = 0; i < size; ++i)
            //{
            //    const r2::SoundDefinition* def = soundDefinitions->definitions()->Get(i);
            //        
            //    SoundDefinition soundDef;
            //    r2::util::PathCpy(soundDef.soundName, def->soundName()->c_str());
            //    soundDef.minDistance = def->minDistance();
            //    soundDef.maxDistance = def->maxDistance();
            //    soundDef.defaultVolume = def->defaultVolume();
            //    soundDef.defaultPitch = def->pitch();
            //    soundDef.loadOnRegister = def->loadOnRegister();
            //        
            //    if (def->loop())
            //    {
            //        soundDef.flags |= LOOP;
            //    }
            //        
            //    if (def->is3D())
            //    {
            //        soundDef.flags |= IS_3D;
            //    }
            //        
            //    if (def->stream())
            //    {
            //        soundDef.flags |= STREAM;
            //    }
            //    AudioEngine audio;
            //    soundDef.soundKey = r2::asset::GetAssetNameForFilePath(soundDef.soundName, r2::asset::SOUND);
            // //   SoundID soundID = audio.RegisterSound(soundDef);    
            //}
                
            FREE(manifestDataArray, *MEM_ENG_SCRATCH_PTR);
        }
    }
    
  //  AudioEngine::SoundID AudioEngine::RegisterSound(const SoundDefinition& soundDef)
  //  {
  //      //check to see if this was already registered
  //      SoundID soundID = InvalidSoundID;
  //      
  //      for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
  //      {
  //          const Sound& sound = r2::sarr::At(*gImpl->mSounds, i);
  //          
  //          if (strcmp(sound.mSoundDefinition.soundName, soundDef.soundName) == 0)
  //          {
  //              //return the index that we already had for this sound definition
  //              soundID = i;
  //              break;
  //          }
  //      }
  //      
  //      if (soundID == InvalidSoundID)
  //      {
  //          soundID = NextAvailableSoundID();
  //      }
  //      
  //      if (soundID == InvalidSoundID)
  //      {
  //          R2_CHECK(false, "AudioEngine::RegisterSound - We couldn't find a new spot for the sound definition! We must be out of space!");
  //          return soundID;
  //      }
  //      
  //      //copy over the new sound definition
  //      Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
  //      sound.mSoundDefinition.flags = soundDef.flags;
  //      sound.mSoundDefinition.maxDistance = soundDef.maxDistance;
  //      sound.mSoundDefinition.minDistance = soundDef.minDistance;
  //      sound.mSoundDefinition.defaultVolume = soundDef.defaultVolume;
  //      sound.mSoundDefinition.defaultPitch = soundDef.defaultPitch;
  //      r2::util::PathCpy( sound.mSoundDefinition.soundName, soundDef.soundName );
  //      
  //      r2::shashmap::Set(*gImpl->mDefinitions, soundDef.soundKey, soundID);

  //      if (soundDef.loadOnRegister)
  //      {
  //          bool result = LoadSound(soundID);
  //          R2_CHECK(result, "AudioEngine::RegisterSound - we couldn't load the sound: %s", soundDef.soundName);
  //      }
  //  
  //      return soundID;
  //  }

  //  void AudioEngine::UnregisterSound(const char* soundAssetName)
  //  {
		//SoundID defaultID = AudioEngine::InvalidSoundID;
		//SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundAssetName, r2::asset::SOUND), defaultID);

		//if (theSoundID == defaultID)
		//{
		//	R2_CHECK(false, "AudioEngine::UnregisterSound - Couldn't find sound: %s", soundAssetName);
		//	return;
		//}

  //      UnregisterSound(theSoundID);
  //  }
  //  
  //  void AudioEngine::UnregisterSound(SoundID soundID)
  //  {
  //      if (soundID == InvalidSoundID)
  //      {
  //          R2_CHECK(false, "AudioEngine::UnregisterSound - the soundID passed in is Invalid!");
  //          return;
  //      }
  //      
  //      Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
  //      
  //      if (strcmp(sound.mSoundDefinition.soundName, "") == 0)
  //      {
  //          R2_CHECK(false, "AudioEngine::UnregisterSound - we've already unregistered this sound: %lli!", soundID);
  //          return;
  //      }
  //      
  //      if (sound.fmodSound)
  //      {
  //          UnloadSound(soundID);
  //      }
  //      
  //      r2::util::PathCpy(sound.mSoundDefinition.soundName, "");
  //      sound.mSoundDefinition.defaultVolume = 0.0f;
  //      sound.mSoundDefinition.defaultPitch = 0.0f;
  //      sound.mSoundDefinition.flags.Clear();
  //      sound.mSoundDefinition.minDistance = 0.0f;
  //      sound.mSoundDefinition.maxDistance = 0.0f;
  //      sound.mSoundDefinition.flags = NONE;
  //      sound.mSoundDefinition.loadOnRegister = false;

  //      r2::shashmap::Remove(*gImpl->mDefinitions, soundID);
  //  }
  //  
  //  bool AudioEngine::LoadSound(const char* soundName)
  //  {
  //      SoundID defaultID = AudioEngine::InvalidSoundID;
  //      SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundName, r2::asset::SOUND), defaultID);
  //      
  //      if (theSoundID == defaultID)
  //      {
  //          R2_CHECK(false, "AudioEngine::LoadSound - Couldn't find sound: %s", soundName);
  //          return false;
  //      }
  //      
  //      return LoadSound(theSoundID);
  //  }
  //  
  //  bool AudioEngine::LoadSound(SoundID soundID)
  //  {
  //      if (gImpl->SoundIsLoaded(soundID))
  //      {
  //          return true;
  //      }
  //      
  //      Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
  //      if (strcmp( sound.mSoundDefinition.soundName, "") == 0)
  //      {
  //          R2_CHECK(false, "AudioEngine::LoadSound - there's no definition for the sound!");
  //          return false;
  //      }
  //      
  //      FMOD_MODE mode = FMOD_DEFAULT;//FMOD_NONBLOCKING;
  //      mode |= sound.mSoundDefinition.flags.IsSet(IS_3D) ? (FMOD_3D | FMOD_3D_INVERSETAPEREDROLLOFF) : FMOD_2D;
  //      mode |= sound.mSoundDefinition.flags.IsSet(LOOP) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
  //      mode |= sound.mSoundDefinition.flags.IsSet(STREAM) ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
  //      
  //      CheckFMODResult( gImpl->mSystem->createSound(sound.mSoundDefinition.soundName, mode, nullptr, &sound.fmodSound) );
  //      
  //      if (sound.fmodSound)
  //      {
  //          if (sound.mSoundDefinition.flags.IsSet(IS_3D))
  //          {
  //              CheckFMODResult( sound.fmodSound->set3DMinMaxDistance(sound.mSoundDefinition.minDistance, sound.mSoundDefinition.maxDistance) );
  //          }
  //          return true;
  //      }
  //      
  //      R2_CHECK(false, "AudioEngine::LoadSound - Failed to create sound: %s", sound.mSoundDefinition.soundName);
  //      
  //      return false;
  //  }
  //  
  //  void AudioEngine::UnloadSound(SoundID soundID)
  //  {
  //      if (soundID == InvalidSoundID)
  //      {
  //          R2_CHECK(false, "AudioEngine::UnloadSound - you passed in an invalid soundID!");
  //          return;
  //      }
  //      
  //      Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
  //      
  //      if (!sound.fmodSound)
  //      {
  //          return;
  //      }
  //      


  //      CheckFMODResult(sound.fmodSound->release());
  //      sound.fmodSound = nullptr;
  //      
  //  }

  //  void AudioEngine::UnloadSound(const char* soundName)
  //  {
		//SoundID defaultID = AudioEngine::InvalidSoundID;
		//SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundName, r2::asset::SOUND), defaultID);

		//if (theSoundID == defaultID)
		//{
		//	R2_CHECK(false, "AudioEngine::UnLoadSound - Couldn't find sound: %s", soundName);
		//	return;
		//}

  //      UnloadSound(theSoundID);
  //  }
  //  
  //  bool AudioEngine::IsSoundLoaded(const char* soundName)
  //  {
		//SoundID defaultID = AudioEngine::InvalidSoundID;
		//SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundName, r2::asset::SOUND), defaultID);

  //      if (theSoundID == defaultID)
  //      {
  //          return false;
  //      }

  //      return gImpl->SoundIsLoaded(theSoundID);
  //  }

  //  bool AudioEngine::IsSoundRegistered(const char* soundName)
  //  {
		//SoundID defaultID = AudioEngine::InvalidSoundID;
		//SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundName, r2::asset::SOUND), defaultID);
  //      return theSoundID != defaultID;
  //  }

  //  void AudioEngine::UnloadSounds(const r2::SArray<SoundID>& soundsToUnload, bool unregister)
  //  {
  //      const auto numSoundsToUnload = r2::sarr::Size(soundsToUnload);

  //      for (u32 i = 0; i < numSoundsToUnload; ++i)
  //      {
  //          UnloadSound(r2::sarr::At(soundsToUnload, i));
  //      }

  //      if (unregister)
  //      {
  //          for (u32 i = 0; i < numSoundsToUnload; ++i)
  //          {
  //              UnregisterSound(r2::sarr::At(soundsToUnload, i));
  //          }
  //      }
  //  }

  //  void AudioEngine::Set3DListenerAndOrientation(const glm::vec3& position, const glm::vec3& look, const glm::vec3& up)
  //  {
  //      FMOD_VECTOR pos = GLMToFMODVector(position);
  //      FMOD_VECTOR forward = GLMToFMODVector(look);
  //      FMOD_VECTOR upVec = GLMToFMODVector(up);
  //      
  //      CheckFMODResult( gImpl->mSystem->set3DListenerAttributes(0, &pos, nullptr, &forward, &upVec) );
  //  }
  //  
  //  AudioEngine::ChannelID AudioEngine::PlaySound(const char* soundName, const glm::vec3& pos, float volume, float pitch)
  //  {
  //      SoundID defaultID = AudioEngine::InvalidSoundID;
  //      SoundID theSoundID = r2::shashmap::Get(*gImpl->mDefinitions, r2::asset::GetAssetNameForFilePath(soundName, r2::asset::SOUND), defaultID);
  //      
  //      if (theSoundID == defaultID)
  //      {
  //          R2_CHECK(false, "AudioEngine::PlaySound - Couldn't find sound: %s", soundName);
  //          return InvalidChannelID;
  //      }
  //      
  //      return PlaySound(theSoundID, pos, volume, pitch);
  //  }
  //  
  //  AudioEngine::ChannelID AudioEngine::PlaySound(AudioEngine::SoundID soundID, const glm::vec3& pos, float volume, float pitch)
  //  {
  //      if (soundID == InvalidSoundID)
  //      {
  //          R2_CHECK(false, "AudioEngine::PlaySound - passed in invalid SoundID");
  //          return InvalidChannelID;
  //      }
  //      
  //      if (!gImpl->SoundIsLoaded(soundID))
  //      {
  //          R2_CHECK(false, "AudioEngine::PlaySound - tried to play a sound that isn't loaded");
  //          return InvalidChannelID;
  //      }

  //      const Sound& sound = r2::sarr::At(*gImpl->mSounds, soundID);
  //      if (strcmp( sound.mSoundDefinition.soundName, "") == 0)
  //      {
  //          R2_CHECK(false, "AudioEngine::PlaySound - we don't have a proper definition for soundID: %lli", soundID);
  //          return InvalidChannelID;
  //      }
  //      
  //      ChannelID nextChannelID = NextAvailableChannelID();
  //      
  //      if (nextChannelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::PlaySound - we don't have anymore channel slots available to play: %s", sound.mSoundDefinition.soundName);
  //          return InvalidChannelID;
  //      }
  //      
  //      float volumeToPlay = volume > 0.0f ? volume : sound.mSoundDefinition.defaultVolume;
  //      float pitchToPlay = pitch > 0.0f ? pitch : sound.mSoundDefinition.defaultPitch;
  //      
  //      r2::sarr::At( *gImpl->mChannels, nextChannelID ) = ALLOC_PARAMS(Implementation::Channel, *gImpl->mChannelPool, *gImpl, soundID, sound.mSoundDefinition, pos, volumeToPlay, pitchToPlay);
  //      
  //      return nextChannelID;
  //  }
  //  
  //  void AudioEngine::StopChannel(AudioEngine::ChannelID channelID, float fadeoutInSeconds)
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::StopChannel - channelID is Invalid!");
  //          return;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::StopChannel - channel is null?");
  //          return;
  //      }
  //      
  //      channel->Stop(fadeoutInSeconds);
  //  }
  //  
  //  void AudioEngine::PauseChannel(AudioEngine::ChannelID channelID)
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::PauseChannel - channelID is Invalid!");
  //          return;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::PauseChannel - channel is null?");
  //          return;
  //      }
  //      
  //      channel->mPauseRequested = true;
  //  }
  //  
  //  void AudioEngine::PauseAll()
  //  {
  //      for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
  //      {
  //          Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
  //          
  //          if (channel)
  //          {
  //              PauseChannel(i);
  //          }
  //      }
  //  }
  //  
  //  void AudioEngine::StopAllChannels(float fadeOutInSeconds)
  //  {
  //      for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
  //      {
  //          Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
  //          
  //          if (channel)
  //          {
  //              StopChannel(i, fadeOutInSeconds);
  //          }
  //      }
  //  }
  //  
  //  void AudioEngine::SetChannel3DPosition(AudioEngine::ChannelID channelID, const glm::vec3& position)
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannel3DPosition - channelID is Invalid!");
  //          return;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannel3DPosition - channel is null?");
  //          return;
  //      }
  //      
  //      channel->mPos = position;
  //  }
  //  
  //  void AudioEngine::SetChannelVolume(AudioEngine::ChannelID channelID, float volume)
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannelVolume - channelID is Invalid!");
  //          return;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannelVolume - channel is null?");
  //          return;
  //      }
  //      
  //      channel->mVolume = volume;
  //  }
  //  
  //  bool AudioEngine::IsChannelPlaying(AudioEngine::ChannelID channelID) const
  //  {
  //      
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::IsChannelPlaying - channelID is Invalid!");
  //          return false;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::IsChannelPlaying - channel is null?");
  //          return false;
  //      }
  //      
  //      return channel->IsPlaying();
  //  }
  //  
  //  float AudioEngine::GetChannelPitch(AudioEngine::ChannelID channelID) const
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::GetChannelPitch - channelID is Invalid!");
  //          return false;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::GetChannelPitch - channel is null?");
  //          return false;
  //      }
  //      
  //      return channel->mPitch;
  //  }
  //  
  //  void AudioEngine::SetChannelPitch(AudioEngine::ChannelID channelID, float pitch)
  //  {
  //      if (channelID == InvalidChannelID)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannelPitch - channelID is Invalid!");
  //          return;
  //      }
  //      
  //      Implementation::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
  //      if (channel == nullptr)
  //      {
  //          R2_CHECK(false, "AudioEngine::SetChannelPitch - channel is null?");
  //          return;
  //      }
  //      
  //      channel->mPitch = pitch;
  //  }
 
  //  AudioEngine::SoundID AudioEngine::NextAvailableSoundID()
  //  {
  //      for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
  //      {
  //          const Sound& sound = r2::sarr::At(*gImpl->mSounds, i);
  //          if (strcmp(sound.mSoundDefinition.soundName, "") == 0)
  //          {
  //              return i;
  //          }
  //      }
  //      
  //      return AudioEngine::InvalidSoundID;
  //  }
    
    int AudioEngine::GetSampleRate() const
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");
        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return sampleRate;
    }
    
    AudioEngine::SpeakerMode AudioEngine::GetSpeakerMode() const
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return static_cast<SpeakerMode>(speakerMode);
    }
    
    s32 AudioEngine::GetNumberOfDrivers() const
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        s32 numDrivers = 0;
        CheckFMODResult(gImpl->mSystem->getNumDrivers(&numDrivers));
        return numDrivers;
    }
    
    u32 AudioEngine::GetCurrentDriver() const
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        int driverId;
        CheckFMODResult(gImpl->mSystem->getDriver(&driverId));
        return driverId;
    }
    
    void AudioEngine::SetDriver(int driverId)
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        s32 numDrivers = GetNumberOfDrivers();
        
        R2_CHECK(driverId >= 0 && driverId < numDrivers, "Passed in a driver id that isn't in range");
        
        CheckFMODResult(gImpl->mSystem->setDriver(driverId));
    }
    
    void AudioEngine::GetDriverInfo(int driverId, char* driverName, u32 driverNameLength, u32& systemRate, SpeakerMode& mode, u32& speakerModeChannels)
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        s32 numDrivers = GetNumberOfDrivers();
        
        R2_CHECK(driverId >= 0 && driverId < numDrivers, "Passed in a driver id that isn't in range");
        int rate, channels;
        FMOD_SPEAKERMODE fmodSpeakerMode;
        CheckFMODResult(gImpl->mSystem->getDriverInfo(driverId, driverName, driverNameLength, nullptr, &rate, &fmodSpeakerMode, &channels));
        
        systemRate = rate;
        mode = static_cast<SpeakerMode>(fmodSpeakerMode);
        speakerModeChannels = channels;
    }
    
   /* AudioEngine::ChannelID AudioEngine::NextAvailableChannelID()
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
    }*/

    /*
     * New Interface proposal
     */

    //Banks
	const r2::audio::AudioEngine::EventInstanceHandle r2::audio::AudioEngine::InvalidEventInstanceHandle = {};

    AudioEngine::BankHandle FindNextAvailableBankHandle()
    {
        const u32 banksCapacity = r2::sarr::Capacity(*gImpl->mLoadedBanks);

        for (u32 i = 0; i < banksCapacity; ++i)
        {
            if (r2::sarr::At(*gImpl->mLoadedBanks, i) == nullptr)
            {
                return static_cast<AudioEngine::BankHandle>( i );
            }
        }

        return AudioEngine::InvalidBank;
    }


    AudioEngine::BankHandle AudioEngine::LoadBank(const char* path, u32 flags)
    {
        if (!gImpl)
        {
            R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
            return InvalidBank;
        }

        const u32 numLoadedBanks = r2::sarr::Size(*gImpl->mLoadedBanks);

        for (u32 i = 0; i < numLoadedBanks; ++i)
        {
            const FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, i);

            char nextBankPath[r2::fs::FILE_PATH_LENGTH];
            int retrieved;
            if (bank && bank->getPath(nextBankPath, r2::fs::FILE_PATH_LENGTH, &retrieved) == FMOD_OK)
            {
                if (retrieved && strcmp(nextBankPath, path) == 0)
                {
                    return static_cast<BankHandle>(i);
                }
            }
        }

        BankHandle result = FindNextAvailableBankHandle();

        R2_CHECK(result != InvalidBank, "We couldn't find a bank handle for this bank");

        FMOD::Studio::Bank* loadedBank;

        CheckFMODResult(gImpl->mStudioSystem->loadBankFile(path, flags, &loadedBank));

        R2_CHECK(loadedBank->isValid(), "?");

        r2::sarr::At(*gImpl->mLoadedBanks, result) = loadedBank;

        return result;
    }

    //AudioEngine::BankHandle AudioEngine::GetBankHandle(const char* bankAssetName)
    //{
    //    return InvalidBank;
    //}

    bool AudioEngine::UnloadSoundBank(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

        FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);
        
        if (!bank)
        {
            R2_CHECK(false, "Bank file was nullptr meaning you might have already unloaded this?");
            return false;
        }
        
        FMOD_STUDIO_LOADING_STATE loadingState;
        CheckFMODResult(bank->getSampleLoadingState(&loadingState));

        if (loadingState == FMOD_STUDIO_LOADING_STATE_LOADED ||
            loadingState == FMOD_STUDIO_LOADING_STATE_LOADING)
        {
            CheckFMODResult(bank->unloadSampleData());
        }
        
        CheckFMODResult(bank->getLoadingState(&loadingState));

        if (loadingState == FMOD_STUDIO_LOADING_STATE_LOADED)
        {
            CheckFMODResult(bank->unload());
        }
        
        r2::sarr::At(*gImpl->mLoadedBanks, bankHandle) = nullptr;

        return true;
    }
    
    bool AudioEngine::IsBankValid(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

        if (bankHandle == InvalidBank )
        {
            return false;
        }

        if (bankHandle < 0 || bankHandle >= r2::sarr::Capacity(*gImpl->mLoadedBanks))
        {
            R2_CHECK(false, "Somehow we're outside the valid range of the bank handles?");
            return false;
        }

        FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);

        if (!bank)
        {
            return false;
        }

        return bank->isValid();
    }

    bool AudioEngine::HasBankFinishedLoading(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		if (bankHandle == InvalidBank)
		{
			return false;
		}

		if (bankHandle < 0 || bankHandle >= r2::sarr::Capacity(*gImpl->mLoadedBanks))
		{
			R2_CHECK(false, "Somehow we're outside the valid range of the bank handles?");
			return false;
		}

		FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);

		if (!bank)
		{
            R2_CHECK(false, "The bank is nullptr?");
            return false;
		}

        FMOD_STUDIO_LOADING_STATE loadingState;
        CheckFMODResult(bank->getLoadingState(&loadingState));

        return loadingState == FMOD_STUDIO_LOADING_STATE_LOADED;
    }

    void AudioEngine::UnloadAllBanks()
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        StopAllEvents(false);

		const u32 banksCapacity = r2::sarr::Capacity(*gImpl->mLoadedBanks);

		for (u32 i = 0; i < banksCapacity; ++i)
		{
            FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, i);

			if (bank != nullptr)
			{
                FMOD_STUDIO_LOADING_STATE loadingState;
                CheckFMODResult(bank->getSampleLoadingState(&loadingState));

                if (loadingState == FMOD_STUDIO_LOADING_STATE_LOADED)
                {
                    CheckFMODResult(bank->unloadSampleData());
                }

                bank->getLoadingState(&loadingState);
                if (loadingState == FMOD_STUDIO_LOADING_STATE_LOADED)
                {
                    CheckFMODResult(bank->unload());
                }
			}
		}

        r2::sarr::Clear(*gImpl->mLoadedBanks);
        FMOD::Studio::Bank* emptyBank = nullptr;
        r2::sarr::Fill(*gImpl->mLoadedBanks, emptyBank);
    }

    bool AudioEngine::LoadSampleDataForBank(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		if (bankHandle == InvalidBank)
		{
			return false;
		}

		if (bankHandle < 0 || bankHandle >= r2::sarr::Capacity(*gImpl->mLoadedBanks))
		{
			R2_CHECK(false, "Somehow we're outside the valid range of the bank handles?");
			return false;
		}

        FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);

		if (!bank)
		{
			R2_CHECK(false, "The bank is nullptr?");
			return false;
		}

        CheckFMODResult(bank->loadSampleData());

        return true;
    }
    
    bool AudioEngine::UnloadSampleDataForBank(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		if (bankHandle == InvalidBank)
		{
			return false;
		}

		if (bankHandle < 0 || bankHandle >= r2::sarr::Capacity(*gImpl->mLoadedBanks))
		{
			R2_CHECK(false, "Somehow we're outside the valid range of the bank handles?");
			return false;
		}

		FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);

		if (!bank)
		{
			R2_CHECK(false, "The bank is nullptr?");
			return false;
		}

        FMOD_STUDIO_LOADING_STATE loadingState;
        
        CheckFMODResult(bank->getSampleLoadingState(&loadingState));

        if (loadingState == FMOD_STUDIO_LOADING_STATE_LOADED)
        {
            CheckFMODResult(bank->unloadSampleData());
        }
        
        return true;
    }

    bool AudioEngine::HasSampleDataFinishedLoadingForBank(BankHandle bankHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		if (bankHandle == InvalidBank)
		{
			return false;
		}

		if (bankHandle < 0 || bankHandle >= r2::sarr::Capacity(*gImpl->mLoadedBanks))
		{
			R2_CHECK(false, "Somehow we're outside the valid range of the bank handles?");
			return false;
		}

		FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, bankHandle);

		if (!bank)
		{
			R2_CHECK(false, "The bank is nullptr?");
			return false;
		}

        FMOD_STUDIO_LOADING_STATE sampleLoadingState;
        CheckFMODResult(bank->getSampleLoadingState(&sampleLoadingState));

        return sampleLoadingState == FMOD_STUDIO_LOADING_STATE_LOADED;
    }

    //Listeners - there's always 1 I think?
    void AudioEngine::SetNumListeners(u32 numListeners)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        CheckFMODResult(gImpl->mStudioSystem->setNumListeners(numListeners));
    }

    void AudioEngine::SetListener3DAttributes(u32 listenerIndex, const Attributes3D& attributes3D)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        FMOD_3D_ATTRIBUTES fmodAttributes;
        fmodAttributes.position = GLMToFMODVector(attributes3D.position);
        fmodAttributes.up = GLMToFMODVector(attributes3D.up);
        fmodAttributes.forward = GLMToFMODVector(attributes3D.look);
        fmodAttributes.velocity = GLMToFMODVector(attributes3D.velocity);

        CheckFMODResult(gImpl->mStudioSystem->setListenerAttributes(static_cast<int>(listenerIndex), &fmodAttributes));
    }

    u32 AudioEngine::GetNumListeners()
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return 0;
		}
        
        int numListeners;

        CheckFMODResult(gImpl->mStudioSystem->getNumListeners(&numListeners));

        return numListeners;
    }

    //Events

    AudioEngine::EventInstanceHandle AudioEngine::CreateEventInstance(const char* eventName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return InvalidEventInstanceHandle;
		}

        EventInstanceHandle newInstanceHandle = InvalidEventInstanceHandle;
        
        FMOD::Studio::EventDescription* description = nullptr;
        CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName, &description));

        if (description)
        {
            FMOD::Studio::EventInstance* instance = nullptr;

            CheckFMODResult(description->createInstance(&instance));

            if (instance)
            {
                newInstanceHandle.eventName = r2::utils::HashBytes32(eventName, strlen(eventName));
                newInstanceHandle.instance = ++gImpl->mEventInstanceCount;
                newInstanceHandle.padding = 0;

                s32 instanceIndex = FindNexAvailableEventInstanceIndex();

                R2_CHECK(instanceIndex != -1, "No more instances available");

                EventInstanceHandle& instanceHandleToUse = r2::sarr::At(*gImpl->mEventInstanceHandles, instanceIndex);

                instanceHandleToUse = newInstanceHandle;

                instance->setUserData(&instanceHandleToUse);

                r2::sarr::Push(*gImpl->mLiveEventInstances, instance);
            }
        }

        return newInstanceHandle;
    }

    void AudioEngine::ReleaseEventInstance(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

        R2_CHECK(instanceIndex != -1, "Should never happen?");

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        CheckFMODResult(instance->release());

        r2::sarr::RemoveAndSwapWithLastElement(*gImpl->mLiveEventInstances, instanceIndex);

        s32 handleIndex = FindInstanceHandleIndex(eventInstanceHandle);

        R2_CHECK(handleIndex != -1, "Should never happen?");

        r2::sarr::At(*gImpl->mEventInstanceHandles, handleIndex) = InvalidEventInstanceHandle;
    }

    AudioEngine::EventInstanceHandle AudioEngine::PlayEvent(const char* eventName, const Attributes3D& attributes3D, bool releaseAfterPlay )
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return InvalidEventInstanceHandle;
		}
		EventInstanceHandle newInstanceHandle = InvalidEventInstanceHandle;

		FMOD::Studio::EventDescription* description = nullptr;
		CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName, &description));

        if (description)
        {
            FMOD::Studio::EventInstance* instance = nullptr;

            CheckFMODResult(description->createInstance(&instance));

            if (instance)
            {
                FMOD_3D_ATTRIBUTES fmodAttributes;
                Attributes3DToFMOD3DAttributes(attributes3D, fmodAttributes);
                CheckFMODResult(instance->set3DAttributes(&fmodAttributes));

                CheckFMODResult(instance->start());

                if (releaseAfterPlay)
                {
                    CheckFMODResult(instance->release());
                }
                else
                {
                    //save it
					newInstanceHandle.eventName = r2::utils::HashBytes32(eventName, strlen(eventName));
					newInstanceHandle.instance = ++gImpl->mEventInstanceCount;
					newInstanceHandle.padding = 0;

					s32 instanceIndex = FindNexAvailableEventInstanceIndex();

					R2_CHECK(instanceIndex != -1, "No more instances available");

					EventInstanceHandle& instanceHandleToUse = r2::sarr::At(*gImpl->mEventInstanceHandles, instanceIndex);

					instanceHandleToUse = newInstanceHandle;

					instance->setUserData(&instanceHandleToUse);

					r2::sarr::Push(*gImpl->mLiveEventInstances, instance);
                }
            }
        }

        return newInstanceHandle;
    }

    bool AudioEngine::PlayEvent(const EventInstanceHandle& eventInstanceHandle, const Attributes3D& attributes3D, bool releaseAfterPlay)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

        s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

        if (instanceIndex == -1)
        {
            return false;
        }

        FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        FMOD_3D_ATTRIBUTES fmodAttributes;
        Attributes3DToFMOD3DAttributes(attributes3D, fmodAttributes);
        CheckFMODResult(instance->set3DAttributes(&fmodAttributes));

        CheckFMODResult(instance->start());

        if (releaseAfterPlay)
        {
            CheckFMODResult(instance->release());

            r2::sarr::RemoveAndSwapWithLastElement(*gImpl->mLiveEventInstances, instanceIndex);

            s32 instanceHandleIndex = FindInstanceHandleIndex(eventInstanceHandle);
            
            R2_CHECK(instanceHandleIndex != -1, "Should never happen");

            r2::sarr::At(*gImpl->mEventInstanceHandles, instanceHandleIndex) = InvalidEventInstanceHandle;
        }

        return true;
    }

    bool AudioEngine::PauseEvent(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			return false;
		}

        FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        bool isPaused;
        CheckFMODResult(instance->getPaused(&isPaused));
        CheckFMODResult(instance->setPaused(!isPaused));

        return true;
    }

    bool AudioEngine::StopEvent(const EventInstanceHandle& eventInstanceHandle, bool allowFadeOut)
    {
        //We may want an option to release after stop?
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			return false;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        FMOD_STUDIO_STOP_MODE stopMode;

        stopMode = FMOD_STUDIO_STOP_IMMEDIATE;
        if (allowFadeOut)
        {
            stopMode = FMOD_STUDIO_STOP_ALLOWFADEOUT;
        }

        CheckFMODResult(instance->stop(stopMode));

        return true;
    }

    bool AudioEngine::StopAllEvents(bool allowFadeOut)
    {
        //We may want an option to release after stop?
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}
		

        //for now I guess just loop through all active events
        //apparently the proper way is to stop all the events for the bus: https://qa.fmod.com/t/stopping-all-sounds/13954
        //We don't have bus support yet

        const u32 numLiveEvents = r2::sarr::Size(*gImpl->mLiveEventInstances);

		FMOD_STUDIO_STOP_MODE stopMode;

		stopMode = FMOD_STUDIO_STOP_IMMEDIATE;
		if (allowFadeOut)
		{
			stopMode = FMOD_STUDIO_STOP_ALLOWFADEOUT;
		}

		for (u32 i = 0; i < numLiveEvents; ++i)
		{
            FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, i);
            CheckFMODResult(instance->stop(stopMode));
		}

        return true;
    }

    void AudioEngine::SetEventParameterByName(const EventInstanceHandle& eventInstanceHandle, const char* paramName, float value)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			R2_CHECK(false, "Probably a bug");
			return;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        CheckFMODResult(instance->setParameterByName(paramName, value));
    }
    
    float AudioEngine::GetEventParameterValue(const EventInstanceHandle& eventInstanceHandle, const char* paramName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return 0.0f;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			R2_CHECK(false, "Probably a bug");
			return 0.0f;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);
        float value;
        CheckFMODResult(instance->getParameterByName(paramName, &value));

        return value;
    }

    bool AudioEngine::IsEventPlaying(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			R2_CHECK(false, "Probably a bug");
			return false;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        FMOD_STUDIO_PLAYBACK_STATE state;

        CheckFMODResult(instance->getPlaybackState(&state));

        return state == FMOD_STUDIO_PLAYBACK_PLAYING;
    } 

    bool AudioEngine::IsEventPaused(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
            R2_CHECK(false, "Probably a bug");
			return false;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);
        bool isPaused;
        CheckFMODResult(instance->getPaused(&isPaused));

        return isPaused;
    }

    bool AudioEngine::HasEventStopped(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			R2_CHECK(false, "Probably a bug");
			return false;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

		FMOD_STUDIO_PLAYBACK_STATE state;

		CheckFMODResult(instance->getPlaybackState(&state));

		return state == FMOD_STUDIO_PLAYBACK_STOPPED;
    }

    bool AudioEngine::IsEventValid(const EventInstanceHandle& eventInstanceHandle)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		s32 instanceIndex = FindInstanceIndex(eventInstanceHandle);

		if (instanceIndex == -1)
		{
			R2_CHECK(false, "Probably a bug");
			return false;
		}

		FMOD::Studio::EventInstance* instance = r2::sarr::At(*gImpl->mLiveEventInstances, instanceIndex);

        return instance->isValid();
    }

    bool AudioEngine::IsEvent3D(const char* eventName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		EventInstanceHandle newInstanceHandle = InvalidEventInstanceHandle;

		FMOD::Studio::EventDescription* description = nullptr;
		CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName, &description));
        
        bool is3D;
        CheckFMODResult(description->is3D(&is3D));
        
        return is3D;
    }

    bool AudioEngine::ReleaseAllEventInstances(const char* eventName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

        u32 hashBytes = r2::utils::HashBytes32(eventName, strlen(eventName));

        const auto numLiveEvents = r2::sarr::Size(*gImpl->mLiveEventInstances);

        r2::SArray<FMOD::Studio::EventInstance*>* remainder = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, FMOD::Studio::EventInstance*, numLiveEvents);

        for (u32 i = 0; i < numLiveEvents; ++i)
        {
            FMOD::Studio::EventInstance* nextInstance = r2::sarr::At(*gImpl->mLiveEventInstances, i);

            EventInstanceHandle* eventInstanceHandle = nullptr;

            nextInstance->getUserData((void**)&eventInstanceHandle);

            if (eventInstanceHandle->eventName == hashBytes)
            {
                CheckFMODResult(nextInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
                CheckFMODResult(nextInstance->release());
            }
            else
            {
                r2::sarr::Push(*remainder, nextInstance);
            }
        }

        r2::sarr::Clear(*gImpl->mLiveEventInstances);

        const u32 numInstancesRemaining = r2::sarr::Size(*remainder);
        for (u32 i = 0; i < numInstancesRemaining; ++i)
        {
            r2::sarr::Push(*gImpl->mLiveEventInstances, r2::sarr::At(*remainder, i));
        }

        FREE(remainder, *MEM_ENG_SCRATCH_PTR);

        //now remove all of the instance handles

        u32 numInstanceHandles = r2::sarr::Size(*gImpl->mEventInstanceHandles);
        for (u32 i = 0; i < numInstanceHandles; ++i)
        {
            EventInstanceHandle& nextHandle = r2::sarr::At(*gImpl->mEventInstanceHandles, i);
            if (nextHandle.eventName == hashBytes)
            {
                nextHandle = InvalidEventInstanceHandle;
            }
        }

        return true;
    }

    u32 AudioEngine::GetInstanceCount(const char* eventName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return 0;
		}

		EventInstanceHandle newInstanceHandle = InvalidEventInstanceHandle;

		FMOD::Studio::EventDescription* description = nullptr;
		CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName, &description));

        int instanceCount;
        
        CheckFMODResult(description->getInstanceCount(&instanceCount));

        return instanceCount;
    }

    void AudioEngine::GetMinMaxDistance(const char* eventName, float& minDistance, float& maxDistance)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

		EventInstanceHandle newInstanceHandle = InvalidEventInstanceHandle;

		FMOD::Studio::EventDescription* description = nullptr;
		CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName, &description));

        CheckFMODResult(description->getMinMaxDistance(&minDistance, &maxDistance));
    }

    //global params
    void AudioEngine::SetGlobalParameter(const char* paramName, float value)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        CheckFMODResult(gImpl->mStudioSystem->setParameterByName(paramName, value));
    }

    float AudioEngine::GetGlobalParamater(const char* paramName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return -1.0f;
		}
        float value = 0.0f;

        CheckFMODResult(gImpl->mStudioSystem->getParameterByName(paramName, &value));

        return value;
    }

    void AudioEngine::ReleaseAllEventInstances()
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the implementation");
			return;
		}

		const u32 numInstances = r2::sarr::Size(*gImpl->mLiveEventInstances);
        for (u32 i = 0; i < numInstances; ++i)
        {
            r2::sarr::At(*gImpl->mLiveEventInstances, i)->release();
        }

        r2::sarr::Clear(*gImpl->mLiveEventInstances);
        r2::sarr::Clear(*gImpl->mEventInstanceHandles);

        r2::sarr::Fill(*gImpl->mEventInstanceHandles, InvalidEventInstanceHandle);
    }

    s32 AudioEngine::FindInstanceHandleIndex(const EventInstanceHandle& eventInstance)
    {
        if (!gImpl)
        {
            R2_CHECK(false, "We haven't initialized the implementation");
            return -1;
        }

        const u32 numInstances = r2::sarr::Size(*gImpl->mEventInstanceHandles);
        for (u32 i = 0; i < numInstances; ++i)
        {
            const auto& instance = r2::sarr::At(*gImpl->mEventInstanceHandles, i);
            if (instance.instance == eventInstance.instance)
            {
                return static_cast<s32>(i);
            }
        }

        return -1;
    }

    s32 AudioEngine::FindNexAvailableEventInstanceIndex()
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the implementation");
			return -1;
		}

        for (u32 i = 0; i < r2::sarr::Capacity(*gImpl->mEventInstanceHandles); ++i)
        {
            if (r2::sarr::At(*gImpl->mEventInstanceHandles, i).instance == -1)
            {
                return static_cast<s32>(i);
            }
        }

        return -1;
    }

    s32 AudioEngine::FindInstanceIndex(const EventInstanceHandle& eventInstance)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the implementation");
			return -1;
		}

		const u32 numInstances = r2::sarr::Size(*gImpl->mLiveEventInstances);
		for (u32 i = 0; i < numInstances; ++i)
		{
			const auto& instance = r2::sarr::At(*gImpl->mLiveEventInstances, i);

            EventInstanceHandle* eventInstanceHandle;

            instance->getUserData((void**)(&eventInstanceHandle));

			if (eventInstanceHandle->instance == eventInstance.instance)
			{
				return static_cast<s32>(i);
			}
		}

		return -1;
    }
}
