//
//  SDL2Platform.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#include "SDL2Platform.h"
#include <SDL2/SDL.h>
#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

namespace r2
{
    static std::unique_ptr<Platform> s_platform = nullptr;
    
    
    std::unique_ptr<Platform> CreatePlatform()
    {
        return std::make_unique<SDL2Platform>();
    }
    
    const r2::Platform& Platform::GetConst()
    {
        if(!s_platform)
        {
            s_platform = CreatePlatform();
        }
        return *s_platform;
    }
    
    r2::Platform& Platform::Get()
    {
        if(!s_platform)
        {
            s_platform = CreatePlatform();
        }
        
        return *s_platform;
    }

    SDL2Platform::SDL2Platform():moptrWindow(nullptr), mRunning(false)
    {
        
    }
    
    bool SDL2Platform::Init(std::unique_ptr<r2::Application> app)
    {
        Platform::Init(std::move(app));
        
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            //TODO(Serge): add logging for error
            return false;
        }
        
        if(!mApplication->Init())
        {
            //TODO(Serge): add logging for error
            return false;
        }
        
        Resolution res = mApplication->GetInitialResolution();
        
        moptrWindow = SDL_CreateWindow(mApplication->GetApplicationName().c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, res.width, res.height, 0);
        
        if(moptrWindow)
        {
            mRunning = true;
        }
        
        return moptrWindow != nullptr;
    }
    
    void SDL2Platform::Run()
    {
        u32 currentTime = SDL_GetTicks();
        u32 accumulator = 0;
        
       // u32 lastTime = currentTime;
        u32 t = 0;
        u32 dt = TickRate();
        
        while (mRunning)
        {
            SDL_Event e;
            
            while (SDL_PollEvent(&e))
            {
                switch (e.type)
                {
                    case SDL_QUIT:
                        mRunning = false;
                        break;
                        
                    default:
                        break;
                }
            }
            
            u32 newTime = SDL_GetTicks();
            u32 delta = newTime - currentTime;
            currentTime = newTime;
            
            if (delta > 300)
            {
                delta = 300;
            }
            
            accumulator += delta;
            
            while (accumulator >= dt)
            {
                mApplication->Update();
                t+= dt;
                accumulator -= dt;
            }
        }
    }

    void SDL2Platform::Shutdown()
    {
        mApplication->Shutdown();
        
        if (moptrWindow)
        {
            SDL_DestroyWindow(moptrWindow);
        }
        
        SDL_Quit();
    }
    
    const std::string& SDL2Platform::GetBasePath() const
    {
        static std::string basePath = SDL_GetBasePath();
        return basePath;
    }
    
    u32 SDL2Platform::TickRate() const
    {
        return 1;
    }
}


#endif
