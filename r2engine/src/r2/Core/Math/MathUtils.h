//
//  MathUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//

#ifndef MathUtils_h
#define MathUtils_h

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
namespace r2::math
{
    glm::vec3 Lerp(const glm::vec3& v1, const glm::vec3& v2, float t);
    glm::vec4 Lerp(const glm::vec4& v1, const glm::vec4& v2, float t);
    glm::quat Lerp(const glm::quat& q1, const glm::quat& q2, float t);
    glm::quat Slerp(const glm::quat& q1, const glm::quat& q2, float t);
    bool NearEq(float v1, float v2);
    bool GreaterThanOrEq(float v1, float v2);
    bool LessThanOrEq(float v1, float v2);
}

#endif /* MathUtils_h */
