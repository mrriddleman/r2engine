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
#include "r2/Core/Math/MathUtils.h"
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
#include "SDL_image.h"
#include "r2/Core/Layer/EditorLayer.h"
#include "r2/Core/Application.h"
#ifdef R2_IMGUI
#include "backends/imgui_impl_sdl2.h"
#endif
#include "r2/Utils/Timer.h"

namespace
{
    const u32 MAX_NUM_FILES = 1024;
    const u32 MAX_NUM_STORAGE_AREAS = 8;
    const f64 k_millisecondsToSeconds = 1000.;
}

namespace r2
{
	const s32 FULL_SCREEN_WINDOW = SDL_WINDOW_FULLSCREEN;
	const s32 FULL_SCREEN_DESKTOP= SDL_WINDOW_FULLSCREEN_DESKTOP;

    //@NOTE: Increase as needed this is for dev
    const u64 SDL2Platform::MAX_NUM_MEMORY_AREAS = 16;
    
    //@NOTE: Increase as needed for dev
#ifdef R2_DEBUG
    const u64 SDL2Platform::TOTAL_INTERNAL_ENGINE_MEMORY = Megabytes(400);
#elif R2_RELEASE || R2_PUBLISH
    const u64 SDL2Platform::TOTAL_INTERNAL_ENGINE_MEMORY = Megabytes(256);
#endif
    //@NOTE: Should never exceed the above memory
    const u64 SDL2Platform::TOTAL_INTERNAL_PERMANENT_MEMORY = Megabytes(8);
    
    const u64 SDL2Platform::TOTAL_SCRATCH_MEMORY = Megabytes(32);
    
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
        
		int flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
		int initted = IMG_Init(flags);
        if ((initted & flags) != flags) {
        
            printf("IMG_Init: %s\n", IMG_GetError());
            R2_LOGE("Failed to initialize SDL_image!");
            return false;
        }

        //Global memory setup for the engine
        {
            u32 maxNumMemoryAreas = MAX_NUM_MEMORY_AREAS;
#ifdef R2_EDITOR
            //@NOTE: need one more for the editor
            maxNumMemoryAreas++;
#endif

            r2::mem::GlobalMemory::Init(maxNumMemoryAreas,
                                        TOTAL_INTERNAL_ENGINE_MEMORY,
                                        TOTAL_INTERNAL_PERMANENT_MEMORY,
                                        TOTAL_SCRATCH_MEMORY);
        }

        //Initialize file system
        {
            const char* basePath = SDL_GetBasePath();
            const char* prefPath = SDL_GetPrefPath(mEngine.OrganizationName().c_str(), app->GetApplicationName());
            
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
          //  setupParams.flags |= r2::draw::rendererimpl::VSYNC;

            r2::util::PathCpy(mApplicationName, app->GetApplicationName());

            setupParams.windowName = mApplicationName;

            //@TODO(Serge): maybe the platform should enable a way to read a settings file for the app so we know the proper resolution. 
            //              This is so that we don't have to init the app before we can know the app's current resolution
            setupParams.resolution = app->GetAppResolution(); 
            int resizableFlag = app->IsWindowResizable() ? SDL_WINDOW_RESIZABLE : 0;
            int maximized = app->WindowShouldBeMaximized() ? SDL_WINDOW_MAXIMIZED : 0;
            setupParams.platformFlags = SDL_WINDOW_OPENGL | resizableFlag | maximized;
          //  setupParams.flags.Set(draw::rendererimpl::VSYNC);
            
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

            mEngine.SetWindowPositionCallback([this](s32 xPos, s32 yPos) {
                r2::draw::rendererimpl::SetWindowPosition(xPos, yPos);
            });

            mEngine.CenterWindowCallback([this]() {
                r2::draw::rendererimpl::CenterWindow();
            });

            
            mEngine.SetGetPerformanceFrequencyCallback([]{
                static u64 frequency = SDL_GetPerformanceFrequency();
                return frequency;
            });
            
            mEngine.SetGetPerformanceCounterCallback([]{
                return SDL_GetPerformanceCounter();
            });
            
            mEngine.SetGetTicksCallback([this]{
                return  (static_cast<f64>(SDL_GetPerformanceCounter() - mStartTime)  / (f64)SDL_GetPerformanceFrequency()) * k_millisecondsToSeconds;
            });
           
            mEngine.mSetClipboardTextFunc = SDL2SetClipboardTextFunc;
            mEngine.mGetClipboardTextFunc = SDL2GetClipboardTextFunc;
        }
        
