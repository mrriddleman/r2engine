//
//  RenderLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "RenderLayer.h"
#include "r2/Core/Events/Events.h"
#include "r2/Platform/Platform.h"
//temp
#include "r2/Render/Backends/RendererBackend.h"

namespace r2
{
    RenderLayer::RenderLayer(): Layer("Render Layer", true)
    {
        
    }
    
    void RenderLayer::Init()
    {
        //@Temp
        mPersController.Init(2.5f, 45.0f, static_cast<float>(CENG.DisplaySize().width)/ static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.0f);
        
        r2::draw::OpenGLInit();
    }
    
    void RenderLayer::Update()
    {
        mPersController.Update();
    }
    
    void RenderLayer::Render(float alpha)
    {
        //@Temp
        r2::draw::OpenGLSetCamera(mPersController.GetCamera());
        
        r2::draw::OpenGLDraw(alpha);
    }
    
    void RenderLayer::OnEvent(evt::Event& event)
    {
        r2::evt::EventDispatcher dispatcher(event);
        
        dispatcher.Dispatch<r2::evt::WindowResizeEvent>([](const r2::evt::WindowResizeEvent& e)
        {
            r2::draw::OpenGLResizeWindow(e.Width(), e.Height());
            return true;
        });
        
        
        mPersController.OnEvent(event);
    }
    
    void RenderLayer::Shutdown()
    {
        //@Temp
        r2::draw::OpenGLShutdown();
    }
}
