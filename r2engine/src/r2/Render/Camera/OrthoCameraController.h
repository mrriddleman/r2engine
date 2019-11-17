//
//  OrthoCameraController.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-16.
//

#ifndef OrthoCameraController_h
#define OrthoCameraController_h

#include "r2/Render/Camera/Camera.h"
#include "r2/Core/Events/Events.h"

namespace r2::cam
{
    class OrthoCameraController
    {
    public:
        void Init(float cameraMoveSpeed, float aspect, float near, float far);
        void Update();
        void OnEvent(evt::Event& e);
        const Camera& GetCamera() const {return mCamera;}
        
    private:
        Camera mCamera;
        
        CameraDirectionPressed mDirectionPressed;
        float mCameraMoveSpeed;
        float mZoom = 1.0f;
        float mAspect;
        float mNear;
        float mFar;
    };
}

#endif /* OrthoCameraController_h */
