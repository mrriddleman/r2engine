//
//  RenderLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "RenderLayer.h"
#include "r2/Core/Events/Events.h"
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
        r2::draw::OpenGLInit();
    }
    
    void RenderLayer::Update()
    {
        
    }
    
    void RenderLayer::Render(float alpha)
    {
        //@Temp
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
    }
    
    void RenderLayer::Shutdown()
    {
        //@Temp
        r2::draw::OpenGLShutdown();
    }
}
