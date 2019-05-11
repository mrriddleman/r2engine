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
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/FileStorageArea.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"

namespace
{
    const u32 MAX_NUM_FILES = 1024;
    const u32 MAX_NUM_STORAGE_AREAS = 8;
}

namespace r2
{
    
    
    
    //@NOTE: Increase as needed this is for dev
    const u64 SDL2Platform::MAX_NUM_MEMORY_AREAS = 16;
    
    //@NOTE: Increase as needed this is for dev
    const u64 SDL2Platform::TOTAL_INTERNAL_ENGINE_MEMORY = Megabytes(28);
    
    //@NOTE: Should never exceed the above memory
    const u64 SDL2Platform::TOTAL_INTERNAL_PERMANENT_MEMORY = Megabytes(8);
    
    const u64 SDL2Platform::TOTAL_SCRATCH_MEMORY = Megabytes(4);
    
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

    SDL2Platform::SDL2Platform():moptrWindow(nullptr), mBasePath(nullptr), mPrefPath(nullptr), mRunning(false)
    {
        
    }
    
    bool SDL2Platform::Init(std::unique_ptr<r2::Application> app)
    {
        //@TODO(Serge): add in more subsystems here
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            R2_LOGE("Failed to initialize SDL!");
            return false;
        }

        //Global memory setup for the engine
        {
            r2::mem::GlobalMemory::Init(MAX_NUM_MEMORY_AREAS,
                                        TOTAL_INTERNAL_ENGINE_MEMORY,
                                        TOTAL_INTERNAL_PERMANENT_MEMORY,
                                        TOTAL_SCRATCH_MEMORY);
        }

        //Initialize file system
        {
            mBasePath = SDL_GetBasePath();
            mPrefPath = SDL_GetPrefPath(mEngine.OrganizationName().c_str(), app->GetApplicationName().c_str());
            
            mRootStorage = ALLOC_PARAMS(r2::fs::FileStorageArea, *MEM_ENG_PERMANENT_PTR, mBasePath, MAX_NUM_FILES);
            
            R2_CHECK(mRootStorage != nullptr, "Root Storage was not created!");
            
            mAppStorage = ALLOC_PARAMS(r2::fs::FileStorageArea, *MEM_ENG_PERMANENT_PTR, mPrefPath, MAX_NUM_FILES);
           
            R2_CHECK(mAppStorage != nullptr, "App Storage was not created!");
            
            bool mountResult = mRootStorage->Mount(*MEM_ENG_PERMANENT_PTR);
            
            R2_CHECK(mountResult, "We couldn't mount root storage");
            
            mountResult = mAppStorage->Mount(*MEM_ENG_PERMANENT_PTR);
            
            R2_CHECK(mountResult, "We couldn't mount app storage");
            
            bool fileSystemCreated = r2::fs::FileSystem::Init(*MEM_ENG_PERMANENT_PTR, MAX_NUM_STORAGE_AREAS);
            
            R2_CHECK(fileSystemCreated, "File system was not initialized");
            
            r2::fs::FileSystem::Mount(*mRootStorage);
            r2::fs::FileSystem::Mount(*mAppStorage);

            TestFiles();
        }
        
