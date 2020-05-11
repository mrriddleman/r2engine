//
//  Engine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-08.
//

#include "r2pch.h"
#include "Engine.h"
#include "r2/Core/Events/Events.h"
#include "r2/ImGui/ImGuiLayer.h"
#include "r2/Core/Layer/AppLayer.h"
#include "r2/Core/Layer/SoundLayer.h"
#include "r2/Core/Layer/RenderLayer.h"
#include "r2/Platform/Platform.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/File/PathUtils.h"
#ifdef R2_DEBUG
#include <chrono>
#include "r2/Core/Assets/Pipeline/AssetWatcher.h"
#endif

namespace r2
{
    const u32 Engine::NUM_PLATFORM_CONTROLLERS;
    

    void TestSound(const std::string& appPath)
    {
        r2::audio::AudioEngine audio;

        bool loaded = audio.LoadSound((r2::audio::AudioEngine::SoundID)1);
        if (loaded)
            audio.PlaySound((r2::audio::AudioEngine::SoundID)1, glm::vec3(0, 0, 0), 0.5, 1.0);
        else
            R2_CHECK(false, "");
    }

    Engine::Engine():mSetVSyncFunc(nullptr), mFullScreenFunc(nullptr), mWindowSizeFunc(nullptr), mMinimized(false)
    {
        for (u32 i = 0; i < NUM_PLATFORM_CONTROLLERS; ++i)
        {
            mPlatformControllers[i] = nullptr;
        }
    }
    
    Engine::~Engine()
    {
        
    }
    
    bool Engine::Init(std::unique_ptr<Application> app)
    {
        if(app)
        {
            const Application * noptrApp = app.get();
            r2::Log::Init(r2::Log::INFO, noptrApp->GetAppLogPath() + "full.log", CPLAT.RootPath() + "logs/" + "full.log");

            //@TODO(Serge): figure out how much to give the asset lib
            //@Test
            {
                mAssetLibMemBoundary = MAKE_BOUNDARY(*MEM_ENG_PERMANENT_PTR, Megabytes(1), CPLAT.CPUCacheLineSize());
            }
            
            r2::asset::lib::Init(mAssetLibMemBoundary);

#ifdef R2_ASSET_PIPELINE
            r2::asset::pln::SoundDefinitionCommand soundCommand;

            soundCommand.soundDefinitionFilePath = noptrApp->GetSoundDefinitionPath();
            soundCommand.soundDirectories = noptrApp->GetSoundDirectoryWatchPaths();
            soundCommand.buildFunc = [](std::vector<std::string> paths)
            {
                r2::audio::AudioEngine::PushNewlyBuiltSoundDefinitions(paths);
            };

            r2::asset::pln::AssetCommand assetCommand;

            std::string flatcPath = R2_FLATC;
            assetCommand.assetManifestsPath = noptrApp->GetAssetManifestPath();
            assetCommand.assetTempPath = noptrApp->GetAssetCompilerTempPath();
            assetCommand.pathsToWatch = noptrApp->GetAssetWatchPaths();
            assetCommand.assetsBuldFunc = r2::asset::lib::PushFilesBuilt;

            r2::asset::pln::ShaderManifestCommand shaderCommand;
            char path[r2::fs::FILE_PATH_LENGTH];

            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_MANIFEST, "", path);
            shaderCommand.manifestDirectory = std::string(path);
            shaderCommand.manifestFilePath = noptrApp->GetShaderManifestsPath();

            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "", path);
            shaderCommand.shaderWatchPath = std::string(path);

            r2::asset::pln::Init(flatcPath, std::chrono::milliseconds(200), assetCommand, soundCommand, shaderCommand);
#endif
            
            mDisplaySize = noptrApp->GetPreferredResolution();
           

            //@TODO(Serge): don't use make unique!
            PushLayer(std::make_unique<RenderLayer>(noptrApp->GetShaderManifestsPath().c_str()));
            PushLayer(std::make_unique<SoundLayer>());
            
            std::unique_ptr<ImGuiLayer> imguiLayer = std::make_unique<ImGuiLayer>();
            mImGuiLayer = imguiLayer.get();
            PushOverlay(std::move(imguiLayer));

            //Should be last/ first in the stack
            //@TODO(Serge): should check to see if the app initialized!
            PushLayer(std::make_unique<AppLayer>(std::move(app)));

