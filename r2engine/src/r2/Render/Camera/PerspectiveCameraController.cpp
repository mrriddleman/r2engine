//
//  PerspectiveCameraController.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "r2/Platform/Platform.h"
namespace r2::cam
{

    void PerspectiveController::Init(float camMoveSpeed, float fov, float aspect, float near, float far, const glm::vec3& position)
    {
        
        mCameraMoveSpeed = camMoveSpeed;
        mFOV = fov;
        mAspect = aspect;
        mNear = near;
        mFar = far;
        InitPerspectiveCam(mCamera, fov, aspect, near, far, position);
    }
    
    void PerspectiveController::Update()
    {
        float speed = util::MillisecondsToSeconds(CPLAT.TickRate()) * mCameraMoveSpeed;
        
        glm::vec3 deltaMove = mCamera.facing * speed;
        
        if(mDirectionPressed.IsSet(FORWARD))
        {
            MoveCameraBy(mCamera, deltaMove);
        }
        else if(mDirectionPressed.IsSet(BACKWARD))
        {
            MoveCameraBy(mCamera, -deltaMove);
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
    
    void PerspectiveController::SetAspect(float aspect)
    {
        mAspect = aspect;
        SetPerspectiveCam(mCamera, mFOV, aspect, mNear, mFar);
    }
    
    void PerspectiveController::SetFOV(float fov)
    {
        mFOV = fov;
        SetPerspectiveCam(mCamera, fov, mAspect, mNear, mFar);
    }
    
    void PerspectiveController::OnEvent(r2::evt::Event& event)
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

        static float lastX = CENG.DisplaySize().width/2;
        static float lastY = CENG.DisplaySize().height/2;
        static float yaw = -90.f, pitch=0.f;
        dispatcher.Dispatch<r2::evt::MouseMovedEvent>([this](const r2::evt::MouseMovedEvent& e){
            
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
            
            SetFacingDir(mCamera, pitch, yaw);
            
            return true;
        });
        
        

        dispatcher.Dispatch<r2::evt::MouseWheelEvent>([this](const r2::evt::MouseWheelEvent&e){
            
            if(mFOV >= 1.0f && mFOV <= 45.0f)
                mFOV -= e.YAmount();
            if(mFOV <= 1.0f)
                mFOV = 1.0f;
            if(mFOV >= 45.0f)
                mFOV = 45.0f;
            
            SetPerspectiveCam(mCamera, mFOV, mAspect, mNear, mFar);
           return true;
        });
    }
}