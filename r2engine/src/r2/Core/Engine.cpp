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
#include "r2/Platform/Platform.h"
#include "imgui.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Core/Containers/SHashMap.h"

namespace r2
{
    const u64 Engine::MAX_NUM_MEMORY_AREAS = 10;
    
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
            r2::mem::GlobalMemory::Init<MAX_NUM_MEMORY_AREAS>();

            const Application * noptrApp = app.get();
            r2::Log::Init(r2::Log::INFO, noptrApp->GetAppLogPath() + "full.log", CPLAT.RootPath() + "logs/" + "full.log");
            //@TODO(Serge): should check to see if the app initialized!
            PushLayer(std::make_unique<AppLayer>(std::move(app)));
            PushLayer(std::make_unique<ImGuiLayer>());
        
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
        mLayerStack.Update();
    }
    
    void Engine::Shutdown()
    {
        mLayerStack.ShutdownAll();
        
        r2::mem::GlobalMemory::Shutdown();
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
