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
        explicit RenderLayer(const char* shaderManifestPath, const char* internalShaderManifestPath);
        
        virtual void Init() override;
        virtual void Render(float alpha) override;
        virtual void OnEvent(evt::Event& event) override;
        virtual void Shutdown() override;
        virtual void Update() override;
    private:
        
        //ensure that is always valid
        char mShaderManifestPath[r2::fs::FILE_PATH_LENGTH];
        char mInternalShaderManifestPath[r2::fs::FILE_PATH_LENGTH];
    };
}

#endif /* RenderLayer_h */
