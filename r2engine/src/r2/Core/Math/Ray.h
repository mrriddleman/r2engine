//
//  Ray.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-19.
//

#ifndef Ray_h
#define Ray_h

#include "glm/glm.hpp"

namespace r2::math
{
    struct Ray
    {
        glm::vec3 direction;
        glm::vec3 origin;
    };
    
    Ray CreateRay(const glm::vec3& origin, const glm::vec3& dir);
    
}

#endif /* Ray_h */
