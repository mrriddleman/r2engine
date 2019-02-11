//
//  Engine.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-08.
//

#include "Engine.h"
#include "glad/glad.h"
#include "r2/Core/Events/Events.h"

namespace r2
{
    Engine::Engine():mSetVSyncFunc(nullptr), mFullScreenFunc(nullptr), mWindowSizeFunc(nullptr)
    {
        
    }
    
    Engine::~Engine()
    {
        
    }
    
    bool Engine::Init(std::unique_ptr<Application> app)
    {
        mApp = std::move(app);
        
        if(mApp)
        {
            bool appInitialized = mApp->Init();
            glClearColor(0.f, 0.5f, 1.0f, 1.0f);
            glViewport(0, 0, GetInitialResolution().width, GetInitialResolution().height);
            
            return appInitialized;
        }

        return false;
    }
    
    void Engine::Update()
    {
        mApp->Update();
    }
    
    void Engine::Shutdown()
    {
        mApp->Shutdown();
    }
    
    void Engine::Render(float alpha)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    utils::Size Engine::GetInitialResolution() const
    {
        utils::Size res;
        res.width = 1024;
        res.height = 720;
        
        return res;
    }
    
    const std::string& Engine::OrganizationName() const
    {
        static std::string org = "ScreamStudios";
        return org;
    }
    
    void Engine::WindowResizedEvent(u32 width, u32 height)
    {
        evt::WindowResizeEvent e(width, height);
        OnEvent(e);
        glViewport(0, 0, width, height);
    }
    
    void Engine::WindowSizeChangedEvent(u32 width, u32 height)
    {
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
            evt::KeyReleasedEvent e(keyData.code);
            OnEvent(e);
        }
    }
    
    void Engine::OnEvent(evt::Event& e)
    {
        //does the brute force OnEvent stuff
    }

    
}
