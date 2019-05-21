//
//  AudioEngine.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-20.
//

#ifndef AudioEngine_h
#define AudioEngine_h

#include "glm/vec3.hpp"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::audio
{
    class AudioEngine
    {
        static void Init();
        static void Update();
        static void Shutdown();
        
        using SoundID = s64;
        using ChannelID = s64;
        
        static const SoundID InvalidSoundID = -1;
        static const ChannelID InvalidChannelID = -1;
        
        SoundID LoadSound(const char* soundName, bool is3D = true, bool loop=false, bool stream=false);
        void UnloadSound(SoundID handle);
        void Set3DListenerAndOrientation(const glm::vec3& position, const glm::vec3& look, const glm::vec3& up);
        ChannelID PlaySound(SoundID sound, const glm::vec3& pos= glm::vec3{0,0,0}, float volume = 0.0f, float picth = 1.0f);
        void StopChannel(ChannelID channelID);
        void PauseChannel(ChannelID channelID);
        void StopAllChannels();
        void SetChannel3DPosition(ChannelID channelID, const glm::vec3& position);
        void SetChannelVolume(ChannelID channelID, float volume);
        bool IsChannelPlaying(ChannelID channelID) const;
        float GetChannelPitch(ChannelID channelID) const;
        void SetChannelPitch(ChannelID channelID, float picth);
        
    private:
        static r2::mem::MemoryArea::MemorySubArea::Handle mSoundMemoryAreaHandle;
        static r2::mem::LinearArena* mSoundAllocator;
        
        SoundID NextAvailableSoundID();
        ChannelID NextAvailableChannelID();
    };
}

#endif /* AudioEngine_h */
