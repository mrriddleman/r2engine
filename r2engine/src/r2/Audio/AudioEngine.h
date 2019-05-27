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
    public:
        using SoundID = s64;
        using ChannelID = s64;
        
        static const SoundID InvalidSoundID = -1;
        static const ChannelID InvalidChannelID = -1;
        
        static void Init();
        static void Update();
        static void Shutdown();
        
        enum _SoundFlags
        {
            IS_3D = 1 << 0,
            LOOP = 1 << 1,
            STREAM = 1 << 2,
        };
        
        using SoundFlags = r2::Flags<_SoundFlags, u8>;
        
        struct SoundDefinition
        {
            char soundName[fs::FILE_PATH_LENGTH];
            float defaultVolume = 0.0f;
            float minDistance = 0.0f;
            float maxDistance = 0.0f;
            SoundFlags flags;
            
            SoundDefinition() = default;
            ~SoundDefinition() = default;
            SoundDefinition(const SoundDefinition& soundDef);
            SoundDefinition& operator=(const SoundDefinition& soundDef);
            SoundDefinition(SoundDefinition && soundDef) = default;
            SoundDefinition& operator=(SoundDefinition&& soundDef) = default;
        };
        
        SoundID RegisterSound(const SoundDefinition& soundDef, bool load = true);
        void UnregisterSound(SoundID soundID);
        
        bool LoadSound(SoundID soundID);
        void UnloadSound(SoundID handle);
        void Set3DListenerAndOrientation(const glm::vec3& position, const glm::vec3& look, const glm::vec3& up);
        ChannelID PlaySound(SoundID sound, const glm::vec3& pos= glm::vec3{0,0,0}, float volume = 0.5f, float pitch = 1.0f);
        void StopChannel(ChannelID channelID, float fadeOutInSeconds = 0.0f);
        
        void StopAllChannels(float fadeOutInSeconds = 0.0f);
        void PauseChannel(ChannelID channelID);
        void PauseAll();
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
