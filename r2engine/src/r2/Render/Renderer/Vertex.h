//
//  Vertex.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-13.
//

#ifndef Vertex_h
#define Vertex_h

#include "glm/glm.hpp"

namespace r2::draw
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
    };
    
#define MAX_BONE_WEIGHTS 4
    
    struct BoneData
    {
        glm::vec4 boneWeights = glm::vec4(0.0f);
        glm::ivec4 boneIDs = glm::ivec4(0);
    };
    
    struct SkinnedVertex
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec2 texCoords = glm::vec2(0.0f);
        glm::vec4 boneWeights = glm::vec4(0.0f);
        glm::ivec4 boneIDs = glm::ivec4(0);
    };
    
    enum TextureType
    {
        Diffuse = 0,
        Specular,
        NUM_TEXTURE_TYPES
    };
    
    struct Texture
    {
        u32 texID;
        TextureType type;
        char path[r2::fs::FILE_PATH_LENGTH];
    };
}

#endif /* Vertex_h */