        //Init OpenGL
        {
            SDL_GL_LoadLibrary(nullptr);
            
            util::Size res = mEngine.GetInitialResolution();
            
            moptrWindow = SDL_CreateWindow("r2engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, res.width, res.height, SetupSDLOpenGL() | SDL_WINDOW_RESIZABLE);
            
            R2_CHECK(moptrWindow != nullptr, "We should have a window pointer!");
            
            mglContext = SDL_GL_CreateContext(moptrWindow);
            
            SDL_assert(mglContext != nullptr);
            
            gladLoadGLLoader(SDL_GL_GetProcAddress);
            SDL_GL_SetSwapInterval(1);
            
            if(moptrWindow && mglContext)
            {
                mRunning = true;
            }
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
            R2_LOGE("Failed to initialize the engine!\n");
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
            
            r2::mem::GlobalMemory::EngineMemory().singleFrameArena->EnsureZeroAllocations();
            
            r2::mem::GlobalMemory::EngineMemory().singleFrameArena->GetPolicyRef().Reset();
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
        
        r2::fs::FileSystem::Shutdown(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        
        mAppStorage->Unmount(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        mRootStorage->Unmount(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        
        FREE(mAppStorage, *r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        FREE(mRootStorage, *r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        
        if (mPrefPath)
        {
            SDL_free(mPrefPath);
        }
        
        if (mBasePath)
        {
            SDL_free(mBasePath);
        }
        
        r2::mem::GlobalMemory::Shutdown();
        
        SDL_Quit();
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
    
    const std::string SDL2Platform::RootPath() const
    {
        return mBasePath;
    }
    
    const std::string SDL2Platform::AppPath() const
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
    
    void SDL2Platform::TestFiles()
    {
        char filePath[Kilobytes(1)];
        char filePath2[Kilobytes(1)];
        char filePath3[Kilobytes(1)];
        
        strcpy(filePath, mPrefPath);
        strcpy(filePath2, mPrefPath);
        strcpy(filePath3, mPrefPath);
        
        strcat(filePath, "test_pref_path.txt");
        
        R2_LOGI("filePath: %s\n", filePath);
        
        char textToWrite[] = "This is some text that I am writing!";
        
        r2::fs::FileMode mode;
        mode |= r2::fs::Mode::Write;
        
        r2::fs::File* fileToWrite = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
        
        R2_CHECK(fileToWrite != nullptr, "We don't have a proper file!");
        
        u64 numBytesWritten = fileToWrite->Write(textToWrite, strlen(textToWrite));
        
        R2_LOGI("I wrote %llu bytes\n", numBytesWritten);
        
        r2::fs::FileSystem::Close(fileToWrite);
        
        mode = r2::fs::Mode::Read;
        
        r2::fs::File* fileToRead = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
        
        u64 fileSize = fileToRead->Size();
        
        R2_LOGI("file size: %llu\n", fileSize);
        
        char textToRead[Kilobytes(1)];
        
        fileToRead->Skip(5);
        
        u64 bytesRead = fileToRead->Read(textToRead, 2);
        textToRead[2] = '\0';
        
        R2_CHECK(bytesRead == 2, "We didn't read all of the file!");
        
        R2_LOGI("Here's what I read:\n%s\n", textToRead);
        
        r2::fs::FileSystem::Close(fileToRead);
        
        //Test file exists, file delete, file rename, and file copy
        
        R2_CHECK(r2::fs::FileSystem::FileExists(filePath), "This file should exist!");
        
        R2_CHECK(!r2::fs::FileSystem::FileExists("some_fake_file.txt"), "This file should not exist!");
        
        strcat(filePath2, "copy_of_file.txt");
        
        
        R2_CHECK(r2::fs::FileSystem::CopyFile(filePath, filePath2), "We should be able to copy files!");
        
        R2_CHECK(r2::fs::FileSystem::FileExists(filePath2), "filePath2 should exist!");
        
        strcat(filePath3, "renamed_file.txt");
        
        R2_CHECK(r2::fs::FileSystem::RenameFile(filePath2, filePath3), "We should be able to rename a file");
        
        R2_CHECK(r2::fs::FileSystem::FileExists(filePath3), "filePath3 should exist");
        R2_CHECK(!r2::fs::FileSystem::FileExists(filePath2), "filePath2 should not exist");
        
        R2_CHECK(r2::fs::FileSystem::DeleteFile(filePath3), "FilePath3 should be deleted");
        
        
        r2::fs::DeviceConfig safeConfig;
        safeConfig.AddModifier(r2::fs::DeviceModifier::Safe);
        
        r2::fs::File* safeFile = r2::fs::FileSystem::Open(safeConfig, filePath, mode);
        
        r2::fs::FileSystem::Close(safeFile);
        
        /* @TODO(Serge): TOO PROBLEMATIC right now enable some day
        r2::fs::DiskFile* asyncFile = (r2::fs::DiskFile*)r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
        
        strcpy(textToRead, "");
        
        R2_LOGI("textToRead: %s\n", textToRead);
        r2::fs::DiskFileAsyncOperation op = asyncFile->ReadAsync(textToRead, asyncFile->Size(), 0);
        op.WaitUntilFinished(0);
        textToRead[asyncFile->Size()] = '\0';
        R2_LOGI("textToRead: %s", textToRead);
        
        */
        
        
    }
}




#endif
