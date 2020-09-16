//
//  MathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//
#include "r2pch.h"
#include "MathUtils.h"
#include <cmath>

namespace r2::math
{
    const float EPSILON = 0.0001f;
    
    glm::vec3 Lerp(const glm::vec3& v1, const glm::vec3& v2, float t)
    {
        float oneMinusT = (1.0f - t);
        return glm::vec3(
            oneMinusT * v1.x + t * v2.x,
            oneMinusT * v1.y + t * v2.y,
            oneMinusT * v1.z + t * v2.z
        );
    }
    
    glm::vec4 Lerp(const glm::vec4& v1, const glm::vec4& v2, float t)
    {
        float oneMinusT = (1.0f - t);
        return glm::vec4(
                         oneMinusT * v1.x + t * v2.x,
                         oneMinusT * v1.y + t * v2.y,
                         oneMinusT * v1.z + t * v2.z,
                         oneMinusT * v1.w + t * v2.w);
    }
    
    glm::vec4 Vectorize(const glm::quat& q)
    {
        return glm::vec4(q.x, q.y, q.z, q.w);
    }
    
    glm::quat Lerp(const glm::quat& q1, const glm::quat& q2, float t)
    {
        return glm::lerp(q1, q2, t);
    }
    
    glm::quat Slerp(const glm::quat& q1, const glm::quat& q2, float t)
    {
        return glm::slerp(q1, q2, t);
    }
    
    bool NearEq(float v1, float v2)
    {
        return fabsf(v1 - v2) < EPSILON;
    }

    bool NearZero(float v1)
    {
        return fabsf(v1) < EPSILON;
    }
    
    bool GreaterThanOrEq(float v1, float v2)
    {
        return v1 > v2 || NearEq(v1, v2);
    }
    
    bool LessThanOrEq(float v1, float v2)
    {
        return v1 < v2 || NearEq(v1, v2);
    }
}
