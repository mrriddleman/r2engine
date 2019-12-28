//
//  OpenGLImage.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#ifndef OpenGLImage_h
#define OpenGLImage_h

namespace r2::draw::opengl
{
    u32 LoadImageTexture(const char* path);
    u32 CreateImageTexture(u32 width, u32 height, void* data);
}

#endif /* OpenGLImage_h */
