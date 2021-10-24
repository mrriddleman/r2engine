//
//  MathUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//

#ifndef MathUtils_h
#define MathUtils_h
#define GLM_FORCE_INLINE 
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <cmath>

namespace r2::math
{
    const float EPSILON = 0.00001f;
    const glm::vec3 GLOBAL_UP = { 0.0f, 0.0f, 1.0f }; //@TODO(serge): change to {0.0f, 0.0f, 1.0f}

    glm::vec3 Lerp(const glm::vec3& v1, const glm::vec3& v2, float t);
    glm::vec4 Lerp(const glm::vec4& v1, const glm::vec4& v2, float t);
    glm::quat Lerp(const glm::quat& q1, const glm::quat& q2, float t);

    glm::quat Interpolate(const glm::quat& q1, const glm::quat& q2, float t);
    glm::vec3 Interpolate(const glm::vec3& v1, const glm::vec3& v2, float t);
    float Interpolate(float a, float b, float t);

    inline void Neighborhood(const float& a, float& b) {}
    inline void Neighborhood(const glm::vec3& a, glm::vec3& b){}
    inline void Neighborhood(const glm::quat& a, glm::quat& b)
    {
        if (glm::dot(a, b) < 0)
        {
            b = -b;
        }
    }

    glm::quat Slerp(const glm::quat& q1, const glm::quat& q2, float t);
    glm::quat NLerp(const glm::quat& q1, const glm::quat& q2, float t);
    
    inline bool NearEq(float v1, float v2) 
    {
        return fabsf(v1 - v2) < EPSILON;
    }

    inline bool NearZero(float v1)
    {
        return fabsf(v1) < EPSILON;
    }

    inline bool GreaterThanOrEq(float v1, float v2)
    {
        return v1 > v2 || NearEq(v1, v2);
    }

    inline bool LessThanOrEq(float v1, float v2)
    {
        return v1 < v2 || NearEq(v1, v2);
    }
}

#endif /* MathUtils_h */
