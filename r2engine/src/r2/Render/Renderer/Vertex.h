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
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec2 texCoords = glm::vec2(0.0f);
        //glm::vec3 tangent;
    };
    
#define MAX_BONE_WEIGHTS 4
    
    struct BoneData
    {
        glm::vec4 boneWeights = glm::vec4(0.0f);
        glm::ivec4 boneIDs = glm::ivec4(0);
    };
    
}

#endif /* Vertex_h */
