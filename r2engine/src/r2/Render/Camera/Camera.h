//
//  Camera.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-15.
//

#ifndef Camera_h
#define Camera_h

#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include "r2/Utils/Flags.h"
#include "r2/Core/Math/Ray.h"
#include "r2/Core/Math/MathUtils.h"

namespace r2
{
    struct Camera
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        glm::mat4 vp = glm::mat4(1.0f);
        
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 facing = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f));
        glm::vec3 up = math::GLOBAL_UP;
    };
}

namespace r2::cam
{
    enum CameraDirection
    {
        FORWARD = 1 << 0,
        BACKWARD = 1 << 1,
        LEFT = 1 << 2,
        RIGHT = 1 << 3
    };
    
    using CameraDirectionPressed = r2::Flags<u8, u8>;
    
    void InitPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far, const glm::vec3& position);
    
    void InitOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far, const glm::vec3& position);
    
    void SetPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far);
    void SetOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far);
    void MoveCameraTo(Camera& cam, const glm::vec3& pos);
    void MoveCameraBy(Camera& cam, const glm::vec3& offset);
    void SetFacingDir(Camera& cam, float pitch, float yaw);
    void SetFacingDir(Camera& cam, const glm::vec3& facingDir);
    
    glm::vec3 CalculateFacingDirection(float pitch, float yaw, const glm::vec3& upDir);
    
    r2::math::Ray CalculateRayFromMousePosition(const Camera& cam, u32 mouseX, u32 mouseY);
}

#endif /* Camera_h */
