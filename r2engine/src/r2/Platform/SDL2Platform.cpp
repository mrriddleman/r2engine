//
//  SDL2Platform.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)
#include "SDL2Platform.h"
#include "glad/glad.h"
#include "glm/glm.hpp"

namespace r2
{
    std::unique_ptr<Platform> SDL2Platform::s_platform = nullptr;
    
    std::unique_ptr<Platform> SDL2Platform::CreatePlatform()
    {
        auto platform = new SDL2Platform();
        std::unique_ptr<Platform> ptr;
        ptr.reset(platform);
        
        return ptr;
    }
    
    const r2::Platform& Platform::GetConst()
    {
        if(!SDL2Platform::s_platform)
        {
            SDL2Platform::s_platform = SDL2Platform::CreatePlatform();
        }
        return *SDL2Platform::s_platform;
    }
    
    r2::Platform& Platform::Get()
    {
        if(!SDL2Platform::s_platform)
        {
            SDL2Platform::s_platform = SDL2Platform::CreatePlatform();
        }
        
        return *SDL2Platform::s_platform;
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
        
        SDL_GL_LoadLibrary(nullptr);
        
        if(!mApplication->Init())
        {
            //TODO(Serge): add logging for error
            return false;
        }
        
        utils::Size res = mApplication->GetInitialResolution();
        
        moptrWindow = SDL_CreateWindow(mApplication->GetApplicationName().c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, res.width, res.height, SDL_WINDOW_OPENGL);
        
        SDL_assert(moptrWindow != nullptr);
        
        mglContext = SDL_GL_CreateContext(moptrWindow);
        
        SDL_assert(mglContext != nullptr);
        
        
        gladLoadGLLoader(SDL_GL_GetProcAddress);
        
        
        if(moptrWindow && mglContext)
        {
            mRunning = true;
        }
        
        int w,h;
        SDL_GetWindowSize(moptrWindow, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.5f, 1.0f, 0.0f);
        
        return moptrWindow != nullptr && mglContext != nullptr;
    }
    
    void SDL2Platform::Run()
    {
        u32 currentTime = SDL_GetTicks();
        u32 accumulator = 0;
        
       // u32 lastTime = currentTime;
        u32 t = 0;
        const u32 dt = TickRate();
        
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
            
            
            glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            SDL_GL_SwapWindow(moptrWindow);
        }
    }

    void SDL2Platform::Shutdown()
    {
        mApplication->Shutdown();
        
        SDL_GL_DeleteContext(mglContext);
        
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
    
    const u32 SDL2Platform::TickRate() const
    {
        return 1;
    }
    
    //--------------------------------------------------
    //                  Private
    //--------------------------------------------------
    
    void SDL2Platform::SetupSDLOpenGL()
    {
        //TODO(Serge): should be in the renderer or something, Also should be system dependent
        
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    }
}




#endif
