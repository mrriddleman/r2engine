//
//  SDL2Platform.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//


#include "r2pch.h"
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
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"
#include "r2/Utils/Handle.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"

#include "r2/Render/Renderer/RendererImpl.h"

namespace
{
    const u32 MAX_NUM_FILES = 1024;
    const u32 MAX_NUM_STORAGE_AREAS = 8;
    
    r2::mem::MallocArena zipArena{r2::mem::utils::MemBoundary()};
    
    void* AllocZip(u64 size, u64 align)
    {
        return ALLOC_BYTESN(zipArena, size, align);
    }
    
    void FreeZip(byte* ptr)
    {
        FREE(ptr, zipArena);
    }
}

namespace r2
{
    //@NOTE: Increase as needed this is for dev
    const u64 SDL2Platform::MAX_NUM_MEMORY_AREAS = 16;
    
    //@NOTE: Increase as needed dev
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

    SDL2Platform::SDL2Platform()
        : mRootStorage(nullptr)
        , mAppStorage(nullptr)
        , mRunning(false)
    {
        r2::util::PathCpy( mSoundDefinitionPath, "" );
    }
    
    bool SDL2Platform::Init(std::unique_ptr<r2::Application> app)
    {
        //@TODO(Serge): add in more subsystems here
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            R2_LOGE("Failed to initialize SDL!");
            return false;
        }
        
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
        
        //Global memory setup for the engine
        {
            r2::mem::GlobalMemory::Init(MAX_NUM_MEMORY_AREAS,
                                        TOTAL_INTERNAL_ENGINE_MEMORY,
                                        TOTAL_INTERNAL_PERMANENT_MEMORY,
                                        TOTAL_SCRATCH_MEMORY);
        }

        //Initialize file system
        {
            const char* basePath = SDL_GetBasePath();
            const char* prefPath = SDL_GetPrefPath(mEngine.OrganizationName().c_str(), app->GetApplicationName().c_str());
            
            r2::fs::utils::SanitizeSubPath(basePath, mBasePath);
            r2::fs::utils::SanitizeSubPath(prefPath, mPrefPath);

            r2::util::PathCpy(mSoundDefinitionPath, app->GetSoundDefinitionPath().c_str());
            
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

            mAssetPathResolver = app->GetPathResolver();
            

            
            //TestFiles();
        }
        
        //Init Platform Renderer
        {
            r2::draw::rendererimpl::PlatformRendererSetupParams setupParams;
            setupParams.flags |= r2::draw::rendererimpl::VSYNC;
            setupParams.windowName = "r2Engine";
            setupParams.resolution = mEngine.GetInitialResolution();
            setupParams.platformFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

            if(r2::draw::rendererimpl::PlatformInit(setupParams))
            {
                mRunning = true;
            }

            R2_CHECK(mRunning, "Why aren't we running?");
        }

        //@NOTE: maybe it's a bad idea to get the initial resolution without initializing the engine first?
        