            DetectGameControllers();

            
         //   TestSound(CPLAT.RootPath());
            
            
            return true;
        }

        return false;
    }
    
    void Engine::Update()
    {
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::Update(); //for shaders
#endif
        r2::asset::lib::Update();
        if (!mMinimized)
        {
            mLayerStack.Update();
        }
    }
    
    void Engine::Shutdown()
    {
        mLayerStack.ShutdownAll();
        
        for (u32 i = 0; i < NUM_PLATFORM_CONTROLLERS; ++i)
        {
            if (mPlatformControllers[i] != nullptr)
            {
                CloseGameController(i);
            }
        }
        
        r2::asset::lib::Shutdown();
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::Shutdown();
#endif
        
        FREE((byte*)mAssetLibMemBoundary.location, *MEM_ENG_PERMANENT_PTR);
    }
    
    void Engine::Render(float alpha)
    {
        
        
        if (!mMinimized)
        {
            mLayerStack.Render(alpha);
        }
        
        mImGuiLayer->Begin();
        mLayerStack.ImGuiRender();
        mImGuiLayer->End();
    }
    
    util::Size Engine::GetInitialResolution() const
    {
        util::Size res;
        res.width = 1024;
        res.height = 720;
        
        return res;
    }
    
    const std::string& Engine::OrganizationName() const
    {
        static std::string org = "ScreamStudios";
        return org;
    }
    
    u64 Engine::GetPerformanceFrequency() const
    {
        if(mGetPerformanceFrequencyFunc)
            return mGetPerformanceFrequencyFunc();
        return 0;
    }
    
    u64 Engine::GetPerformanceCounter() const
    {
        if(mGetPerformanceCounterFunc)
            return mGetPerformanceCounterFunc();
        return 0;
    }
    
    u32 Engine::GetTicks() const
    {
        if(mGetTicksFunc)
            return mGetTicksFunc();
        return 0;
    }
    
    r2::io::ControllerID Engine::OpenGameController(r2::io::ControllerID controllerID)
    {
        if (mOpenGameControllerFunc)
        {
            if (controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID &&
                controllerID < NUM_PLATFORM_CONTROLLERS &&
                mPlatformControllers[controllerID] == nullptr)
            {
                mPlatformControllers[controllerID] = mOpenGameControllerFunc(controllerID);
                
                evt::GameControllerConnectedEvent e(controllerID);
                OnEvent(e);
                return controllerID;
            }
        }
        return r2::io::ControllerState::INVALID_CONTROLLER_ID;
    }
    
    u32 Engine::NumberOfGameControllers() const
    {
        if (mNumberOfGameControllersFunc)
        {
            return mNumberOfGameControllersFunc();
        }
        return 0;
    }
    
    bool Engine::IsGameController(r2::io::ControllerID controllerID)
    {
        if (mIsGameControllerFunc)
        {
            return mIsGameControllerFunc(controllerID);
        }
        
        return false;
    }
    
    void Engine::CloseGameController(r2::io::ControllerID controllerID)
    {
        if (mCloseGameControllerFunc)
        {
            if (controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                
                mCloseGameControllerFunc( mPlatformControllers[controllerID] );
                mPlatformControllers[controllerID] = nullptr;
            }
        }
    }
    
    bool Engine::IsGameControllerAttached(r2::io::ControllerID controllerID)
    {
        if (mIsGameControllerAttchedFunc)
        {
            if (controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                return mIsGameControllerAttchedFunc(mPlatformControllers[controllerID]);
            }
        }
        return false;
    }
    
    const char* Engine::GetGameControllerMapping(r2::io::ControllerID controllerID)
    {
        if (mGetGameControllerMappingFunc)
        {
            if (controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                return mGetGameControllerMappingFunc(mPlatformControllers[controllerID]);
            }
        }
        return nullptr;
    }
    
    u8 Engine::GetGameControllerButtonState(r2::io::ControllerID controllerID, r2::io::ControllerButtonName buttonName)
    {
        if (mGetGameControllerButtonStateFunc)
        {
            if (controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                return mGetGameControllerButtonStateFunc(mPlatformControllers[controllerID], buttonName);
            }
        }
        
        return 0;
    }
    
    s16 Engine::GetGameControllerAxisValue(r2::io::ControllerID controllerID, r2::io::ControllerAxisName axisName)
    {
        if (mGetGameControllerAxisValueFunc)
        {
            if(controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                return mGetGameControllerAxisValueFunc(mPlatformControllers[controllerID], axisName);
            }
        }
        
        return 0;
    }
    
    const char* Engine::GetGameControllerButtonName(r2::io::ControllerButtonName buttonName)
    {
        if(mGetStringForButtonFunc)
        {
            return mGetStringForButtonFunc(buttonName);
        }
        return nullptr;
    }
    
    const char* Engine::GetGameControllerAxisName(r2::io::ControllerAxisName axisName)
    {
        if(mGetStringForAxisFunc)
        {
            return mGetStringForAxisFunc(axisName);
        }
        return nullptr;
    }
    
    const char* Engine::GetGameControllerName(r2::io::ControllerID controllerID)
    {
        if (mGetGameControllerNameFunc)
        {
            if(controllerID > r2::io::ControllerState::INVALID_CONTROLLER_ID && controllerID < NUM_PLATFORM_CONTROLLERS && mPlatformControllers[controllerID])
            {
                return mGetGameControllerNameFunc(mPlatformControllers[controllerID]);
            }
        }
        
        return nullptr;
    }
    
    void Engine::PushLayer(std::unique_ptr<Layer> layer)
    {
        layer->Init();
        mLayerStack.PushLayer(std::move(layer));
    }
    
    void Engine::PushOverlay(std::unique_ptr<Layer> overlay)
    {
        overlay->Init();
        mLayerStack.PushOverlay(std::move(overlay));
    }
    
    void Engine::WindowResizedEvent(u32 width, u32 height)
    {
        mDisplaySize.width = width;
        mDisplaySize.height = height;
        evt::WindowResizeEvent e(width, height);
        OnEvent(e);
        
    }
    
    void Engine::WindowSizeChangedEvent(u32 width, u32 height)
    {
        mDisplaySize.width = width;
        mDisplaySize.height = height;
        evt::WindowResizeEvent e(width, height);
        OnEvent(e);
    }
    
    void Engine::WindowMinimizedEvent()
    {
        mMinimized = true;
        evt::WindowMinimizedEvent e;
        OnEvent(e);
    }
    
    void Engine::WindowUnMinimizedEvent()
    {
        mMinimized = false;
        evt::WindowUnMinimizedEvent e;
        OnEvent(e);
    }

    void Engine::QuitTriggered()
    {
        //create quit event - pass to OnEvent
        evt::WindowCloseEvent e;
        OnEvent(e);
    }
    
    void Engine::MouseButtonEvent(io::MouseData mouseData)
    {
        //create mouse button event - pass to OnEvent
        if(mouseData.state == io::BUTTON_PRESSED)
        {
            evt::MouseButtonPressedEvent e(mouseData.button, mouseData.x, mouseData.y, mouseData.numClicks);
            OnEvent(e);
        }
        else
        {
            evt::MouseButtonReleasedEvent e(mouseData.button, mouseData.x, mouseData.y);
            OnEvent(e);
        }
    }
    
    void Engine::MouseMovedEvent(io::MouseData mouseData)
    {
        //create Mouse moved event - pass to OnEvent
        evt::MouseMovedEvent e(mouseData.x, mouseData.y);
        OnEvent(e);
    }
    
    void Engine::MouseWheelEvent(io::MouseData mouseData)
    {
        //create mouse wheel event - pass to OnEvent
        evt::MouseWheelEvent e(mouseData.x, mouseData.y, mouseData.direction);
        OnEvent(e);
    }
    
    void Engine::KeyEvent(io::Key keyData)
    {
        //create key event - pass to OnEvent
        if(keyData.state == io::BUTTON_PRESSED)
        {
            evt::KeyPressedEvent e(keyData.code, keyData.modifiers, keyData.repeated);
            OnEvent(e);
        }
        else
        {
            evt::KeyReleasedEvent e(keyData.code, keyData.modifiers);
            OnEvent(e);
        }
    }
    
    void Engine::TextEvent(const char* text)
    {
        evt::KeyTypedEvent e(text);
        OnEvent(e);
    }
    
    void Engine::ControllerDetectedEvent(io::ControllerID controllerID)
    {
        if (mPlatformControllers[controllerID] == nullptr)
        {
            evt::GameControllerDetectedEvent e(controllerID);
            //  R2_LOGI("%s", e.ToString().c_str());
            OnEvent(e);
            
            auto connectedControllerID = OpenGameController(controllerID);
            
            R2_CHECK(connectedControllerID == controllerID, "controller id should be the same");
        }
    }
    
    void Engine::ControllerDisonnectedEvent(io::ControllerID controllerID)
    {
        evt::GameControllerDisconnectedEvent e(controllerID);
        //R2_LOGI("%s", e.ToString().c_str());
        OnEvent(e);
    }
    
    void Engine::ControllerRemappedEvent(io::ControllerID controllerID)
    {
        evt::GameControllerRemappedEvent e(controllerID);
      //  R2_LOGI("%s", e.ToString().c_str());
        OnEvent(e);
    }
    
    void Engine::ControllerAxisEvent(io::ControllerID controllerID, io::ControllerAxisName axis, s16 value)
    {
        evt::GameControllerAxisEvent e(controllerID, {axis, value});
      //  R2_LOGI("%s", e.ToString().c_str());
        OnEvent(e);
    }
    
    void Engine::ControllerButtonEvent(io::ControllerID controllerID, io::ControllerButtonName buttonName, u8 state)
    {
        evt::GameControllerButtonEvent e(controllerID, {buttonName, state});
     //   R2_LOGI("%s", e.ToString().c_str());
        OnEvent(e);
    }
    
    void Engine::DetectGameControllers()
    {
        const u32 numGameControllers = NumberOfGameControllers();
        
        for (u32 i = 0; i < numGameControllers; ++i)
        {
            if (IsGameController(i))
            {
                ControllerDetectedEvent(i);
            }
        }
    }
    
    void Engine::OnEvent(evt::Event& e)
    {
        mLayerStack.OnEvent(e);
    }
}
