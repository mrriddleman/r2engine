//
//  RendererBackend.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#ifndef RendererBackend_h
#define RendererBackend_h

#include "glm/glm.hpp"

namespace r2::draw
{
    //Temp
    void OpenGLInit(u32 width, u32 height);
    void OpenGLDraw(float alpha);
    void OpenGLShutdown();
    void OpenGLResizeWindow(u32 width, u32 height);
    
    //@TODO(Serge): Replace with just a function that takes in a camera
    void OpenGLMoveCameraForward(bool pressed);
    void OpenGLMoveCameraBack(bool pressed);
    void OpenGLMoveCameraLeft(bool pressed);
    void OpenGLMoveCameraRight(bool pressed);
    void OpenGLSetCameraFacing(float pitch, float yaw, const glm::vec3& up);
    void OpenGLSetCameraZoom(float zoom);
}

#endif /* RendererBackend_h */
