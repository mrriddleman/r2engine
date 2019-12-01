//
//  RendererBackend.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#ifndef RendererBackend_h
#define RendererBackend_h

#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"

namespace r2
{
    struct Camera;
};

namespace r2::math
{
    struct Ray;
}

namespace r2::draw
{
    //Temp
    void OpenGLInit();
    void OpenGLDraw(float alpha);
    void OpenGLShutdown();
    void OpenGLResizeWindow(u32 width, u32 height);
    void OpenGLSetCamera(const r2::Camera& cam);
    void OpenGLDrawRay(const r2::math::Ray& ray);

    
}

#endif /* RendererBackend_h */
