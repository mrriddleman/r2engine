//
//  MathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//

#include "MathUtils.h"

namespace r2::math
{
    glm::vec3 Lerp(const glm::vec3& v1, const glm::vec3& v2, float t)
    {
        return glm::vec3(
            (1.0f - t) * v1.x + t * v2.x,
            (1.0f - t) * v1.y + t * v2.y,
            (1.0f - t) * v1.z + t * v2.z
        );
    }
    
    glm::quat Slerp(const glm::quat& q1, const glm::quat& q2, float t)
    {
        return glm::normalize(glm::mix(q1, q2, t));
    }
}
