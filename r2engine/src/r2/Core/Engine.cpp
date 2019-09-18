//
//  Engine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-08.
//

#include "Engine.h"
#include "glad/glad.h"
#include "r2/Core/Events/Events.h"
#include "r2/ImGui/ImGuiLayer.h"
#include "r2/Core/Layer/AppLayer.h"
#include "r2/Core/Layer/SoundLayer.h"
#include "r2/Platform/Platform.h"
#include "imgui.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Audio/AudioEngine.h"
#include <unistd.h>
#include "r2/Core/Assets/AssetLib.h"

#ifdef R2_DEBUG
#include <chrono>
#include "r2/Core/Assets/Pipeline/AssetWatcher.h"
#endif

namespace r2
{
    void TestSound(const std::string& appPath)
    {
        r2::audio::AudioEngine audio;
        
        std::string wavePath = appPath + "sounds" + "/wave.mp3";
        std::string swishPath = appPath + "sounds" + "/swish.wav";
        
        r2::audio::AudioEngine::SoundDefinition waveSoundDef;
        strcpy( waveSoundDef.soundName, wavePath.c_str() );
        waveSoundDef.defaultVolume = 1.0f;
        waveSoundDef.minDistance = 0.0f;
        waveSoundDef.maxDistance = 1.0f;
        waveSoundDef.flags |= r2::audio::AudioEngine::LOOP;
        waveSoundDef.flags |= r2::audio::AudioEngine::STREAM;
        
      //  auto waveSoundID = audio.RegisterSound(waveSoundDef);
        
        r2::audio::AudioEngine::SoundDefinition swishSoundDef;
        strcpy( swishSoundDef.soundName, swishPath.c_str() );
        swishSoundDef.defaultVolume = 1.0f;
        swishSoundDef.minDistance = 0.0f;
        swishSoundDef.maxDistance = 1.0f;
        swishSoundDef.flags |= r2::audio::AudioEngine::STREAM;
        
        auto swishSoundID = audio.RegisterSound(swishSoundDef);
        
        audio.PlaySound(swishSoundID, glm::vec3(0,0,0), 0.5, 0.5);
       // sleep(10);
        
       // audio.StopChannel(channelID, 10);
    }
    
    Engine::Engine():mSetVSyncFunc(nullptr), mFullScreenFunc(nullptr), mWindowSizeFunc(nullptr)
    {
        
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
            
#ifdef R2_ASSET_PIPELINE
            std::string flatcPath = R2_FLATC;
            std::string manifestDir = noptrApp->GetAssetManifestPath();
            std::string assetTemp = noptrApp->GetAssetCompilerTempPath();
            r2::asset::pln::Init(manifestDir, assetTemp, flatcPath, std::chrono::milliseconds(500), noptrApp->GetAssetWatchPaths(), r2::asset::lib::PushFilesBuilt);
#endif
            
            r2::asset::lib::Init();
            //@TODO(Serge): should check to see if the app initialized!
            PushLayer(std::make_unique<AppLayer>(std::move(app)));
            PushLayer(std::make_unique<ImGuiLayer>());
            PushLayer(std::make_unique<SoundLayer>());
            
           // TestSound(CPLAT.RootPath());
            mDisplaySize = noptrApp->GetPreferredResolution();
            glClearColor(0.f, 0.5f, 1.0f, 1.0f);
            
            mWindowSizeFunc(mDisplaySize.width, mDisplaySize.height);
            WindowSizeChangedEvent(mDisplaySize.width, mDisplaySize.height);
        
            R2_LOG(INFO, "Test to see if we get a log file");

            return true;
        }

        return false;
    }
    
    void Engine::Update()
    {
        r2::asset::lib::Update();
        
        mLayerStack.Update();
        
//        static u32 ticks = 0;
//        static bool pauseRequested = false;
//        static bool dontPauseAnymore = false;
//        ticks += CPLAT.TickRate();
//
//        if (ticks > 5000 && !pauseRequested && !dontPauseAnymore)
//        {
//            r2::audio::AudioEngine audio;
//            audio.PauseChannel(0);
//            pauseRequested = true;
//        }
//
//        if (ticks > 10000 && pauseRequested && !dontPauseAnymore)
//        {
//            r2::audio::AudioEngine audio;
//            audio.PauseChannel(0);
//            dontPauseAnymore = true;
//        }
        
    }
    
    void Engine::Shutdown()
    {
        mLayerStack.ShutdownAll();
        
        r2::asset::lib::Shutdown();
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::Shutdown();
#endif
    }
    
    void Engine::Render(float alpha)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mLayerStack.Render(alpha);
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
        glViewport(0, 0, width, height);
    }
    
    void Engine::WindowSizeChangedEvent(u32 width, u32 height)
    {
        mDisplaySize.width = width;
        mDisplaySize.height = height;
        evt::WindowResizeEvent e(width, height);
        OnEvent(e);
        glViewport(0, 0, width, height);
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
    
    void Engine::OnEvent(evt::Event& e)
    {
        mLayerStack.OnEvent(e);
    }
}
