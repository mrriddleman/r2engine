//
//  OrthoCameraController.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#include "OrthoCameraController.h"
#include "r2/Platform/Platform.h"

namespace r2::cam
{
    void OrthoCameraController::Init(float cameraMoveSpeed, float aspect, float near, float far)
    {
        mAspect = aspect;
        mCameraMoveSpeed = cameraMoveSpeed;
        mNear = near;
        mFar = far;
        SetOrthoCam(mCamera, -aspect * mZoom, aspect * mZoom, -mZoom, mZoom, near, far);
    }
    
    void OrthoCameraController::Update()
    {
        float speed = util::MillisecondsToSeconds(CPLAT.TickRate()) * mCameraMoveSpeed;
        if(mDirectionPressed.IsSet(FORWARD))
        {
            MoveCameraBy(mCamera, mCamera.up * speed);
        }
        else if(mDirectionPressed.IsSet(BACKWARD))
        {
            MoveCameraBy(mCamera, mCamera.up * -speed);
        }
        else if(mDirectionPressed.IsSet(LEFT))
        {
            MoveCameraBy(mCamera, glm::normalize(glm::cross(mCamera.facing, mCamera.up)) * -speed);
        }
        else if(mDirectionPressed.IsSet(RIGHT))
        {
            MoveCameraBy(mCamera, glm::normalize(glm::cross(mCamera.facing, mCamera.up)) * speed);
        }
    }
    
    void OrthoCameraController::OnEvent(evt::Event& event)
    {
        r2::evt::EventDispatcher dispatcher(event);
        
        dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e)
        {
          if(e.KeyCode() == r2::io::KEY_w)
          {
              mDirectionPressed.Set(FORWARD);
          }
          else if(e.KeyCode() == r2::io::KEY_s)
          {
              mDirectionPressed.Set(BACKWARD);
          }
          else if(e.KeyCode() == r2::io::KEY_a)
          {
              mDirectionPressed.Set(LEFT);
          }
          else if(e.KeyCode() == r2::io::KEY_d)
          {
              mDirectionPressed.Set(RIGHT);
          }
          
          return true;
       });
        
        dispatcher.Dispatch<r2::evt::KeyReleasedEvent>([this](const r2::evt::KeyReleasedEvent& e)
       {
           if(e.KeyCode() == r2::io::KEY_w)
           {
               mDirectionPressed.Remove(FORWARD);
           }
           else if(e.KeyCode() == r2::io::KEY_s)
           {
               mDirectionPressed.Remove(BACKWARD);
           }
           else if(e.KeyCode() == r2::io::KEY_a)
           {
               mDirectionPressed.Remove(LEFT);
           }
           else if(e.KeyCode() == r2::io::KEY_d)
           {
               mDirectionPressed.Remove(RIGHT);
           }
           
           return true;
       });

        dispatcher.Dispatch<r2::evt::MouseWheelEvent>([this](const r2::evt::MouseWheelEvent&e){
            
            if(mZoom >= 1.0f && mZoom <= 4.0f)
                mZoom -= e.YAmount() * 0.1f;
            if(mZoom <= 1.0f)
                mZoom = 1.0f;
            if(mZoom >= 4.0f)
                mZoom = 4.0f;
            
            SetOrthoCam(mCamera, -mAspect * mZoom, mAspect * mZoom, -mZoom, mZoom, mNear, mFar);
            return true;
        });
    }
}
