//
//  OpenGLImage.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#include "OpenGLImage.h"
#include "glad/glad.h"
#include "stb_image.h"

namespace r2::draw::opengl
{
    u32 OpenGLLoadImageTexture(const char* path)
    {
        stbi_set_flip_vertically_on_load(true);
        //load and create texture
        u32 newTex;
        glGenTextures(1, &newTex);
        glBindTexture(GL_TEXTURE_2D, newTex);
        //set the texture wrapping/filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //load the image
        
        s32 texWidth, texHeight, channels;
        u8* data = stbi_load(path, &texWidth, &texHeight, &channels, 0);
        R2_CHECK(data != nullptr, "We didn't load the image!");
        
        GLenum format;
        if (channels == 3)
        {
            format = GL_RGB;
        }
        else if(channels == 4)
        {
            format = GL_RGBA;
        }
        else
        {
            format = GL_RGB;
            R2_CHECK(false, "UNKNOWN image format");
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return newTex;
    }
    
    u32 OpenGLCreateImageTexture(u32 width, u32 height, void* data)
    {
        u32 newTex;
        glGenTextures(1, &newTex);
        glBindTexture(GL_TEXTURE_2D, newTex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        return newTex;
    }
}
