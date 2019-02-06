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

namespace r2
{
    class R2_API SDL2Platform: public Platform
    {
    public:
        
        virtual bool Init(std::unique_ptr<r2::Application> app) override;

        virtual void Run() override;
        virtual void Shutdown() override;
        
        virtual const std::string& GetBasePath() const override;
        virtual const u32 TickRate() const override;
        
    private:
        friend Platform;
        SDL2Platform();
        
        void SetupSDLOpenGL();
        
        static std::unique_ptr<Platform> CreatePlatform();
        static std::unique_ptr<Platform> s_platform;
        SDL_Window * moptrWindow;
        SDL_GLContext mglContext;
        bool mRunning;
    };
}

#endif

#endif /* SDL2Platform_hpp */