        //Setup engine - bad that it's being set before initialization?
        {
            mEngine.SetVSyncCallback([](bool vsync){
                return r2::draw::rendererimpl::SetVSYNC(vsync);
            });
            
            mEngine.SetFullscreenCallback([this](u32 flags){
                return r2::draw::rendererimpl::SetFullscreen(flags);
            });
            
            mEngine.SetScreenSizeCallback([this](s32 width, s32 height){
                r2::draw::rendererimpl::SetWindowSize(width, height);
            });
            
            mEngine.SetGetPerformanceFrequencyCallback([]{
                static u64 frequency = SDL_GetPerformanceFrequency();
                return frequency;
            });
            
            mEngine.SetGetPerformanceCounterCallback([]{
                return SDL_GetPerformanceCounter();
            });
            
            mEngine.SetGetTicksCallback([]{
                return SDL_GetTicks();
            });
           
            mEngine.mSetClipboardTextFunc = SDL2SetClipboardTextFunc;
            mEngine.mGetClipboardTextFunc = SDL2GetClipboardTextFunc;
            
            //Game Controller stuff
            mEngine.SetOpenGameControllerCallback([](r2::io::ControllerID controllerID){
                SDL_GameController* controller = nullptr;
                
                if(SDL_IsGameController(controllerID))
                {
                    controller = SDL_GameControllerOpen(controllerID);
                }
                
                return controller;
            });
            
            mEngine.SetNumberOfGameControllersCallback([](){
                return SDL_NumJoysticks();
            });
            
            mEngine.SetIsGameControllerCallback([](r2::io::ControllerID controllerID){
                return SDL_IsGameController(controllerID);
            });
            
            mEngine.SetCloseGameControllerCallback([](void* controller){
                SDL_GameControllerClose((SDL_GameController*)controller);
            });
            
            mEngine.SetIsGameControllerAttchedCallback([](void* gameController){
                return SDL_GameControllerGetAttached((SDL_GameController*)gameController);
            });
            
            mEngine.SetGetGameControllerMappingCallback([](void* gameController){
                return SDL_GameControllerMapping((SDL_GameController*)gameController);
            });
            
            mEngine.SetGetGameControllerButtonStateCallback([](void* gameController, r2::io::ControllerButtonName buttonName){
                return SDL_GameControllerGetButton((SDL_GameController*)gameController, (SDL_GameControllerButton)buttonName);
            });
            
            mEngine.SetGetGameControllerAxisValueCallback([](void* gameController, r2::io::ControllerAxisName axisName){
                return SDL_GameControllerGetAxis((SDL_GameController*)gameController, (SDL_GameControllerAxis)axisName);
            });
            
            mEngine.SetGetStringForButtonCallback([](r2::io::ControllerButtonName name){
                return SDL_GameControllerGetStringForButton((SDL_GameControllerButton)name);
            });
            
            mEngine.SetGetStringForAxisCallback([](r2::io::ControllerAxisName name){
                return SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)name);
            });
            
            mEngine.SetGetGameControllerNameFunc([](void* gameController){
                return SDL_GameControllerName((SDL_GameController*)gameController);
            });
        }
        
        if(!mEngine.Init(std::move(app)))
        {
            R2_LOGE("Failed to initialize the engine!\n");
            return false;
        }

        return mRunning;
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
                        switch (e.window.event)
                        {
                            case SDL_WINDOWEVENT_RESIZED:
                                mEngine.WindowResizedEvent(e.window.data1, e.window.data2);
                                break;
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                                mEngine.WindowSizeChangedEvent(e.window.data1, e.window.data2);
                                break;
                            case SDL_WINDOWEVENT_MINIMIZED:
                                mEngine.WindowMinimizedEvent();
                                break;
                            case SDL_WINDOWEVENT_EXPOSED:
                                mEngine.WindowUnMinimizedEvent();
                                break;
                            case SDL_WINDOWEVENT_MAXIMIZED:
                                
                                break;
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
                            keyData.modifiers |= io::Key::SHIFT_PRESSED_KEY;
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
                        
                    case SDL_CONTROLLERDEVICEADDED:
                    {
                        mEngine.ControllerDetectedEvent(e.cdevice.which);
                    }
                    break;
                    case SDL_CONTROLLERDEVICEREMOVED:
                    {
                        mEngine.ControllerDisonnectedEvent(e.cdevice.which);
                    }
                    break;
                    case SDL_CONTROLLERDEVICEREMAPPED:
                    {
                        mEngine.ControllerRemappedEvent(e.cdevice.which);
                    }
                    break;
                    case SDL_CONTROLLERBUTTONUP:
                    case SDL_CONTROLLERBUTTONDOWN:
                    {
                        mEngine.ControllerButtonEvent(e.cbutton.which, (r2::io::ControllerButtonName)e.cbutton.button, u8(e.cbutton.state == SDL_PRESSED));
                    }
                    break;
                    case SDL_CONTROLLERAXISMOTION:
                    {
                        mEngine.ControllerAxisEvent(e.caxis.which, (r2::io::ControllerAxisName)e.caxis.axis, e.caxis.value);
                    }
                    break;
                    default:
                        break;
                }
            }
            
            u32 newTime = SDL_GetTicks();
            u32 delta = newTime - currentTime;
            currentTime = newTime;
       //     printf("delta: %u\n", delta);

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
            
            r2::draw::rendererimpl::SwapScreens();
            
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
        
        r2::draw::rendererimpl::Shutdown();
        
        r2::fs::FileSystem::Shutdown(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        
        mAppStorage->Unmount(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        mRootStorage->Unmount(*r2::mem::GlobalMemory::EngineMemory().permanentStorageArena);
        
        FREE(mAppStorage, *MEM_ENG_PERMANENT_PTR);
        FREE(mRootStorage, *MEM_ENG_PERMANENT_PTR);
        
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
        return 10;
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
    
    const std::string SDL2Platform::SoundDefinitionsPath() const
    {
        return mSoundDefinitionPath;
    }
    
    //--------------------------------------------------
    //                  Private
    //--------------------------------------------------
    void SDL2Platform::TestFiles()
    {
//        char filePath[r2::fs::FILE_PATH_LENGTH];
//        char filePath2[r2::fs::FILE_PATH_LENGTH];
//        char filePath3[r2::fs::FILE_PATH_LENGTH];
//
//
//        strcpy(filePath, mPrefPath);
//        strcpy(filePath2, mPrefPath);
//        strcpy(filePath3, mPrefPath);
//
//
//        strcat(filePath, "test_pref_path.txt");
//
//        R2_LOGI("filePath: %s\n", filePath);
//
//        char textToWrite[] = "This is some text that I am writing!";
//
//        r2::fs::FileMode mode;
//        mode |= r2::fs::Mode::Write;
//
//        r2::fs::File* fileToWrite = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
//
//        R2_CHECK(fileToWrite != nullptr, "We don't have a proper file!");
//
//        u64 numBytesWritten = fileToWrite->Write(textToWrite, strlen(textToWrite));
//
//        R2_LOGI("I wrote %llu bytes\n", numBytesWritten);
//
//        r2::fs::FileSystem::Close(fileToWrite);
//
//        mode = r2::fs::Mode::Read;
//
//        r2::fs::File* fileToRead = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
//
//        u64 fileSize = fileToRead->Size();
//
//        R2_LOGI("file size: %llu\n", fileSize);
//
//        char textToRead[r2::fs::FILE_PATH_LENGTH];
//
//        fileToRead->Skip(5);
//
//        u64 bytesRead = fileToRead->Read(textToRead, 2);
//        textToRead[2] = '\0';
//
//        R2_CHECK(bytesRead == 2, "We didn't read all of the file!");
//
//        R2_LOGI("Here's what I read:\n%s\n", textToRead);
//
//        r2::fs::FileSystem::Close(fileToRead);
//
//        //Test file exists, file delete, file rename, and file copy
//
//        R2_CHECK(r2::fs::FileSystem::FileExists(filePath), "This file should exist!");
//
//        R2_CHECK(!r2::fs::FileSystem::FileExists("some_fake_file.txt"), "This file should not exist!");
//
//        strcat(filePath2, "copy_of_file.txt");
//
//
//        R2_CHECK(r2::fs::FileSystem::CopyFile(filePath, filePath2), "We should be able to copy files!");
//
//        R2_CHECK(r2::fs::FileSystem::FileExists(filePath2), "filePath2 should exist!");
//
//        strcat(filePath3, "renamed_file.txt");
//
//        R2_CHECK(r2::fs::FileSystem::RenameFile(filePath2, filePath3), "We should be able to rename a file");
//
//        R2_CHECK(r2::fs::FileSystem::FileExists(filePath3), "filePath3 should exist");
//
//        R2_CHECK(!r2::fs::FileSystem::FileExists(filePath2), "filePath2 should not exist");
//
//        R2_CHECK(r2::fs::FileSystem::DeleteFile(filePath3), "FilePath3 should be deleted");
//
//
//        r2::fs::DeviceConfig safeConfig;
//        safeConfig.AddModifier(r2::fs::DeviceModifier::Safe);
//
//        r2::fs::File* safeFile = r2::fs::FileSystem::Open(safeConfig, filePath, mode);
//
//        r2::fs::FileSystem::Close(safeFile);
        
        //Zipity do da
        
//        char filePath4[r2::fs::FILE_PATH_LENGTH];
//        
//        r2::fs::utils::AppendSubPath(mBasePath, filePath4, "Archive.zip");
//        
//        r2::fs::DeviceConfig zipConfig;
//        zipConfig.AddModifier(r2::fs::DeviceModifier::Zip);
//        
//        r2::fs::ZipFile* zipFile = (r2::fs::ZipFile*)r2::fs::FileSystem::Open(zipConfig, filePath4, r2::fs::Mode::Read | r2::fs::Mode::Binary);
//        
//        R2_CHECK(zipFile != nullptr, "We have a zip file");
//        R2_CHECK(!zipFile->IsOpen(), "Zip file shouldn't be open");
//        
//        
//        zipFile->InitArchive(AllocZip, FreeZip);
//        
//        const auto numFiles = zipFile->GetNumberOfFiles();
//        
//        for (u64 i = 0; i < numFiles; ++i)
//        {
//            r2::fs::ZipFile::ZipFileInfo info;
//            bool foundExtra = false;
//            
//            if (zipFile->GetFileInfo(i, info, &foundExtra))
//            {
//                printf("Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u, Is Dir: %u\n", info.filename, info.comment, (uint)info.uncompressedSize, (uint)info.compressedSize, info.isDirectory);
//            }
//        }
//        
//        r2::fs::FileSystem::Close(zipFile);
        
        /* @TODO(Serge): fix async files... again...
        r2::fs::DiskFile* asyncFile = (r2::fs::DiskFile*)r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath, mode);
        
       // char filePath4[Kilobytes(1)];
       // strcpy(filePath4, mPrefPath);
      //  strcat(filePath4, "test_async_write.txt");
        
        
//        r2::fs::FileMode asyncWriteMode;
//        asyncWriteMode |= r2::fs::Mode::Write;
//
//        r2::fs::DiskFile* asyncFile2 = (r2::fs::DiskFile*)r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), filePath4, asyncWriteMode);
//
//        char textToRead2[Kilobytes(1)];
//        strcpy(textToRead, "");
//        strcpy(textToRead2, "");
        
        r2::fs::DiskFileAsyncOperation op = asyncFile->ReadAsync(textToRead, asyncFile->Size(), 0);
       // r2::fs::DiskFileAsyncOperation op2 = asyncFile->ReadAsync(textToRead2, asyncFile->Size(), 0);
        
        
        
        op.WaitUntilFinished(1);
       // op2.WaitUntilFinished(1);
        textToRead[asyncFile->Size()] = '\0';
        //textToRead2[asyncFile->Size()] = '\0';
        
        
       // r2::fs::DiskFileAsyncOperation op3 = asyncFile2->WriteAsync(textToRead2, asyncFile->Size(), 0);
        
      //  op3.WaitUntilFinished(1);
        
        
        
        R2_LOGI("textToRead: %s", textToRead);
       // R2_LOGI("textToRead2: %s", textToRead2);
        
        R2_LOGI("temp");
        
        r2::fs::FileSystem::Close(asyncFile);
       // r2::fs::FileSystem::Close(asyncFile2);
        */
    }
}




#endif
