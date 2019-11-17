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
#include "r2/Utils/Flags.h"

namespace r2::cam
{
    class PerspectiveController
    {
    public:
        
        void Init(float camMoveSpeed, float fov, float aspect, float near, float far);
        void OnEvent(r2::evt::Event& e);
        void Update();
        const Camera& GetCamera() const {return mCamera;}
    private:
        
        enum CameraDirection
        {
            
            FORWARD = 1 << 0,
            BACKWARD = 1 << 1,
            LEFT = 1 << 2,
            RIGHT = 1 << 3
        };
        
        using CameraDirectionPressed = r2::Flags<u8, u8>;
        
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
