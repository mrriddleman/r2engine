//
//  RendererUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#include "r2/Render/Renderer/RendererUtils.h"
#include "r2/Render/Backends/RendererBackend.h"

namespace r2::draw::utils
{
    u32 LoadImageTexture(const char* path)
    {
        //@TODO(Serge): make it so we don't call this directly
        return OpenGLLoadImageTexture(path);
    }
    
    u32 CreateImageTexture(u32 width, u32 height, void* data)
    {
        //@TODO(Serge): make it so we don't call this directly
        return OpenGLCreateImageTexture(width, height, data);
    }
}
