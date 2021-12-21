//
//  Camera.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//
#include "r2pch.h"
#include "r2/Render/Camera/Camera.h"

#include "glm/gtc/matrix_transform.hpp"
#include "r2/Platform/Platform.h"

namespace r2::cam
{

    void CalculateFrustumProjections(Camera& cam)
    {
        float frustumSplits[NUM_FRUSTUM_SPLITS];
        GetFrustumSplits(cam, frustumSplits);

        for (u32 i = 0; i < NUM_FRUSTUM_SPLITS; ++i)
        {
            if (i == 0)
            {
                cam.frustumProjections[i] = glm::perspective(cam.fov, cam.aspectRatio, cam.nearPlane, frustumSplits[i]);
            }
            else
            {
                cam.frustumProjections[i] = glm::perspective(cam.fov, cam.aspectRatio, frustumSplits[i - 1], frustumSplits[i]);
            }
        }
    }
    
    void InitPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far, const glm::vec3& pos, const glm::vec3& facing)
    {
        cam.position = pos;
        cam.facing = facing;
        cam.view = glm::lookAt(pos, pos + cam.facing, cam.up);
       
        SetPerspectiveCam(cam, fovDegrees, aspect, near, far);
    }
    
    void InitOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far, const glm::vec3& pos)
    {
        cam.position = pos;
        cam.view = glm::lookAt(pos, pos + cam.facing, cam.up);

        SetOrthoCam(cam, left, right, bottom, top, near, far);
    }

    void SetFOV(Camera& cam, float fovDegrees)
    {
        SetPerspectiveCam(cam, fovDegrees, cam.aspectRatio, cam.nearPlane, cam.farPlane);
    }

    void SetPerspectiveCam(Camera& cam, float fovDegrees, float aspect, float near, float far)
    {
        cam.aspectRatio = aspect;
        cam.fov = glm::radians(fovDegrees);
        cam.nearPlane = near;
        cam.farPlane = far;

        cam.proj = glm::perspective(cam.fov, aspect, near, far);
        cam.vp = cam.proj * cam.view;

        CalculateFrustumProjections(cam);
    }
    
    void SetOrthoCam(Camera& cam, float left, float right, float bottom, float top, float near, float far)
    {
        cam.aspectRatio = (right - left) / abs(bottom - top);
        cam.nearPlane = near;
        cam.farPlane = far;

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

    void GetFrustumCorners(const Camera& cam, glm::vec3 corners[NUM_FRUSTUM_CORNERS])
    {
        return GetFrustumCorners(cam.proj, cam.view, corners);
    }

    void GetFrustumCorners(const glm::mat4& proj, const glm::mat4& view, glm::vec3 outCorners[NUM_FRUSTUM_CORNERS])
    {
		constexpr glm::vec4 frustumCorners[NUM_FRUSTUM_CORNERS] =
		{
			glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
			glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f)
		};

		const auto inv = glm::inverse(proj * view);

		for (u32 i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
		{
			glm::vec4 pt = inv * frustumCorners[i];
			outCorners[i] = pt / pt.w;
		}

		//const auto inv = glm::inverse(proj * view);
  //      s32 i = 0;

		//for (unsigned int x = 0; x < 2; ++x)
		//{
		//	for (unsigned int y = 0; y < 2; ++y)
		//	{
		//		for (unsigned int z = 0; z < 2; ++z)
		//		{
		//			const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
  //                  outCorners[i] = pt / pt.w;
  //                  ++i;
		//		}
		//	}
		//}

		//return frustumCorners;
    }

    glm::vec3 GetFrustumCenter(const glm::vec3 corners[NUM_FRUSTUM_CORNERS])
    {
        glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);

        for (u32 i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
        {
            center += corners[i];
        }

        return center * (1.0f / (float)NUM_FRUSTUM_CORNERS);
    }
    
    void GetFrustumSplits(const Camera& cam, float frustumSplits[NUM_FRUSTUM_SPLITS])
    {
		constexpr float m_pssm_lambda = 0.3;
		constexpr float m_near_offset = 250.0f;

        float ratio = cam.farPlane / cam.nearPlane;

        frustumSplits[0] = 10.f; 
        frustumSplits[1] = 30.f; 
        frustumSplits[2] = 80.f; 
        frustumSplits[3] = cam.farPlane;
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
