//
//  RenderLayer.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#ifndef RenderLayer_h
#define RenderLayer_h

#include "r2/Core/Layer/Layer.h"
#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "r2/Render/Camera/OrthoCameraController.h"

namespace r2
{
    class R2_API RenderLayer: public Layer
    {
    public:
        explicit RenderLayer(const char* shaderManifestPath);
        
        virtual void Init() override;
        virtual void Render(float alpha) override;
        virtual void OnEvent(evt::Event& event) override;
        virtual void Shutdown() override;
        virtual void Update() override;
    private:
        cam::PerspectiveController mPersController;
        //ensure that is always valid
        const char* mShaderManifestPath;

    };
}

#endif /* RenderLayer_h */
