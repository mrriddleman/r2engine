//
//  AudioEngine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-20.
//

#include "AudioEngine.h"
#include "fmod.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include <cstring>

namespace
{
    const u32 MAX_NUM_SOUNDS = 512;
    const u32 MAX_NUM_CHANNELS = 128;
}

namespace r2::audio
{
    r2::mem::MemoryArea::MemorySubArea::Handle AudioEngine::mSoundMemoryAreaHandle = r2::mem::MemoryArea::MemorySubArea::Invalid;
    r2::mem::LinearArena* AudioEngine::mSoundAllocator = nullptr;
    
    const AudioEngine::SoundID AudioEngine::InvalidSoundID;
    const AudioEngine::ChannelID AudioEngine::InvalidChannelID;
    
    FMOD_VECTOR GLMToFMODVector(const glm::vec3& vec)
    {
        FMOD_VECTOR v;
        v.x = vec.x;
        v.y = vec.y;
        v.z = vec.z;
        return v;
    }
    
    struct Implementation
    {
        void Init(r2::mem::LinearArena& allocator);
        void Update();
        void Shutdown(r2::mem::LinearArena& allocator);
        
        static u64 TotalAllocatedSize();
        static u64 SoundPoolSize();
        
        using SoundList = r2::SArray<FMOD::Sound*>*;
        using ChannelList = r2::SArray<FMOD::Channel*>*;
        
        FMOD::System* mSystem = nullptr;
        SoundList mSounds = nullptr;
        ChannelList mChannels = nullptr;
        byte* mSoundPool = nullptr;
    };
    
    void CheckFMODResult(FMOD_RESULT result)
    {
        R2_CHECK(result == FMOD_OK, "We had an error in FMOD: %i", result);
    }
    
    u64 Implementation::TotalAllocatedSize()
    {
        return MAX_NUM_SOUNDS * (sizeof(FMOD::Sound*)) + MAX_NUM_CHANNELS * ( sizeof(FMOD::Channel*));
    }
    
    u64 Implementation::SoundPoolSize()
    {
        return Megabytes(4);
    }
    
    void Implementation::Init(r2::mem::LinearArena& allocator)
    {
        mSounds = MAKE_SARRAY(allocator, FMOD::Sound*, MAX_NUM_SOUNDS);
        
        R2_CHECK(mSounds != nullptr, "We should have sounds initialized!");
        
        for (u64 i = 0 ; i < MAX_NUM_SOUNDS; ++i)
        {
            r2::sarr::At(*mSounds, i) = nullptr;
        }
        
        mChannels = MAKE_SARRAY(allocator, FMOD::Channel*, MAX_NUM_CHANNELS);
        
        R2_CHECK(mChannels != nullptr, "We should have channels initialized!");
        
        for (u64 i = 0 ; i < MAX_NUM_CHANNELS; ++i)
        {
            r2::sarr::At(*mChannels, i) = nullptr;
        }
        
        ALLOC_BYTESN(allocator, SoundPoolSize(), 16);
        
        CheckFMODResult( FMOD::Memory_Initialize(mSoundPool, SoundPoolSize(), 0, 0, 0) );
        CheckFMODResult( FMOD::System_Create(&mSystem) );
        CheckFMODResult( mSystem->init(MAX_NUM_CHANNELS, FMOD_INIT_NORMAL, nullptr));
    }
    
