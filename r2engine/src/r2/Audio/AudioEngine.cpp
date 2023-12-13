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

    glm::vec3 FMODVectorToGLM(const FMOD_VECTOR& fmodVec)
    {
        glm::vec3 r;
        r.x = fmodVec.x;
        r.y = fmodVec.y;
        r.z = fmodVec.z;

        return r;
    }

    void Attributes3DToFMOD3DAttributes(const r2::audio::AudioEngine::Attributes3D& attributes, FMOD_3D_ATTRIBUTES& fmodAttributes)
    {
        fmodAttributes.forward = GLMToFMODVector(attributes.look);
        fmodAttributes.position = GLMToFMODVector(attributes.position);
        fmodAttributes.up = GLMToFMODVector(attributes.up);
        fmodAttributes.velocity = GLMToFMODVector(attributes.velocity);
    }

    void FMOD3DAttributesToAttributes3D(const FMOD_3D_ATTRIBUTES& fmodAttributes, r2::audio::AudioEngine::Attributes3D& attributes)
    {
        attributes.look = FMODVectorToGLM(fmodAttributes.forward);
        attributes.position = FMODVectorToGLM(fmodAttributes.position);
        attributes.up = FMODVectorToGLM(fmodAttributes.up);
        attributes.velocity = FMODVectorToGLM(fmodAttributes.velocity);
    }
    
    void CheckFMODResult(FMOD_RESULT result)
    {
        if (result != FMOD_OK && result != FMOD_ERR_INVALID_HANDLE )
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
    
    struct Implementation
    {

        void Init(const r2::mem::utils::MemBoundary& memoryBoundary, u32 boundsChecking);
        void Update();
        void Shutdown();

        static u64 MemorySize(u32 maxNumBanks, u32 maxNumEvents, u32 headerSize, u32 boundsCheckingSize);
        
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

        CheckFMODResult( FMOD::Studio::System::create(&mStudioSystem) );
        CheckFMODResult(mStudioSystem->initialize(MAX_NUM_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr));
        CheckFMODResult(mStudioSystem->getCoreSystem(&mSystem));
    }
    
    void Implementation::Update()
    {
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
        
        R2_CHECK(gImpl == nullptr, "Should always be the case");
        //do memory setup here
        {
            r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
            
            u32 boundsChecking = 0;
#if defined(R2_DEBUG)|| defined(R2_RELEASE)
            boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
            u64 memorySizeForImplementation = Implementation::MemorySize(MAX_NUM_BANKS, MAX_NUM_EVENT_INSTANCES, r2::mem::StackAllocator::HeaderSize(), boundsChecking);
            
            r2::mem::MemoryArea* memArea = r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle);
            AudioEngine::mSoundMemoryAreaHandle = memArea->AddSubArea(memorySizeForImplementation);
            R2_CHECK(AudioEngine::mSoundMemoryAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "We have an invalid sound memory area!");
            
            r2::mem::MemoryArea::SubArea* soundMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(engineMem.internalEngineMemoryHandle)->GetSubArea(AudioEngine::mSoundMemoryAreaHandle);
            
            gImpl = ALLOC(Implementation, *MEM_ENG_PERMANENT_PTR);

			R2_CHECK(gImpl != nullptr, "Something went wrong trying to allocate the implementation for sound!");

            gImpl->Init(soundMemoryArea->mBoundary, boundsChecking);

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
			r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

			r2::SArray<const byte*>* manifestDataArray = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const byte*, 1);

			r2::asset::lib::GetManifestDataForType(assetLib, asset::SOUND_DEFINTION, manifestDataArray);

			const auto* soundDefinitions = flat::GetSoundDefinitions(r2::sarr::At(*manifestDataArray, 0));

            //We need to figure out all of the banks that were already loaded - save their paths/names
            const u32 numBankSlots = r2::sarr::Capacity(*gImpl->mLoadedBanks);

            r2::SArray<char*>* loadedBanks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, numBankSlots);

            for (u32 i = 0; i < numBankSlots; ++i)
            {
                FMOD::Studio::Bank* bank = r2::sarr::At(*gImpl->mLoadedBanks, i);
                
                if (bank)
                {
                    char* path = ALLOC_ARRAYN(char, r2::fs::FILE_PATH_LENGTH, *MEM_ENG_SCRATCH_PTR);
                    int retrieved;
                    CheckFMODResult( bank->getPath(path, r2::fs::FILE_PATH_LENGTH, &retrieved) );

                    r2::sarr::Push(*loadedBanks, path);
                }
            }

            StopAllEvents(false);

            ReleaseAllEventInstances();
            
            UnloadAllBanks();

			char directoryPath[r2::fs::FILE_PATH_LENGTH];

		    r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SOUNDS, soundDefinitions->masterBank()->binPath()->c_str(), directoryPath); //@NOTE(Serge): maybe should be SOUND_DEFINITIONS?

            gImpl->mMasterBank = LoadBank(directoryPath, FMOD_STUDIO_LOAD_BANK_NORMAL);

			r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SOUNDS, soundDefinitions->masterBankStrings()->binPath()->c_str(), directoryPath); //@NOTE(Serge): maybe should be SOUND_DEFINITIONS?

            gImpl->mMasterStringsBank = LoadBank(directoryPath, FMOD_STUDIO_LOAD_BANK_NORMAL);
            

            //load all of the banks that were loaded before
            //problem is that the paths are FMOD paths not an os path
            //this needs to be resolved somehow

            const u32 numPaths = r2::sarr::Size(*loadedBanks);
            for (u32 i = 0; i < numPaths; ++i)
            {
                char* loadedBank = r2::sarr::At(*loadedBanks, i);

				char bankPath[r2::fs::FILE_PATH_LENGTH];
				BuildBankFilePathFromFMODPath(loadedBank, bankPath, r2::fs::FILE_PATH_LENGTH);

                LoadBank(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL);
            }

            FMOD::Studio::Bank* stringsBank = r2::sarr::At(*gImpl->mLoadedBanks, gImpl->mMasterStringsBank);

            R2_CHECK(stringsBank != nullptr, "?");

            int stringsCount = 0;
            CheckFMODResult(stringsBank->getStringCount(&stringsCount));

            for (int i = 0; i < stringsCount; ++i)
            {
                FMOD_GUID guid;
                char path[fs::FILE_PATH_LENGTH];
                int retrieved;
                CheckFMODResult(stringsBank->getStringInfo(i, &guid, path, fs::FILE_PATH_LENGTH, &retrieved));

            //    printf("FMOD Path: %s\n", path);
            }
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
                
            //free all of the temp data

            const s32 numLoadedBanks = r2::sarr::Size(*loadedBanks);

            for (s32 i = numLoadedBanks - 1; i >= 0; --i)
            {
                FREE(r2::sarr::At(*loadedBanks, i), *MEM_ENG_SCRATCH_PTR);
            }

            FREE(loadedBanks, *MEM_ENG_SCRATCH_PTR);
            FREE(manifestDataArray, *MEM_ENG_SCRATCH_PTR);
        }
    }
    
    int AudioEngine::GetSampleRate()
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");
        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return sampleRate;
    }
    
    AudioEngine::SpeakerMode AudioEngine::GetSpeakerMode()
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        int sampleRate, numRawSpeakers;
        FMOD_SPEAKERMODE speakerMode;
        
        CheckFMODResult( gImpl->mSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers) );
        return static_cast<SpeakerMode>(speakerMode);
    }
    
    s32 AudioEngine::GetNumberOfDrivers()
    {
        R2_CHECK(gImpl != nullptr, "We haven't initialized the AudioEngine yet!");

        s32 numDrivers = 0;
        CheckFMODResult(gImpl->mSystem->getNumDrivers(&numDrivers));
        return numDrivers;
    }
    
    u32 AudioEngine::GetCurrentDriver()
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

            if (!bank)
            {
                continue;
            }

            char nextBankPath[r2::fs::FILE_PATH_LENGTH];
            int retrieved;

            bank->getPath(nextBankPath, r2::fs::FILE_PATH_LENGTH, &retrieved);
            if (!retrieved)
            {
                continue;
            }

			char bankPath[r2::fs::FILE_PATH_LENGTH];
            BuildBankFilePathFromFMODPath(nextBankPath, bankPath, r2::fs::FILE_PATH_LENGTH);

            char sanitizedPath[r2::fs::FILE_PATH_LENGTH];

            r2::fs::utils::SanitizeSubPath(path, sanitizedPath);

            if (retrieved && strcmp(bankPath, sanitizedPath) == 0)
            {
                return static_cast<BankHandle>(i);
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

    AudioEngine::BankHandle AudioEngine::GetBankHandle(u64 bankAssetName)
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

            if (!bank)
            {
                continue;
            }

            char nextBankPath[r2::fs::FILE_PATH_LENGTH];
            int retrieved;

            bank->getPath(nextBankPath, r2::fs::FILE_PATH_LENGTH, &retrieved);
            if (!retrieved)
            {
                continue;
            }

            char bankPath[r2::fs::FILE_PATH_LENGTH];
            BuildBankFilePathFromFMODPath(nextBankPath, bankPath, r2::fs::FILE_PATH_LENGTH);

            auto assetName = r2::asset::GetAssetNameForFilePath(bankPath, r2::asset::SOUND);

            if (assetName == bankAssetName)
            {
                return static_cast<BankHandle>(i);
            }
        }

        return InvalidBank;
    }

    bool AudioEngine::UnloadSoundBank(BankHandle bankHandle)
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

    void AudioEngine::SetListener3DAttributes(Listener listenerIndex, const Attributes3D& attributes3D)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        FMOD_3D_ATTRIBUTES fmodAttributes;
        
        Attributes3DToFMOD3DAttributes(attributes3D, fmodAttributes);

        CheckFMODResult(gImpl->mStudioSystem->setListenerAttributes(static_cast<int>(listenerIndex), &fmodAttributes));
    }

    void AudioEngine::GetListener3DAttributes(Listener listenerIndex, Attributes3D& attributes3D)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        FMOD_3D_ATTRIBUTES fmodAttributes;

        CheckFMODResult(gImpl->mStudioSystem->getListenerAttributes(listenerIndex, &fmodAttributes));

        FMOD3DAttributesToAttributes3D(fmodAttributes, attributes3D);
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

    AudioEngine::EventInstanceHandle AudioEngine::PlayEvent(const char* eventName, bool releaseAfterPlay)
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

    bool AudioEngine::PlayEvent(const EventInstanceHandle& eventInstanceHandle, bool releaseAfterPlay)
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

    bool AudioEngine::HasEvent(const char* eventName)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return false;
		}

		FMOD::Studio::EventDescription* description = nullptr;
		gImpl->mStudioSystem->getEvent(eventName, &description);

        return description != nullptr;
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

    bool AudioEngine::SetAttributes3DForEvent(const EventInstanceHandle& eventInstanceHandle, const Attributes3D& attributes3D)
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

        FMOD_3D_ATTRIBUTES fmodAttributes;

        Attributes3DToFMOD3DAttributes(attributes3D, fmodAttributes);

        CheckFMODResult(instance->set3DAttributes(&fmodAttributes));

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

    bool AudioEngine::IsEventInstanceHandleValid(const EventInstanceHandle& eventInstanceHandle)
    {
        return eventInstanceHandle.instance != -1 && eventInstanceHandle.eventName != 0;
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

#ifdef R2_EDITOR
    void AudioEngine::GetEventNames(std::vector<std::string>& eventNames)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

		FMOD::Studio::Bank* stringsBank = r2::sarr::At(*gImpl->mLoadedBanks, gImpl->mMasterStringsBank);

		R2_CHECK(stringsBank != nullptr, "?");

		int stringsCount = 0;

		CheckFMODResult(stringsBank->getStringCount(&stringsCount));

		for (int i = 0; i < stringsCount; ++i)
		{
			FMOD_GUID guid;
			char path[fs::FILE_PATH_LENGTH];
			int retrieved;
			CheckFMODResult(stringsBank->getStringInfo(i, &guid, path, fs::FILE_PATH_LENGTH, &retrieved));

            std::string strPath = path;

            auto result = strPath.find("event:/");

			if (retrieved && result != std::string::npos)
			{
                eventNames.push_back(strPath);
			}
		}
    }

    void AudioEngine::GetEventParameters(const std::string& eventName, std::vector<AudioEngineParameterDesc>& parameterDescriptions)
    {
		if (!gImpl)
		{
			R2_CHECK(false, "We haven't initialized the AudioEngine yet!");
			return;
		}

        FMOD::Studio::EventDescription* eventDescription = nullptr;

        CheckFMODResult(gImpl->mStudioSystem->getEvent(eventName.c_str(), &eventDescription));

        if (eventDescription)
        {
            int parameterCount = 0;
            CheckFMODResult(eventDescription->getParameterDescriptionCount(&parameterCount));

            for (int i = 0; i < parameterCount; ++i)
            {
                FMOD_STUDIO_PARAMETER_DESCRIPTION fmodParamDescription;
                CheckFMODResult(eventDescription->getParameterDescriptionByIndex(i, &fmodParamDescription));

                AudioEngineParameterDesc paramDesc;
                paramDesc.name = fmodParamDescription.name;
                paramDesc.defaultValue = fmodParamDescription.defaultvalue;
                paramDesc.minValue = fmodParamDescription.minimum;
                paramDesc.maxValue = fmodParamDescription.maximum;

                parameterDescriptions.push_back(paramDesc);
            }
        }
    }

#endif

    void AudioEngine::BuildBankFilePathFromFMODPath(const char* fmodPath, char* outpath, u32 size)
    {
		char bankPathURI[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::GetRelativePath("bank:/", fmodPath, bankPathURI);

		r2::fs::utils::BuildPathFromCategory(fs::utils::SOUNDS, bankPathURI, outpath);

		r2::fs::utils::AppendExt(outpath, ".bank");
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
