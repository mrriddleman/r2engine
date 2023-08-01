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

namespace r2::audio
{
    class R2_API AudioEngine
    {
    public:
        
        enum SpeakerMode
        {
            DEFAULT,
            RAW,
            MONO,
            STEREO,
            QUAD,
            SURROUND,
            FIVE_POINT1,
            SEVEN_POINT1,
            SEVEN_POINT1POINT4,
            MAX
        };

        enum _LoadBankFlags
        {
            LOAD_BANK_NORMAL = 0,
            LOAD_BANK_NONBLOCKING,
            LOAD_BANK_DECOMPRESS_SAMPLES,
            LOAD_BANK_UNENCRYPTED
        };

        static void Init();
        static void Update();
        static void Shutdown();

#ifdef R2_ASSET_PIPELINE
        static void PushNewlyBuiltSoundDefinitions(std::vector<std::string> paths);
#endif
        static void ReloadSoundDefinitions();
        
        static int GetSampleRate();
        static SpeakerMode GetSpeakerMode();
        
        static s32 GetNumberOfDrivers();
        static u32 GetCurrentDriver();
        static void SetDriver(int driverId);
        static void GetDriverInfo(int driverId, char* driverName, u32 driverNameLength, u32& systemRate, SpeakerMode& mode, u32& speakerModeChannels);
        
        /*
        * New Interface proposal
        */
        using Listener = u32;

        static const Listener DEFAULT_LISTENER = 0;

		struct Attributes3D
		{
            glm::vec3 position;
            glm::vec3 velocity;
            glm::vec3 look;
            glm::vec3 up;
		};

		struct EventInstanceHandle
		{
			s64 instance = -1;
			u32 eventName = 0;
			u32 padding = 0;
		};

        static const EventInstanceHandle InvalidEventInstanceHandle;

        //Banks
        using BankHandle = s64;

        static const BankHandle InvalidBank = -1;
        
        static BankHandle LoadBank(const char* path, u32 flags);
        static bool UnloadSoundBank(BankHandle bank);
        static bool IsBankValid(BankHandle bank);
        static bool HasBankFinishedLoading(BankHandle bank);
        
        static void UnloadAllBanks();

        static bool LoadSampleDataForBank(BankHandle bank);
        static bool UnloadSampleDataForBank(BankHandle bank);
        static bool HasSampleDataFinishedLoadingForBank(BankHandle bank);

        //Listeners - there's always 1 I think?
        static void SetNumListeners(u32 numListeners);
        static void SetListener3DAttributes(Listener listenerIndex, const Attributes3D& attributes3D);
        static void GetListener3DAttributes(Listener listenerIndex, Attributes3D& attributes3D);
        static u32  GetNumListeners();

        //Events
        static EventInstanceHandle CreateEventInstance(const char* eventName);
        static void ReleaseEventInstance(const EventInstanceHandle& eventInstanceHandle);

        //@NOTE(Serge): if you use releaseAfterPlay, then we don't return a valid EventInstanceHandle
        static EventInstanceHandle PlayEvent(const char* eventName, const Attributes3D& attributes3D, bool releaseAfterPlay = true);
        static EventInstanceHandle PlayEvent(const char* eventName, bool releaseAfterPlay = true);
        static bool PlayEvent(const EventInstanceHandle& eventInstanceHandle, bool releaseAfterPlay = false);
        static bool PlayEvent(const EventInstanceHandle& eventInstanceHandle, const Attributes3D& attributes3D, bool releaseAfterPlay = false);

        static bool PauseEvent(const EventInstanceHandle& eventInstance);
        static bool StopEvent(const EventInstanceHandle& eventInstance, bool allowFadeOut);

        static bool StopAllEvents(bool allowFadeOut);

        static void SetEventParameterByName(const EventInstanceHandle& eventInstanceHandle, const char* paramName, float value);
        static float GetEventParameterValue(const EventInstanceHandle& eventInstanceHandle, const char* paramName);

        static bool IsEventPlaying(const EventInstanceHandle& eventInstance);
        static bool IsEventPaused(const EventInstanceHandle& eventInstance);
        static bool HasEventStopped(const EventInstanceHandle& eventInstance);
        static bool IsEventValid(const EventInstanceHandle& eventInstance);

        static bool IsEventInstanceHandleValid(const EventInstanceHandle& eventInstanceHandle);

        static bool IsEvent3D(const char* eventName);
        static bool ReleaseAllEventInstances(const char* eventName);
        static u32  GetInstanceCount(const char* eventName);
        static void GetMinMaxDistance(const char* eventName, float& minDistance, float& maxDistance);

        //global params
        static void SetGlobalParameter(const char* paramName, float value);
        static float GetGlobalParamater(const char* paramName);

    private:

        static void ReleaseAllEventInstances();
        static void BuildBankFilePathFromFMODPath(const char* fmodPath, char* outpath, u32 size);
        static s32 FindInstanceHandleIndex(const EventInstanceHandle& eventInstance);
        static s32 FindInstanceIndex(const EventInstanceHandle& eventInstance);
        static s32 FindNexAvailableEventInstanceIndex();

        static r2::mem::MemoryArea::SubArea::Handle mSoundMemoryAreaHandle;
    };
}

#endif /* AudioEngine_h */
