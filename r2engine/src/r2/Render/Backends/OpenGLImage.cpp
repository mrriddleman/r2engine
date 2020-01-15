//
//  OpenGLImage.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#include "OpenGLImage.h"
#include "glad/glad.h"
#include "stb_image.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::draw::opengl
{
    u32 LoadImageTexture(const char* path)
    {
        stbi_set_flip_vertically_on_load(true);
        //load and create texture
        u32 newTex;
        glGenTextures(1, &newTex);
        glBindTexture(GL_TEXTURE_2D, newTex);

        //load the image
        
        s32 texWidth, texHeight, channels;
        u8* data = stbi_load(path, &texWidth, &texHeight, &channels, 0);
        R2_CHECK(data != nullptr, "We didn't load the image!");
        
        GLenum format;
        GLenum internalFormat;
        if (channels == 3)
        {
            internalFormat = GL_SRGB;
            format = GL_RGB;
        }
        else if(channels == 4)
        {
            internalFormat = GL_SRGB_ALPHA;
            format = GL_RGBA;
        }
        else
        {
            format = GL_RGB;
            internalFormat = GL_SRGB;
            R2_CHECK(false, "UNKNOWN image format");
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
        //set the texture wrapping/filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return newTex;
    }
    
    u32 CreateImageTexture(u32 width, u32 height, void* data)
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
    
    u32 CreateCubeMap(const std::vector<std::string>& faces)
    {
        stbi_set_flip_vertically_on_load(false);
        u32 textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        
        s32 width, height, channels;
        byte * data;
        
        for (u32 i = 0; i < faces.size(); ++i)
        {
            char path[r2::fs::FILE_PATH_LENGTH];
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, faces[i].c_str(), path);
            data = stbi_load(path, &width, &height, &channels, 0);
            R2_CHECK(data != nullptr, "Failed to load cubemap face: %s", path);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        return textureID;
    }
    
    u32 CreateDepthCubeMap(u32 width, u32 height)
    {
        u32 depthCubeMap;
        glGenTextures(1, &depthCubeMap);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
        
        for (u32 i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        return depthCubeMap;
    }
}
