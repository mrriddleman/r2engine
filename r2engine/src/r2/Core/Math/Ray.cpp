//
//  Ray.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-19.
//

#include "Ray.h"

namespace r2::math
{
    Ray CreateRay(const glm::vec3& origin, const glm::vec3& dir)
    {
        //R2_CHECK(glm::length(dir) == 1.0f, "direction should be normalized!");
        Ray aRay;
        aRay.origin = origin;
        aRay.direction = dir;
        return aRay;
    }
}
