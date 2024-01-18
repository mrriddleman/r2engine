//
//  MathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-16.
//
#include "r2pch.h"
#include "MathUtils.h"
#include "assetlib/RModel_generated.h"

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
        return glm::quat(
            lerp(q1.w, q2.w, t),
            lerp(q1.x, q2.x, t),
            lerp(q1.y, q2.y, t),
            lerp(q1.z, q2.z, t)
        );//glm::lerp(q1, q2, t);
    }

    glm::quat Interpolate(const glm::quat& q1, const glm::quat& q2, float t)
    {
        //glm::quat result = Lerp(q1, q2, t);
        //if (glm::dot(q1, q2) < 0)
        //{
        //    return Lerp(q1, -q2, t);
        //}
        //
        return glm::normalize(Slerp(q1, q2, t));
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

	glm::mat4 GetGLMMatrix4FromFlatMatrix(const flat::Matrix4* mat)
	{
		glm::mat4 glmMat;

		glmMat[0] = glm::vec4(mat->cols()->Get(0)->v()->Get(0), mat->cols()->Get(0)->v()->Get(1), mat->cols()->Get(0)->v()->Get(2), mat->cols()->Get(0)->v()->Get(3));
		glmMat[1] = glm::vec4(mat->cols()->Get(1)->v()->Get(0), mat->cols()->Get(1)->v()->Get(1), mat->cols()->Get(1)->v()->Get(2), mat->cols()->Get(1)->v()->Get(3));
		glmMat[2] = glm::vec4(mat->cols()->Get(2)->v()->Get(0), mat->cols()->Get(2)->v()->Get(1), mat->cols()->Get(2)->v()->Get(2), mat->cols()->Get(2)->v()->Get(3));
		glmMat[3] = glm::vec4(mat->cols()->Get(3)->v()->Get(0), mat->cols()->Get(3)->v()->Get(1), mat->cols()->Get(3)->v()->Get(2), mat->cols()->Get(3)->v()->Get(3));

		return glmMat;
	}


}
