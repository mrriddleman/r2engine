//
//  Camera.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-15.
//

#ifndef Camera_h
#define Camera_h

#include "glm/glm.hpp"

namespace r2
{
    struct Camera
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        glm::mat4 vp = glm::mat4(1.0f);
        
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 facing = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    };
}

namespace r2::cam
{
    void SetPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far);
    void SetOrthoCam(Camera& cam, float left, float right, float bottom, float top);
    void MoveCameraTo(Camera& cam, const glm::vec3& pos);
    void MoveCameraBy(Camera& cam, const glm::vec3& offset);
    void SetFacingDir(Camera& cam, float pitch, float yaw);
    void SetFacingDir(Camera& cam, const glm::vec3& facingDir);
    
    glm::vec3 CalculateFacingDirection(float pitch, float yaw, const glm::vec3& upDir);
}

#endif /* Camera_h */
