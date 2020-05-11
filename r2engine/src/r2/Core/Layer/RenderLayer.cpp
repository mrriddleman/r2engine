//
//  RenderLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "r2pch.h"
#include "RenderLayer.h"
#include "r2/Core/Events/Events.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Platform/Platform.h"
//temp
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/CommandBucket.h"
#include "r2/Render/Renderer/RenderKey.h"
//#include "r2/Render/Backends/RendererBackend.h"



namespace r2
{
    RenderLayer::RenderLayer(const char* shaderManifestPath)
        : Layer("Render Layer", true)
        , mShaderManifestPath(shaderManifestPath)
    {
        
    }
    
    void RenderLayer::Init()
    {
        //@Temp
        mPersController.Init(2.5f, 45.0f, static_cast<float>(CENG.DisplaySize().width)/ static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.f, glm::vec3(0.0f, 0.0f, 3.0f));
        r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
        r2::draw::renderer::Init(engineMem.internalEngineMemoryHandle, mShaderManifestPath);

        //@TODO(Serge): kinda clunky - maybe we should create buckets and add them to the renderer
        r2::draw::renderer::SetCameraPtrOnBucket(mPersController.GetCameraPtr());
    }
    
    void RenderLayer::Update()
    {
        mPersController.Update();
        r2::draw::renderer::Update();
    }
    
    void RenderLayer::Render(float alpha)
    {

        r2::draw::renderer::Render(alpha);
        //@Temp
      //  r2::draw::OpenGLSetCamera(mPersController.GetCamera());
        
      //  r2::draw::OpenGLDraw(alpha);
    }
    
    void RenderLayer::OnEvent(evt::Event& event)
    {
        r2::evt::EventDispatcher dispatcher(event);
        
        dispatcher.Dispatch<r2::evt::WindowResizeEvent>([this](const r2::evt::WindowResizeEvent& e)
        {
            mPersController.SetAspect(static_cast<float>(e.Width()) / static_cast<float>(e.Height()));
            
            r2::draw::renderer::WindowResized(e.Width(), e.Height());
           // r2::draw::OpenGLResizeWindow(e.Width(), e.Height());
            return true;
        });
        
        dispatcher.Dispatch<r2::evt::MouseButtonPressedEvent>([this](const r2::evt::MouseButtonPressedEvent& e)
        {
            if(e.MouseButton() == r2::io::MOUSE_BUTTON_LEFT)
            {
                r2::math::Ray ray = r2::cam::CalculateRayFromMousePosition(mPersController.GetCamera(), e.XPos(), e.YPos());
                
              //  r2::draw::OpenGLDrawRay(ray);
                return true;
            }
            
            return false;
        });
        
        dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e){
            if (e.KeyCode() == r2::io::KEY_LEFT)
            {
            //    r2::draw::OpenGLPrevAnimation();
                return true;
            }
            else if(e.KeyCode() == r2::io::KEY_RIGHT)
            {
            //    r2::draw::OpenGLNextAnimation();
                return true;
            }
            return false;
        });
        
        mPersController.OnEvent(event);
    }
    
    void RenderLayer::Shutdown()
    {
        r2::draw::renderer::Shutdown();
        //@Temp
        //r2::draw::OpenGLShutdown();
    }
}
