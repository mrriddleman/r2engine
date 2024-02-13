//
//  PerspectiveCameraController.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//
#include "r2pch.h"
#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "r2/Platform/Platform.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace r2::cam
{
    
  //  void PerspectiveController::Init(float camMoveSpeed, float fov, float aspect, float near, float far, const glm::vec3& position, const glm::vec3& facing)
  //  {
		//glm::vec3 yawFacing = glm::normalize(glm::vec3(facing.x, facing.y, 0));
  //      yaw = -glm::degrees(glm::angle(glm::vec3(1, 0, 0), yawFacing));
  //      pitch = 0.0f;

  //      if (math::NearZero(glm::dot(facing, glm::vec3(1, 0, 0))))
  //      {
  //          pitch = glm::degrees(glm::angle(facing, glm::vec3(0, 1, 0)));
  //      }
  //      else
  //      {
  //          pitch = glm::degrees(glm::angle(facing, glm::vec3(1, 0, 0)));
  //      }

		//lastX = (f32)CENG.DisplaySize().width / 2.0f;
  //      lastY = (f32)CENG.DisplaySize().height / 2.0f;

  //      mCameraMoveSpeed = camMoveSpeed;
  //      //mFOV = fov; 
  //      //mAspect = aspect;
  //      //mNear = near;
  //      //mFar = far;
  //      mRightMouseButtonHeld = false;
  //      InitPerspectiveCam(mCamera, fov, aspect, near, far, position, facing);
  //  }
    
    void PerspectiveController::Update()
    {
        float speed = util::MillisecondsToSeconds(CPLAT.TickRate()) * mCameraMoveSpeed;
        
        glm::vec3 deltaMove = mCameraPtr->facing * speed;
        
        if(mDirectionPressed.IsSet(FORWARD))
        {
            MoveCameraBy(*mCameraPtr, deltaMove);
        }
        else if(mDirectionPressed.IsSet(BACKWARD))
        {
            MoveCameraBy(*mCameraPtr, -deltaMove);
        }
        else if(mDirectionPressed.IsSet(LEFT))
        {
            MoveCameraBy(*mCameraPtr, cam::GetWorldRight(*mCameraPtr) * -speed);
        }
        else if(mDirectionPressed.IsSet(RIGHT))
        {
            MoveCameraBy(*mCameraPtr, cam::GetWorldRight(*mCameraPtr) * speed);
        }
    }
    
    void PerspectiveController::SetAspect(float aspect)
    {

        SetPerspectiveCam(*mCameraPtr, mCameraPtr->fov, aspect, mCameraPtr->nearPlane, mCameraPtr->farPlane);
    }
    
    void PerspectiveController::SetFOV(float fov)
    {

        SetPerspectiveCam(*mCameraPtr, fov, mCameraPtr->aspectRatio, mCameraPtr->nearPlane, mCameraPtr->farPlane);
    }
    
    void PerspectiveController::OnEvent(r2::evt::Event& event)
    {
        r2::evt::EventDispatcher dispatcher(event);
        
        dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e)
        {
              if(e.KeyCode() == r2::io::KEY_w)
              {
                  mDirectionPressed.Set(FORWARD);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_s)
              {
                  mDirectionPressed.Set(BACKWARD);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_a)
              {
                  mDirectionPressed.Set(LEFT);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_d)
              {
                  mDirectionPressed.Set(RIGHT);
                  return true;
              }
              
              return false;
        });
        
        dispatcher.Dispatch<r2::evt::KeyReleasedEvent>([this](const r2::evt::KeyReleasedEvent& e)
          {
              if(e.KeyCode() == r2::io::KEY_w)
              {
                  mDirectionPressed.Remove(FORWARD);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_s)
              {
                  mDirectionPressed.Remove(BACKWARD);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_a)
              {
                  mDirectionPressed.Remove(LEFT);
                  return true;
              }
              else if(e.KeyCode() == r2::io::KEY_d)
              {
                  mDirectionPressed.Remove(RIGHT);
                  return true;
              }
              
              return false;
        });

        
        

        dispatcher.Dispatch<r2::evt::MouseButtonPressedEvent>([this](const r2::evt::MouseButtonPressedEvent& e) {

            if (e.MouseButton() == r2::io::MOUSE_BUTTON_RIGHT && !mRightMouseButtonHeld)
            {
                
                mRightMouseButtonHeld = true;
            }


            return true;
        });

        dispatcher.Dispatch<r2::evt::MouseButtonReleasedEvent>([this](const r2::evt::MouseButtonReleasedEvent& e) {
                
			if (e.MouseButton() == r2::io::MOUSE_BUTTON_RIGHT && mRightMouseButtonHeld)
			{
				
				mRightMouseButtonHeld = false;
			}

            return true;
        });

        dispatcher.Dispatch<r2::evt::MouseMovedEvent>([this](const r2::evt::MouseMovedEvent& e) {

            if (!mRightMouseButtonHeld)
            {
				lastX = (f32)e.X();
				lastY = (f32)e.Y();
                return true;
            }

            float xOffset = e.X() - lastX;
            float yOffset = lastY - e.Y();
            
            lastX = (f32)e.X();
            lastY = (f32)e.Y();
            
            float sensitivity = 0.5f;
            xOffset *= sensitivity;
            yOffset *= sensitivity;
            
            yaw += xOffset;
            pitch += yOffset;

			if (yaw >= 360.0f)
			{
                yaw -= 360.0f;
			}
			if (yaw < 0.0)
			{
                yaw += 360.0f;
			}
            
            if (pitch > 89.0f)
                pitch = 89.0f;
            
            if(pitch < -89.0f)
                pitch = -89.0f;
            
           // R2_LOGI("pitch: %f, yaw: %f", pitch, yaw);
            
            SetFacingDir(*mCameraPtr, pitch, yaw);
            
            return true;
        });

        dispatcher.Dispatch<r2::evt::MouseWheelEvent>([this](const r2::evt::MouseWheelEvent&e){
            
            float fovDegrees = glm::degrees(mCameraPtr->fov);

            if(fovDegrees >= 1.0f && fovDegrees <= 90.0f)
                fovDegrees -= e.YAmount();
            if(fovDegrees <= 1.0f)
                fovDegrees = 1.0f;
			if (fovDegrees >= 90.0f)
                fovDegrees = 90.0f;
            
            SetPerspectiveCam(*mCameraPtr, glm::radians(fovDegrees), mCameraPtr->aspectRatio, mCameraPtr->nearPlane, mCameraPtr->farPlane);
            return true;
        });
    }

	void PerspectiveController::SetCamera(Camera* cameraPtr, const glm::vec3& position, const glm::vec3& facing, float camMoveSpeed)
	{
        R2_CHECK(cameraPtr != nullptr, "Should never happen!");
        
        mCameraPtr = cameraPtr;

		glm::vec3 yawFacing = glm::normalize(glm::vec3(facing.x, facing.y, 0));
		yaw = -glm::degrees(glm::angle(glm::vec3(1, 0, 0), yawFacing));
		pitch = 0.0f;

		if (math::NearZero(glm::dot(facing, glm::vec3(1, 0, 0))))
		{
			pitch = glm::degrees(glm::angle(facing, glm::vec3(0, 1, 0)));
		}
		else
		{
			pitch = glm::degrees(glm::angle(facing, glm::vec3(1, 0, 0)));
		}

		lastX = (f32)CENG.DisplaySize().width / 2.0f;
		lastY = (f32)CENG.DisplaySize().height / 2.0f;

		mCameraMoveSpeed = camMoveSpeed;
		//mFOV = fov; 
		//mAspect = aspect;
		//mNear = near;
		//mFar = far;
		mRightMouseButtonHeld = false;
		InitPerspectiveCam(*mCameraPtr, mCameraPtr->fov, mCameraPtr->aspectRatio, mCameraPtr->nearPlane, mCameraPtr->farPlane, position, facing);
	}

}
