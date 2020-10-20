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
        glm::vec3 texCoords = glm::vec3(0.0f);
        //glm::vec3 tangent;
    };
    

    
}

#endif /* Vertex_h */
