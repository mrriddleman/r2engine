//
//  Camera.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#include "r2/Render/Camera/Camera.h"

#include "glm/gtc/matrix_transform.hpp"
#include "r2/Platform/Platform.h"

namespace r2::cam
{
    
    void InitPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far, const glm::vec3& pos)
    {
        cam.position = pos;
        cam.view = glm::lookAt(pos, pos + cam.facing, cam.up);
        cam.proj = glm::perspective(glm::radians(fovDegrees), aspect, near, far);
        
        cam.vp = cam.proj * cam.view;
    }
    
    void InitOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far, const glm::vec3& pos)
    {
        cam.position = pos;
        cam.view = glm::lookAt(pos, pos + cam.facing, cam.up);
        cam.proj = glm::ortho(left, right, bottom, top, near, far);
        cam.vp = cam.proj * cam.view;
    }

    void SetPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far)
    {
        cam.proj = glm::perspective(glm::radians(fovDegrees), aspect, near, far);
        cam.vp = cam.proj * cam.view;
    }
    
    void SetOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far)
    {
        cam.proj = glm::ortho(left, right, bottom, top, near, far);
        cam.vp = cam.proj * cam.view;
    }
    
    void MoveCameraTo(Camera& cam, const glm::vec3& pos)
    {
        cam.position = pos;
        
        cam.view = glm::lookAt(pos, pos + cam.facing, cam.up);
        cam.vp = cam.proj * cam.view;
    }
    
    void MoveCameraBy(Camera& cam, const glm::vec3& offset)
    {
        cam.position += offset;
        cam.view = glm::lookAt(cam.position, cam.position + cam.facing, cam.up);
        cam.vp = cam.proj * cam.view;
    }
    
    void SetFacingDir(Camera& cam, float pitch, float yaw)
    {
        cam.facing = CalculateFacingDirection(pitch, yaw, cam.up);
        cam.view = glm::lookAt(cam.position, cam.position + cam.facing, cam.up);
        cam.vp = cam.proj * cam.view;
    }
    
    void SetFacingDir(Camera& cam, const glm::vec3& facingDir)
    {
        cam.facing = facingDir;
        cam.view = glm::lookAt(cam.position, cam.position + cam.facing, cam.up);
        cam.vp = cam.proj * cam.view;
    }
    
    glm::vec3 CalculateFacingDirection(float pitch, float yaw, const glm::vec3& upDir)
    {
        glm::vec3 facingDir(0.0f);
        glm::vec3 yawDir = glm::vec3(1.0f) - upDir;
        facingDir = sin(glm::radians(pitch)) * upDir + cos(glm::radians(pitch))*yawDir;
        
        yawDir += upDir;
        
        if(glm::dot(glm::vec3(0.0f, 1.0f, 0.0), upDir))
        {
            yawDir.x *= cos(glm::radians(yaw));
            yawDir.z *= sin(glm::radians(yaw));
        }
        else if(glm::dot(glm::vec3(0.0f, 0.0f, 1.0f), upDir))
        {
            yawDir.x *= cos(glm::radians(yaw));
            yawDir.y *= -sin(glm::radians(yaw));
        }
        else if(glm::dot(glm::vec3(1.0f, 0.0f, 0.0f), upDir))
        {
            yawDir.z *= cos(glm::radians(yaw));
            yawDir.y *= sin(glm::radians(yaw));
        }
        else
        {
            R2_CHECK(false, "Up direction not supported!");
        }
        
        return glm::normalize(facingDir * yawDir);
    }
    
    r2::math::Ray CalculateRayFromMousePosition(const Camera& cam, u32 mouseX, u32 mouseY)
    {
        float x = (2.0f * mouseX) / static_cast<float>(CENG.DisplaySize().width) - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / static_cast<float>(CENG.DisplaySize().height);
        
        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 rayEye = glm::inverse(cam.proj) * rayClip;
        glm::vec4 rayWorld = glm::inverse(cam.view) * glm::vec4(rayEye.xy(), -1.0, 0);
        
        return r2::math::CreateRay(cam.position, glm::normalize(glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z)));
    }
}
