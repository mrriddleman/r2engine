//
//  SDL2Platform.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)
#include "SDL2Platform.h"
#include "r2/Platform/IO.h"
#include "r2/Core/Engine.h"
#include "glad/glad.h"

namespace r2
{
    static char * mClipboardTextData = nullptr;
    
    std::unique_ptr<Platform> SDL2Platform::s_platform = nullptr;
    
    void SDL2SetClipboardTextFunc(void*, const char* text)
    {
        SDL_SetClipboardText(text);
    }
    
    const char* SDL2GetClipboardTextFunc(void*)
    {
        if(mClipboardTextData)
        {
            SDL_free(mClipboardTextData);
        }
        
        mClipboardTextData = SDL_GetClipboardText();
        return mClipboardTextData;
    }
    
    
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
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            //@TODO(Serge): add logging for error
            return false;
        }
        
        mPrefPath = SDL_GetPrefPath(mEngine.OrganizationName().c_str(), app->GetApplicationName().c_str());
        
        SDL_GL_LoadLibrary(nullptr);
        
        util::Size res = mEngine.GetInitialResolution();
        
        moptrWindow = SDL_CreateWindow("r2engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, res.width, res.height, SetupSDLOpenGL() | SDL_WINDOW_RESIZABLE);
        
        SDL_assert(moptrWindow != nullptr);
        
        mglContext = SDL_GL_CreateContext(moptrWindow);
        
        SDL_assert(mglContext != nullptr);
    
        gladLoadGLLoader(SDL_GL_GetProcAddress);
        SDL_GL_SetSwapInterval(1);
        
        if(moptrWindow && mglContext)
        {
            mRunning = true;
        }
        
        //@NOTE: maybe it's a bad idea to get the initial resolution without initializing the ngine first?
        
        //Setup engine - bad that it's being set before initialization?
        {
            mEngine.SetVSyncCallback([](bool vsync){
                return SDL_GL_SetSwapInterval(static_cast<int>(vsync));
            });
            
            mEngine.SetFullscreenCallback([this](u32 flags){
                return SDL_SetWindowFullscreen(moptrWindow, flags);
            });
            
            mEngine.SetScreenSizeCallback([this](s32 width, s32 height){
                SDL_SetWindowSize(moptrWindow, width, height);
            });
            
            mEngine.SetGetPerformanceFrequencyCallback([]{
                static u64 frequency = SDL_GetPerformanceFrequency();
                return frequency;
            });
            
            mEngine.SetGetPerformanceCounterCallback([]{
                return SDL_GetPerformanceCounter();
            });
            
            mEngine.mSetClipboardTextFunc = SDL2SetClipboardTextFunc;
            mEngine.mGetClipboardTextFunc = SDL2GetClipboardTextFunc;
        }
        
        if(!mEngine.Init(std::move(app)))
        {
            //@TODO(Serge): add logging for error
            return false;
        }

        return moptrWindow != nullptr && mglContext != nullptr;
    }
    
    void SDL2Platform::Run()
    {
        u32 currentTime = SDL_GetTicks();
        u32 accumulator = 0;
        
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
                        mEngine.QuitTriggered();
                        break;
                    case SDL_WINDOWEVENT:
                        switch (e.window.type)
                        {
                            case SDL_WINDOWEVENT_RESIZED:
                                mEngine.WindowResizedEvent(e.window.data1, e.window.data2);
                                break;
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                                mEngine.WindowSizeChangedEvent(e.window.data1, e.window.data2);
                            default:
                                break;
                        }
                        break;
                    case SDL_MOUSEBUTTONUP:
                    case SDL_MOUSEBUTTONDOWN:
                    {
                        r2::io::MouseData mouseData;
                        
                        mouseData.state = e.button.state;
                        mouseData.numClicks = e.button.clicks;
                        mouseData.button = e.button.button;
                        mouseData.x = e.button.x;
                        mouseData.y = e.button.y;
                        
                        mEngine.MouseButtonEvent(mouseData);
                    }
                        break;
                        
                    case SDL_MOUSEMOTION:
                    {
                        r2::io::MouseData mouseData;
                        
                        mouseData.x = e.motion.x;
                        mouseData.y = e.motion.y;
                        mouseData.xrel = e.motion.xrel;
                        mouseData.yrel = e.motion.yrel;
                        mouseData.state = e.motion.state;
                        
                        mEngine.MouseMovedEvent(mouseData);
                    }
                        break;
                        
                    case SDL_MOUSEWHEEL:
                    {
                        r2::io::MouseData mouseData;
                        
                        mouseData.direction = e.wheel.direction;
                        mouseData.x = e.wheel.x;
                        mouseData.y = e.wheel.y;
                        
                        mEngine.MouseWheelEvent(mouseData);
                    }
                        break;
                        
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                    {
                        r2::io::Key keyData;
                        
                        keyData.state = e.key.state;
                        keyData.repeated = e.key.repeat;
                        keyData.code = e.key.keysym.scancode;
                        
                        //@NOTE: Right now we make no distinction between left or right versions of these keys
                        if(e.key.keysym.mod & KMOD_ALT)
                        {
                            keyData.modifiers |= io::Key::ALT_PRESSED;
                        }
                        
                        if(e.key.keysym.mod & KMOD_SHIFT)
                        {
                            keyData.modifiers |= io::Key::SHIFT_PRESSED;
                        }
                        
                        if(e.key.keysym.mod & KMOD_CTRL)
                        {
                            keyData.modifiers |= io::Key::CONTROL_PRESSED;
                        }
                        
                        mEngine.KeyEvent(keyData);
                    }
                        break;
                        
                    case SDL_TEXTINPUT:
                    {
                        mEngine.TextEvent(e.text.text);
                    }
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
                mEngine.Update();
                t+= dt;
                accumulator -= dt;
            }
            
            float alpha = static_cast<float>(accumulator) / static_cast<float>(dt);
            mEngine.Render(alpha);
            
            SDL_GL_SwapWindow(moptrWindow);
        }
    }

    void SDL2Platform::Shutdown()
    {
        mEngine.Shutdown();
        
        if(mClipboardTextData)
        {
            SDL_free(mClipboardTextData);
        }
        
        mClipboardTextData = nullptr;
        
        SDL_GL_DeleteContext(mglContext);
        
        if (moptrWindow)
        {
            SDL_DestroyWindow(moptrWindow);
        }
        
        SDL_Quit();
    }
    
    const std::string& SDL2Platform::RootPath() const
    {
        static std::string basePath = SDL_GetBasePath();
        return basePath;
    }
    
    const u32 SDL2Platform::TickRate() const
    {
        return 1;
    }
    
    const s32 SDL2Platform::NumLogicalCPUCores() const
    {
        return SDL_GetCPUCount();
    }
    
    const s32 SDL2Platform::SystemRAM() const
    {
        return SDL_GetSystemRAM();
    }
    
    const s32 SDL2Platform::CPUCacheLineSize() const
    {
        return SDL_GetCPUCacheLineSize();
    }
    
    const std::string& SDL2Platform::AppPath() const
    {
        return mPrefPath;
    }
    
    //--------------------------------------------------
    //                  Private
    //--------------------------------------------------
    
    int SDL2Platform::SetupSDLOpenGL()
    {
        //TODO(Serge): should be in the renderer or something, Also should be system dependent
        
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        
        return SDL_WINDOW_OPENGL;
    }
}




#endif
