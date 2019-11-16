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
        r2::draw::OpenGLInit(CENG.DisplaySize().width, CENG.DisplaySize().height);
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
        
        dispatcher.Dispatch<r2::evt::KeyPressedEvent>([](const r2::evt::KeyPressedEvent& e)
        {
           // if(!e.Repeated())
            {
                if(e.KeyCode() == r2::io::KEY_w)
                {
                    r2::draw::OpenGLMoveCameraForward(true);
                }
                else if(e.KeyCode() == r2::io::KEY_s)
                {
                    r2::draw::OpenGLMoveCameraBack(true);
                }
                else if(e.KeyCode() == r2::io::KEY_a)
                {
                    r2::draw::OpenGLMoveCameraLeft(true);
                }
                else if(e.KeyCode() == r2::io::KEY_d)
                {
                    r2::draw::OpenGLMoveCameraRight(true);
                }
            }
            return true;
        });
        
        
        dispatcher.Dispatch<r2::evt::KeyReleasedEvent>([](const r2::evt::KeyReleasedEvent& e)
      {
          // if(!e.Repeated())
          {
              if(e.KeyCode() == r2::io::KEY_w)
              {
                  r2::draw::OpenGLMoveCameraForward(false);
              }
              else if(e.KeyCode() == r2::io::KEY_s)
              {
                  r2::draw::OpenGLMoveCameraBack(false);
              }
              else if(e.KeyCode() == r2::io::KEY_a)
              {
                  r2::draw::OpenGLMoveCameraLeft(false);
              }
              else if(e.KeyCode() == r2::io::KEY_d)
              {
                  r2::draw::OpenGLMoveCameraRight(false);
              }
          }
          return true;
      });
        
        
        static float lastX = CENG.DisplaySize().width/2;
        static float lastY = CENG.DisplaySize().height/2;
        static float yaw = 0, pitch=0;
        dispatcher.Dispatch<r2::evt::MouseMovedEvent>([](const r2::evt::MouseMovedEvent& e){
            
            float xOffset = e.X() - lastX;
            float yOffset = lastY - e.Y();
            
            lastX = e.X();
            lastY = e.Y();
            
            float sensitivity = 0.5f;
            xOffset *= sensitivity;
            yOffset *= sensitivity;
            
            yaw += xOffset;
            pitch += yOffset;
            
            if (pitch > 89.0f)
                pitch = 89.0f;
            
            if(pitch < -89.0f)
                pitch = -89.0f;
            
            r2::draw::OpenGLSetCameraFacing(pitch, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            
            
            return true;
        });
        
        static float fov = 45.0f;
        
        dispatcher.Dispatch<r2::evt::MouseWheelEvent>([](const r2::evt::MouseWheelEvent&e){
            
            if(fov >= 1.0f && fov <= 45.0f)
                fov -= e.YAmount();
            if(fov <= 1.0f)
                fov = 1.0f;
            if(fov >= 45.0f)
                fov = 45.0f;
            r2::draw::OpenGLSetCameraZoom(fov);
            return true;
        });
    }
    
    void RenderLayer::Shutdown()
    {
        //@Temp
        r2::draw::OpenGLShutdown();
    }
}
