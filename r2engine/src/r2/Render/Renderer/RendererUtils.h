//
//  RendererUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef RendererUtils_h
#define RendererUtils_h

namespace r2::draw::utils
{
    u32 LoadImageTexture(const char* path);
    u32 CreateImageTexture(u32 width, u32 height, void* data);
}

#endif /* RendererUtils_h */
