//
//  Camera.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#include "r2/Render/Camera/Camera.h"
#include "glm/gtc/matrix_transform.hpp"

namespace r2::cam
{

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
    
}
