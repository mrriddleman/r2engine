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
        
        virtual const u32 TickRate() const override;
        virtual const s32 NumLogicalCPUCores() const override;
        virtual const s32 SystemRAM() const override;
        virtual const s32 CPUCacheLineSize() const override;
        
        virtual const std::string RootPath() const override;
        virtual const std::string AppPath() const override;
        virtual const std::string SoundDefinitionsPath() const override;
    private:
        friend Platform;
        SDL2Platform();
        
        int SetupSDLOpenGL();
        void TestFiles();
        
        static std::unique_ptr<Platform> CreatePlatform();
        static std::unique_ptr<Platform> s_platform;
        SDL_Window * moptrWindow;
        SDL_GLContext mglContext;
        
        r2::fs::FileStorageArea* mRootStorage;
        r2::fs::FileStorageArea* mAppStorage;
        
        char* mPrefPath;
        char* mBasePath;
        char mSoundDefinitionPath[r2::fs::FILE_PATH_LENGTH];
        bool mRunning;
        
        
    };
}

#endif

#endif /* SDL2Platform_hpp */