    void Implementation::Update()
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            bool isPlaying = false;
            r2::sarr::At(*mChannels, i)->isPlaying(&isPlaying);
            if (!isPlaying)
            {
                r2::sarr::At(*mChannels, i) = nullptr;
            }
        }
        
        CheckFMODResult( mSystem->update() );
    }
    
    void Implementation::Shutdown(r2::mem::LinearArena& allocator)
    {
        CheckFMODResult( mSystem->release() );
        
        FREE(mSoundPool, allocator);
        FREE(mChannels, allocator);
        FREE(mSounds, allocator);
    }
    
    static Implementation* gImpl = nullptr;

    void AudioEngine::Init()
    {
        //do memory setup here
        {
            r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
            
            u64 soundAreaSize =
            sizeof(r2::mem::LinearArena) +
            sizeof(Implementation) +
            Implementation::TotalAllocatedSize() +
            Implementation::SoundPoolSize();

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
    }
    
    void AudioEngine::Update()
    {
        gImpl->Update();
    }
    
    void AudioEngine::Shutdown()
    {
        gImpl->Shutdown(*AudioEngine::mSoundAllocator);
        
        FREE(gImpl, *AudioEngine::mSoundAllocator);
    }
    
    AudioEngine::SoundID AudioEngine::LoadSound(const char* soundName, bool is3D, bool loop, bool stream)
    {
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            FMOD::Sound* sound = r2::sarr::At(*gImpl->mSounds, i);
            if (sound != nullptr)
            {
                char name[fs::FILE_PATH_LENGTH];
                sound->getName(name, fs::FILE_PATH_LENGTH);
                
                if (strcmp(soundName, name) == 0)
                {
                    return i;
                }
            }
        }
        
        FMOD_MODE mode = FMOD_DEFAULT;
        mode |= is3D ? FMOD_3D : FMOD_2D;
        mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        mode |= stream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
        
        FMOD::Sound* ptrSound = nullptr;
        gImpl->mSystem->createSound(soundName, mode, nullptr, &ptrSound);
        
        if (ptrSound)
        {
            auto soundID = NextAvailableSoundID();
            if (soundID != AudioEngine::InvalidSoundID)
            {
                r2::sarr::At(*gImpl->mSounds, soundID) = ptrSound;
                return soundID;
            }
        }
        
        return AudioEngine::InvalidSoundID;
    }
    
    void AudioEngine::UnloadSound(SoundID soundID)
    {
        FMOD::Sound* sound = r2::sarr::At(*gImpl->mSounds, soundID);
        if (sound)
        {
            sound->release();
            r2::sarr::At(*gImpl->mSounds, soundID) = nullptr;
        }
    }
    
    void AudioEngine::Set3DListenerAndOrientation(const glm::vec3& position, const glm::vec3& look, const glm::vec3& up)
    {
        FMOD_VECTOR pos = GLMToFMODVector(position);
        FMOD_VECTOR forward = GLMToFMODVector(look);
        FMOD_VECTOR upVec = GLMToFMODVector(up);
        
        gImpl->mSystem->set3DListenerAttributes(0, &pos, nullptr, &forward, &upVec);
    }
    
    AudioEngine::ChannelID AudioEngine::PlaySound(AudioEngine::SoundID soundID, const glm::vec3& pos, float volume)
    {

        FMOD::Sound* soundPtr = r2::sarr::At(*gImpl->mSounds, soundID);
        if (!soundPtr)
        {
            return AudioEngine::InvalidChannelID;
        }
        
        FMOD::Channel* channelPtr = nullptr;
        
        gImpl->mSystem->playSound(soundPtr, nullptr, true, &channelPtr);
        if (channelPtr)
        {
            FMOD_VECTOR position = GLMToFMODVector(pos);
            channelPtr->set3DAttributes(&position, nullptr);
            channelPtr->setVolume(volume);
            channelPtr->setPaused(false);
            
            u32 channelID = NextAvailableChannelID();
            r2::sarr::At(*gImpl->mChannels, channelID) = channelPtr;
            
            return channelID;
        }
        
        return AudioEngine::InvalidChannelID;
    }
    
    void AudioEngine::StopChannel(AudioEngine::ChannelID channelID)
    {
        FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (!channel)
        {
            return;
        }
        
        channel->stop();
    }
    
    void AudioEngine::PauseChannel(AudioEngine::ChannelID channelID)
    {
        FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (!channel)
        {
            return;
        }
        
        bool isPaused;
        channel->getPaused(&isPaused);
        isPaused = !isPaused;
        channel->setPaused(isPaused);
    }
    
    void AudioEngine::StopAllChannels()
    {
        for (u64 i = 0; i < MAX_NUM_CHANNELS; ++i)
        {
            FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, i);
            if (channel)
            {
                channel->stop();
            }
        }
    }
    
    void AudioEngine::SetChannel3DPosition(AudioEngine::ChannelID channelID, const glm::vec3& position)
    {
        FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (!channel)
        {
            return;
        }
        
        FMOD_VECTOR pos = GLMToFMODVector(position);
        channel->set3DAttributes(&pos, nullptr);
    }
    
    void AudioEngine::SetChannelVolume(AudioEngine::ChannelID channelID, float volume)
    {
        FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (!channel)
        {
            return;
        }
        
        channel->setVolume(volume);
    }
    
    bool AudioEngine::IsChannelPlaying(AudioEngine::ChannelID channelID) const
    {
        FMOD::Channel* channel = r2::sarr::At(*gImpl->mChannels, channelID);
        if (!channel)
        {
            return false;
        }
        
        bool isPlaying;
        channel->isPlaying(&isPlaying);
        return isPlaying;
    }
 
    AudioEngine::SoundID AudioEngine::NextAvailableSoundID()
    {
        for (u64 i = 0; i < MAX_NUM_SOUNDS; ++i)
        {
            if (r2::sarr::At(*gImpl->mSounds, i) == nullptr)
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
}
