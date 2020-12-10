//
//  MathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//
#include "r2pch.h"
#include "MathUtils.h"


namespace r2::math
{   
	template <typename T>

	inline T lerp(T v0, T v1, T t) {
		return fma(t, v1, fma(-t, v0, v0));
	}

    glm::vec3 Lerp(const glm::vec3& v1, const glm::vec3& v2, float t)
    {
		return glm::vec3(
			lerp(v1.x, v2.x, t),
			lerp(v1.y, v2.y, t),
			lerp(v1.z, v2.z, t)
		);
    }
    
    glm::vec4 Lerp(const glm::vec4& v1, const glm::vec4& v2, float t)
    {
        return glm::vec4(
                            lerp(v1.x, v2.x, t),
                            lerp(v1.y, v2.y, t),
                            lerp(v1.z, v2.z, t),
                            lerp(v1.w, v2.w, t));
    }
    
    glm::vec4 Vectorize(const glm::quat& q)
    {
        return glm::vec4(q.x, q.y, q.z, q.w);
    }
    
    glm::quat Lerp(const glm::quat& q1, const glm::quat& q2, float t)
    {
        return glm::lerp(q1, q2, t);
    }

    glm::quat Interpolate(const glm::quat& q1, const glm::quat& q2, float t)
    {
        glm::quat result = Lerp(q1, q2, t);
        if (glm::dot(q1, q2) < 0)
        {
            return Lerp(q1, -q2, t);
        }

        return glm::normalize(result);
    }

    glm::vec3 Interpolate(const glm::vec3& v1, const glm::vec3& v2, float t)
    {
        return Lerp(v1, v2, t);
    }

    float Interpolate(float a, float b, float t)
    {
        return lerp(a, b, t);
    }
    
    glm::quat Slerp(const glm::quat& q1, const glm::quat& q2, float t)
    {
        return glm::slerp(q1, q2, t);
    }

    glm::quat NLerp(const glm::quat& from, const glm::quat& to, float t)
    {
        return glm::normalize(from + (to - from) * t);
    }
    
}
