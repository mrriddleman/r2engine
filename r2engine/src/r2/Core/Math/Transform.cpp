#include "r2pch.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Core/Math/MathUtils.h"
#include "glm/gtc/type_ptr.hpp"

namespace r2::math
{
	glm::quat QuatMult(const glm::quat& Q1, const glm::quat& Q2)
	{
		glm::quat r;

		r.x = Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x;
		r.y = -Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y;
		r.z = Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z;
		r.w = -Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w;

		return r;
	}

	glm::vec3 QuatMult(const glm::quat& q, const glm::vec3& v)
	{
		glm::vec3 qv = { q.x, q.y, q.z };

		return qv * 2.0f * glm::dot(qv, v) + v * (q.w * q.w - glm::dot(qv, qv)) + glm::cross(qv, v) * 2.0f * q.w;
	}

	Transform Combine(const Transform& a, const Transform& b) {
		Transform out;

		out.scale = a.scale * b.scale;
		out.rotation = QuatMult(b.rotation, a.rotation);

		out.position = a.rotation * ( a.scale * b.position);
		out.position = a.position + out.position;

		return out;
	}

	Transform Inverse(const Transform& t)
	{
		Transform inv;
		inv.rotation = glm::inverse(t.rotation);
		inv.scale.x = fabsf(t.scale.x) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.x;
		inv.scale.y = fabsf(t.scale.y) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.y;
		inv.scale.z = fabsf(t.scale.z) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.z;
		glm::vec3 invTrans = t.position * -1.0f;
		inv.position = inv.rotation * (inv.scale * invTrans);

		return inv;
	}

	Transform Mix(const Transform& a, const Transform& b, float t)
	{
		glm::quat bRot = b.rotation;
		if (glm::dot(a.rotation, bRot) < 0.0f)
		{
			bRot = -bRot;
		}
		return Transform {	r2::math::Lerp(a.position, b.position, t),
							r2::math::Lerp(a.scale, b.scale, t),
							r2::math::NLerp(a.rotation, bRot, t) };
	}

	glm::mat4 ToMatrix(const Transform& t)
	{
		glm::vec3 x = t.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 y = t.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 z = t.rotation * glm::vec3(0.0f, 0.0f, 1.0f);

		x = x * t.scale.x;
		y = y * t.scale.y;
		z = z * t.scale.z;

		glm::vec3 p = t.position;

		return glm::mat4(glm::vec4(x, 0), glm::vec4(y, 0), glm::vec4(z, 0), glm::vec4(p, 1.0f));
	}

	Transform ToTransform(const glm::mat4& mat)
	{
		float m[16] = { 0.0 };

		const float* pSource = (const float*)glm::value_ptr(mat);
		//for (int i = 0; i < 16; ++i)
		///	m[i] = pSource[i];

		memcpy(m, pSource, sizeof(float) * 16);

		Transform out;

		out.position = glm::vec3(m[12], m[13], m[14]);
		out.rotation = glm::quat_cast(mat);

		glm::mat4 rotScaleMat(
			glm::vec4(m[0], m[1], m[2], 0.f),
			glm::vec4(m[4], m[5], m[6], 0.f),
			glm::vec4(m[8], m[9], m[10], 0.f),
			glm::vec4(0.f, 0.f, 0.f, 1.f)
		);

		glm::mat4 invRotMat = glm::mat4_cast(glm::inverse(out.rotation));
		glm::mat4 scaleSkewMat = rotScaleMat * invRotMat;

		pSource = (const float*)glm::value_ptr(scaleSkewMat);

		memcpy(m, pSource, sizeof(float) * 16);

		out.scale = glm::vec3(
			m[0],
			m[5],
			m[10]
		);

		return out;
	}
}