        if(!mEngine.Init(std::move(app)))
        {
            R2_LOGE("Failed to initialize the engine!\n");
            return false;
        }

        //Now detect any already attached game controllers - do this after we init the Engine since we want it to be setup before we do this

        auto numGameControllers = SDL2GameController::GetNumberOfConnectedGameControllers();

        /*for (u32 i = 0; i < numGameControllers; ++i)
        {
            if (SDL2GameController::IsGameController(i))
            {
                bool success = mSDLGameControllers[i].Open(i);

                R2_CHECK(success, "Failed to open the controller: %u!", i);

                mEngine.GameControllerOpened(mSDLGameControllers[i].GetControllerID(), mSDLGameControllers[i].GetControllerInstanceID());
            }
        }*/

        return mRunning;
    }
    
    void SDL2Platform::Run()
    {
        char newTitle[2048];

        mStartTime = SDL_GetPerformanceCounter();
        u64 currentTime = mStartTime;
        f64 accumulator = 0;
        
        f64 maxMSPerFrame = 0;
        f64 minMSPerFrame = std::numeric_limits<double>::max();
		
        const u64 k_millisecondsForFPSUpdate = 100;
        const u64 k_framesForFPSUpdate = 5;
        const u64 k_frequency = SDL_GetPerformanceFrequency();
        u64 dtUpper = k_frequency / 62; //(1.0 / 61.0) * k_millisecondsToSeconds;
        u64 dtLower = k_frequency / 60;//(1.0 / 59.0) * k_millisecondsToSeconds;

        u64 frames = 0;
        u64 startTime = currentTime;
        u64 endTime = startTime;

        u64 t = 0;
        f64 k_desiredUpdateRate = TickRate();//0.016666666 * 1000.0;
        const f64 dt = k_desiredUpdateRate;
        mResync = false;

		const u64 k_timeHistoryCount = 4;
		u64 timeAverager[k_timeHistoryCount] = { dt, dt, dt, dt };

        const f64 k_dtUpperToleranceMult = 8.0;

		const f64 snap_frequencies[] = { dt,        //60fps
                              dt * 2.0,      //30fps
                              dt * 3.0,      //20fps
                              dt * 4.0,      //15fps
							  (dt + 1.0) / 2.0,  //120fps //120hz, 240hz, or higher need to round up, so that adding 120hz twice guaranteed is at least the same as adding time_60hz once
						   // (time_60hz+2)/3,  //180fps //that's where the +1 and +2 come from in those equations
						   // (time_60hz+3)/4,  //240fps //I do not want to snap to anything higher than 120 in my engine, but I left the math in here anyway
		};

        
        const f64 vsync_maxerror = dt * .002;
        const u32 k_ignoreFrames = 1000;

        u64 totalFrames = 0;

        auto tempCurrent = SDL_GetPerformanceCounter();
        auto tempLast = tempCurrent;
        
        f64 engineUpdateTimeHistory[10];
        f64 engineRenderTimeHistory[10];

        for (u32 i = 0; i < 10; ++i)
        {
            engineUpdateTimeHistory[i] = 0.0;
            engineRenderTimeHistory[i] = 0.0;
        }

        while (mRunning)
        {

			u64 newTime = SDL_GetPerformanceCounter();
			f64 delta = (f64(newTime - currentTime) * k_millisecondsToSeconds) / (f64)k_frequency;
            currentTime = newTime;

			if (delta > dt * k_dtUpperToleranceMult)
			{
				delta = dt;
			}

			if (delta < 0) {
                delta = 0;
			}

			/*for (s64 snap : snap_frequencies)
			{
				if (std::abs(delta - snap) <= vsync_maxerror)
				{
					delta = snap;
					break;
				}
			}*/

   //         //first k_ignoreFrames frames are a wash
   //         //just assume they're fine so we don't mess up the timing
   //        if (totalFrames < k_ignoreFrames)
   //        {
   //             delta = dt;
   //        }

			////delta time averaging
			//for (u32 i = 0; i < k_timeHistoryCount - 1; i++) 
   //         {
			//	timeAverager[i] = timeAverager[i + 1];
			//}
			//
   //       //  timeAverager[totalFrames % k_timeHistoryCount] = delta;

   //         timeAverager[k_timeHistoryCount - 1] = delta;
			//
   //         delta = 0;
			//
   //         for (u32 i = 0; i < k_timeHistoryCount; i++)
   //         {
   //             delta += timeAverager[i];
   //         }
			//
   //         delta /= k_timeHistoryCount;

			accumulator += delta;

			if (accumulator > dt * k_dtUpperToleranceMult) {
				mResync = true;
			}

			//timer resync if requested
			if (mResync) {
				accumulator = 0;
				delta = dt;
				mResync = false; 

				//for (u32 i = 0; i < k_timeHistoryCount; i++)
				//{
    //                timeAverager[i] = dt;
				//}
			}
           
            ProcessEvents();

            u32 numGameUpdates = 0;

            while (accumulator >= dt)
            {
			//	tempCurrent = SDL_GetPerformanceCounter();
			//	auto temp = tempCurrent - tempLast;
			//	tempLast = tempCurrent;

             //   if (temp > 100)
                {

           //         printf("Time: %f\n", (f64(temp) * k_millisecondsToSeconds) / (f64)k_frequency);
                }

            //    printf("Accum: %f\n", accumulator);
               

               // r2::util::Timer engineTimer("Engine timer", false);
               // engineTimer.Start();
                mEngine.Update();
              //  f64 result = engineTimer.Stop();

				//for (u32 i = 0; i < 10 - 1; i++)
				//{
				//	engineUpdateTimeHistory[i] = engineUpdateTimeHistory[i + 1];
				//}

    //            engineUpdateTimeHistory[10 - 1] = result;


				accumulator -= dt;

                //t+= dt;
                numGameUpdates++;

            //    printf("Num Updated: %u\n", numGameUpdates);
            }

     //       printf("numGameUpdates: %d\n", numGameUpdates);

            float alpha = static_cast<f64>(accumulator) / static_cast<f64>(dt);

			r2::util::Timer renderTimer("Render timer", false);
            renderTimer.Start();

            mEngine.Render(alpha);

			f64 result = renderTimer.Stop();

			for (u32 i = 0; i < 10 - 1; i++)
			{
				engineRenderTimeHistory[i] = engineRenderTimeHistory[i + 1];
			}

            engineRenderTimeHistory[10 - 1] = result;

            
            r2::draw::rendererimpl::SwapScreens();

            r2::mem::GlobalMemory::EngineMemory().singleFrameArena->EnsureZeroAllocations();
            
            r2::mem::GlobalMemory::EngineMemory().singleFrameArena->GetPolicyRef().Reset();

#if defined( R2_DEBUG ) || defined(R2_RELEASE) || defined(R2_PUBLISH)
            frames++;
            totalFrames++;
			//Calculate ms per frame
			{
				endTime = SDL_GetPerformanceCounter();
                f64 msDiff = (f64(endTime - startTime) * k_millisecondsToSeconds) / (f64)k_frequency;

				if (msDiff >= k_millisecondsForFPSUpdate &&
					frames >= k_framesForFPSUpdate)
				{

                    f64 msPerFrame = msDiff / (f64)frames;
                    if (totalFrames > k_ignoreFrames)
                    {
						maxMSPerFrame = glm::max(maxMSPerFrame, msPerFrame);
						minMSPerFrame = glm::min(minMSPerFrame, glm::max(msPerFrame, 0.0));
                    }

					sprintf(newTitle, "%s - ms per frame: %f, 1 frame max: %f, 1 frame min: %f", mApplicationName, msPerFrame, maxMSPerFrame, minMSPerFrame);
					SetWindowTitle(newTitle);

					frames = 0;
					startTime = endTime;
				}
			}


#endif // R2_DEBUG ) || defined(R2_RELEASE)
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
        
        IMG_Quit();
        SDL_Quit();
    }

    const f64 SDL2Platform::TickRate() const
    {
        //@TODO(Serge): fix this
        return 1000.0 / 60.0;
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
    
	void SDL2Platform::ResetAccumulator()
	{
        mResync = true;
	}

    u32 SDL2Platform::GetNumberOfAttachedGameControllers() const
    {
        u32 count = 0;

		for (u32 i = 0; i < Engine::NUM_PLATFORM_CONTROLLERS; ++i)
		{
			if (mSDLGameControllers[i].IsOpen() && mSDLGameControllers[i].IsAttached())
			{
                count++;
			}
		}

        return count;
    }

    void SDL2Platform::CloseGameController(io::ControllerID index)
    {
		mSDLGameControllers[index].Close();
    }

    void SDL2Platform::CloseAllGameControllers()
    {
        for (u32 i = 0; i < Engine::NUM_PLATFORM_CONTROLLERS; ++i)
        {
            mSDLGameControllers[i].Close();
        }
    }

    bool SDL2Platform::IsGameControllerConnected(io::ControllerID index) const
    {
        return mSDLGameControllers[index].IsAttached();
    }

    const char* SDL2Platform::GetGameControllerMapping(r2::io::ControllerID controllerID) const
    {
        return mSDLGameControllers[controllerID].GetMapping();
    }

    const char* SDL2Platform::GetGameControllerButtonName(r2::io::ControllerID controllerID, r2::io::ControllerButtonName buttonName) const
    {
        return mSDLGameControllers[controllerID].GetButtonName(buttonName);
    }

    const char* SDL2Platform::GetGameControllerAxisName(r2::io::ControllerID controllerID, r2::io::ControllerAxisName axisName) const
    {
        return mSDLGameControllers[controllerID].GetAxisName(axisName);
    }

    const char* SDL2Platform::GetGameControllerName(r2::io::ControllerID controllerID) const
    {
        return mSDLGameControllers[controllerID].GetControllerName();
    }

    s32 SDL2Platform::GetPlayerIndex(r2::io::ControllerID controllerID) const
    {
        return mSDLGameControllers[controllerID].GetPlayerIndex();
    }

    void SDL2Platform::SetPlayerIndex(r2::io::ControllerID controllerID, s32 playerIndex)
    {
        mSDLGameControllers[controllerID].SetPlayerIndex(playerIndex);
    }

    io::ControllerType SDL2Platform::GetControllerType(r2::io::ControllerID controllerID)const
    {
        return mSDLGameControllers[controllerID].GetControllerType();
    }

	//--------------------------------------------------
    //                  Private
    //--------------------------------------------------

    io::ControllerID SDL2Platform::GetControllerIDFromControllerInstance(io::ControllerInstanceID instanceID)
    {
        for (u32 i= 0; i < Engine::NUM_PLATFORM_CONTROLLERS; ++i)
        {
            if (mSDLGameControllers[i].GetControllerInstanceID() == instanceID)
            {
                return mSDLGameControllers[i].GetControllerID();
            }
        }

        return io::INVALID_CONTROLLER_ID;
    }

    void SDL2Platform::ProcessEvents()
    {
		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
#ifdef R2_IMGUI
            ImGui_ImplSDL2_ProcessEvent(&e);
            ImGuiIO& io = ImGui::GetIO(); (void)io;
#endif

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
#if defined R2_IMGUI && defined R2_EDITOR
                if (mEngine.mEditorLayer && mEngine.mEditorLayer->IsEnabled())
                {
                    if (io.WantCaptureMouse)
                    {
                        break;
                    }
                }
#endif
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
				
#if defined R2_IMGUI && defined R2_EDITOR
                if (mEngine.mEditorLayer && mEngine.mEditorLayer->IsEnabled())
                {
                    if (io.WantCaptureMouse)
                    {
                        break;
                    }
                }
#endif
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
#if defined R2_IMGUI && defined R2_EDITOR
                if (mEngine.mEditorLayer && mEngine.mEditorLayer->IsEnabled())
                {
                    if (io.WantCaptureMouse)
                    {
                        break;
                    }
                }
#endif
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
                keyData.modifiers = 0;

				//@NOTE: Right now we make no distinction between left or right versions of these keys
				if (e.key.keysym.mod & KMOD_ALT)
				{
					keyData.modifiers |= io::ALT_PRESSED;
				}

				if (e.key.keysym.mod & KMOD_SHIFT)
				{
					keyData.modifiers |= io::Key::SHIFT_PRESSED_KEY;
				}

				if (e.key.keysym.mod & KMOD_CTRL)
				{
					keyData.modifiers |= io::Key::CONTROL_PRESSED;
				}

#if defined R2_IMGUI && defined R2_EDITOR
                //HACK for editor
                if (mEngine.mEditorLayer && mEngine.mEditorLayer->IsEnabled())
                {
					if (io.WantCaptureKeyboard &&
						!(((keyData.modifiers & io::Key::SHIFT_PRESSED_KEY) == io::Key::SHIFT_PRESSED_KEY) &&
							(keyData.code == io::KEY_F1)))
					{
						break;
					}
                }
#endif

				mEngine.KeyEvent(keyData);
			}
			break;

			case SDL_TEXTINPUT:
			{
#if defined R2_IMGUI && defined R2_EDITOR
                if (mEngine.mEditorLayer && mEngine.mEditorLayer->IsEnabled())
                {
                    if (io.WantTextInput)
                    {
                        break;
                    }
                }
#endif
				mEngine.TextEvent(e.text.text);
			}
			break;

			case SDL_CONTROLLERDEVICEADDED:
			{
                io::ControllerID controllerID = e.cdevice.which;


                bool wasOpen = mSDLGameControllers[controllerID].IsOpen();
                if (!wasOpen)
                {
                    bool success = mSDLGameControllers[controllerID].Open(controllerID);

                    R2_CHECK(success, "Failed to open the game controller");
                }

                auto instanceID = mSDLGameControllers[controllerID].GetControllerInstanceID();
                if (!wasOpen)
                {
                    mEngine.GameControllerOpened(controllerID, instanceID);
                }
                else
                {
                    mEngine.GameControllerReconnected(controllerID, instanceID);
                }
                
			}
			break;
			case SDL_CONTROLLERDEVICEREMOVED:
			{
                io::ControllerInstanceID instanceID = e.cdevice.which;

                io::ControllerID controllerID = GetControllerIDFromControllerInstance(instanceID);

                CloseGameController(controllerID);

                mEngine.GameControllerDisconnected(controllerID);
			}
			break;
			case SDL_CONTROLLERDEVICEREMAPPED:
			{
                io::ControllerInstanceID instanceID = e.cdevice.which;
				io::ControllerID controllerID = GetControllerIDFromControllerInstance(instanceID);

                
				mEngine.ControllerRemappedEvent(controllerID);
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
    }


	void SDL2Platform::SetWindowTitle(const char* title)
	{
        r2::draw::rendererimpl::SetWindowName(title);
	}
}




#endif
