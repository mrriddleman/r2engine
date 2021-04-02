//
//  RendererUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//
#include "r2pch.h"
#include "r2/Render/Renderer/RendererUtils.h"
#include "r2/Render/Backends/OpenGLImage.h"

namespace r2::draw::utils
{
    u32 LoadImageTexture(const char* path)
    {
        //@TODO(Serge): make it so we don't call this directly
        return 0;//opengl::LoadImageTexture(path);
    }
    
    u32 CreateImageTexture(u32 width, u32 height, void* data)
    {
        //@TODO(Serge): make it so we don't call this directly
        return 0;//opengl::CreateImageTexture(width, height, data);
    }
    
    u32 CreateCubeMap(const std::vector<std::string>& faces)
    {
        return 0;// opengl::CreateCubeMap(faces);
    }
}
