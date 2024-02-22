//
//  SDL2Platform.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef SDL2Platform_hpp
#define SDL2Platform_hpp

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Platform/Platform.h"
#include "r2/Platform/SDL2/SDL2GameController.h"
#include <SDL2/SDL.h>

struct SDL_Window;

namespace r2::fs
{
    class FileStorageArea;
}

namespace r2
{
    class R2_API SDL2Platform: public Platform
    {
    public:
        static const u64 MAX_NUM_MEMORY_AREAS;
        static const u64 TOTAL_INTERNAL_ENGINE_MEMORY;
        static const u64 TOTAL_INTERNAL_PERMANENT_MEMORY;
        static const u64 TOTAL_SCRATCH_MEMORY;
        
        virtual bool Init(std::unique_ptr<r2::Application> app) override;
        virtual void Run() override;
        virtual void Shutdown() override;
        
        virtual const f64 TickRate() const override;
        virtual const s32 NumLogicalCPUCores() const override;
        virtual const s32 SystemRAM() const override;
        virtual const s32 CPUCacheLineSize() const override;
        
        virtual const std::string RootPath() const override;
        virtual const std::string AppPath() const override;
        virtual const std::string SoundDefinitionsPath() const override;
        
        virtual void ResetAccumulator() override;


		virtual u32 GetNumberOfAttachedGameControllers() const override;
		virtual void CloseGameController(io::ControllerID index) override;
		virtual void CloseAllGameControllers() override;
		virtual bool IsGameControllerConnected(io::ControllerID index) const override;
		virtual const char* GetGameControllerMapping(r2::io::ControllerID controllerID) const override;
		virtual const char* GetGameControllerButtonName(r2::io::ControllerID controllerID, r2::io::ControllerButtonName buttonName) const override;
		virtual const char* GetGameControllerAxisName(r2::io::ControllerID controllerID, r2::io::ControllerAxisName axisName) const override;
		virtual const char* GetGameControllerName(r2::io::ControllerID controllerID) const override;
		virtual s32 GetPlayerIndex(r2::io::ControllerID controllerID) const override;
		virtual void SetPlayerIndex(r2::io::ControllerID controllerID, s32 playerIndex) override;
		virtual io::ControllerType GetControllerType(r2::io::ControllerID controllerID)const override;

    private:
        friend Platform;
        SDL2Platform();
        
        void SetWindowTitle(const char* title);
        void ProcessEvents();
        
        io::ControllerID GetControllerIDFromControllerInstance(io::ControllerInstanceID instanceID);
        
        static std::unique_ptr<Platform> CreatePlatform();
        static std::unique_ptr<Platform> s_platform;
        
        r2::fs::FileStorageArea* mRootStorage;
        r2::fs::FileStorageArea* mAppStorage;
        
        SDL2GameController mSDLGameControllers[Engine::NUM_PLATFORM_CONTROLLERS];

        char mPrefPath[r2::fs::FILE_PATH_LENGTH];
        char mBasePath[r2::fs::FILE_PATH_LENGTH];
        char mSoundDefinitionPath[r2::fs::FILE_PATH_LENGTH];
        char mApplicationName[r2::fs::FILE_PATH_LENGTH];
        u64 mStartTime;
        bool mRunning;
        bool mResync;
        
    };
}

#endif

#endif /* SDL2Platform_hpp */
