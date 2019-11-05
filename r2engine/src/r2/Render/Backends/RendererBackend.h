//
//  RendererBackend.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#ifndef RendererBackend_h
#define RendererBackend_h

namespace r2::draw
{
    //Temp
    void OpenGLInit();
    void OpenGLDraw(float alpha);
    void OpenGLShutdown();
    void OpenGLResizeWindow(u32 width, u32 height);
    
    
}

#endif /* RendererBackend_h */
