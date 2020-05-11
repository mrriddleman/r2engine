//
//  PerspectiveCameraController_h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#ifndef CameraController_h
#define CameraController_h
#include "r2/Render/Camera/Camera.h"
#include "r2/Core/Events/Events.h"


namespace r2::cam
{
    class PerspectiveController
    {
    public:
        
        void Init(float camMoveSpeed, float fov, float aspect, float near, float far, const glm::vec3& position);
        void OnEvent(r2::evt::Event& e);
        void Update();
        
        void SetAspect(float aspect);
        void SetFOV(float fov);
        
        const Camera& GetCamera() const {return mCamera;}
        Camera* GetCameraPtr() { return &mCamera; }
    private:

        Camera mCamera;
        CameraDirectionPressed mDirectionPressed;
        
        float mCameraMoveSpeed;
        float mFOV;
        float mAspect;
        float mNear;
        float mFar;
    };
}

#endif /* PerspectiveCameraController_h */
