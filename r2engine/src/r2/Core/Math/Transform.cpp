#include "r2pch.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Core/Math/MathUtils.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
namespace r2::math
{
	inline glm::quat QuatMult(const glm::quat& Q1, const glm::quat& Q2)
	{
		glm::quat r;

		r.x = Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x;
		r.y = -Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y;
		r.z = Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z;
		r.w = -Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w;

		return r;
	}

	inline glm::vec3 Cross(const glm::vec3& v1, const glm::vec3& v2)
	{
		glm::vec3 r;

		r.x = v1.y * v2.z - v1.z * v2.y;
		r.y = v1.z * v2.x - v1.x * v2.z;
		r.z = v1.x * v2.y - v1.y * v2.x;

		return r;
	}

	inline void Cross(const glm::vec3& v1, const glm::vec3& v2, glm::vec3& r)
	{
		r.x = v1.y * v2.z - v1.z * v2.y;
		r.y = v1.z * v2.x - v1.x * v2.z;
		r.z = v1.x * v2.y - v1.y * v2.x;
	}

	glm::vec3 QuatMult(const glm::quat& q, const glm::vec3& v)
	{
		glm::vec3 qv = { q.x, q.y, q.z };
		glm::vec3 t = {};
		Cross(qv, v, t);

		t.x *= 2.f;
		t.y *= 2.f;
		t.z *= 2.f;
		
		glm::vec3 r = {};
		Cross(qv, t, r);

		t.x *= q.w;
		t.y *= q.w;
		t.z *= q.w;

		t.x += r.x + v.x;
		t.y += r.y + v.y;
		t.z += r.z + v.z;

		return t;
	}

	void Combine(const Transform& a, const Transform& b, Transform& out)
	{
		out.scale.x = a.scale.x * b.scale.x;
		out.scale.y = a.scale.y * b.scale.y;
		out.scale.z = a.scale.z * b.scale.z;

		out.rotation = QuatMult(b.rotation, a.rotation);

		glm::vec3 temp = b.position;

		temp.x *= a.scale.x;
		temp.y *= a.scale.y;
		temp.z *= a.scale.z;

		out.position = QuatMult(a.rotation, temp);

		out.position.x += a.position.x;
		out.position.y += a.position.y;
		out.position.z += a.position.z;
	}

	Transform Combine(const Transform& a, const Transform& b) {
		Transform out;

		out.scale.x = a.scale.x * b.scale.x;
		out.scale.y = a.scale.y * b.scale.y;
		out.scale.z = a.scale.z * b.scale.z;

		out.rotation = QuatMult(b.rotation, a.rotation);

		glm::vec3 temp = b.position;

		temp.x *= a.scale.x;
		temp.y *= a.scale.y;
		temp.z *= a.scale.z;

		out.position = a.rotation * temp;
		
		out.position.x += a.position.x;
		out.position.y += a.position.y;
		out.position.z += a.position.z;

		return out;
	}

	glm::quat Inverse(const glm::quat& q)
	{
		float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
		
		if (lenSq < r2::math::EPSILON) { return glm::quat(0.0f, 0.0f, 0.0f, 1.0f); }
		
		float recip = 1.0f / lenSq;
		
		return glm::quat(-q.x * recip, -q.y * recip, -q.z * recip, q.w * recip);
	}

	Transform Inverse(const Transform& t)
	{
		Transform inv;

		inv.rotation = glm::inverse(t.rotation);
		
		inv.scale.x = fabs(t.scale.x) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.x;
		inv.scale.y = fabs(t.scale.y) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.y;
		inv.scale.z = fabs(t.scale.z) < r2::math::EPSILON ? 0.0f : 1.0f / t.scale.z;

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
		glm::vec3 x = QuatMult(t.rotation, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 y = QuatMult(t.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 z = QuatMult(t.rotation, glm::vec3(0.0f, 0.0f, 1.0f));

		x.x *= t.scale.x;
		x.y *= t.scale.x;
		x.z *= t.scale.x;

		y.x *= t.scale.y;
		y.y *= t.scale.y;
		y.z *= t.scale.y;

		z.x *= t.scale.z;
		z.y *= t.scale.z;
		z.z *= t.scale.z;

		glm::vec3 p = t.position;

		return glm::mat4(glm::vec4(x, 0), glm::vec4(y, 0), glm::vec4(z, 0), glm::vec4(p, 1.0f));
	}

	glm::mat4 QuatToMat4(const glm::quat& q)
	{
		glm::vec3 r = q * glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 u = q * glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 f = q * glm::vec3(0.0f, 0.0f, 1.0f);

		return glm::mat4(
			glm::vec4(r, 0.0f),
			glm::vec4(u, 0.0f),
			glm::vec4(f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);
	}

	glm::quat fromTo(const glm::vec3& from, const glm::vec3& to) {
		glm::vec3 f = glm::normalize(from);
		glm::vec3 t = glm::normalize(to);

		if (f == t) {
			return glm::quat(0, 0, 0, 1);
		}
		else if (f == t * -1.0f) {
			glm::vec3 ortho = glm::vec3(1, 0, 0);
			if (fabsf(f.y) < fabsf(f.x)) {
				ortho = glm::vec3(0, 1, 0);
			}
			if (fabsf(f.z) < fabs(f.y) && fabs(f.z) < fabsf(f.x)) {
				ortho = glm::vec3(0, 0, 1);
			}

			glm::vec3 axis = glm::normalize(cross(f, ortho));
			return glm::quat(axis.x, axis.y, axis.z, 0);
		}

		glm::vec3 half = glm::normalize(f + t);
		glm::vec3 axis = glm::cross(f, half);

		return glm::quat(
			axis.x,
			axis.y,
			axis.z,
			dot(f, half)
		);
	}

	glm::quat lookRotation(const glm::vec3& direction, const glm::vec3& up)
	{
		// Find orthonormal basis vectors
		glm::vec3 f = glm::normalize(direction);// Object Forward
		glm::vec3 u = glm::normalize(up); // Desired Up
		glm::vec3 r = glm::cross(u, f); // Object Right
		u = glm::cross(f, r); // Object Up 
		// From world forward to object forward 
		glm::quat worldToObject = fromTo(glm::vec3(0, 0, 1), f);
		// what direction is the new object up?
		glm::vec3 objectUp = worldToObject * glm::vec3(0, 1, 0);
		
		// From object up to desired up 
		glm::quat u2u = fromTo(objectUp, u);
		// Rotate to forward direction first
		// then twist to correct up
		glm::quat result = worldToObject * u2u;
		// Don't forget to normalize the result
		return glm::normalize(result);
	}

		
	glm::quat Mat4ToQuat(const glm::mat4& m) 
	{ 
		glm::vec3 up = glm::normalize(glm::vec3(m[1].x, m[1].y, m[1].z));
		glm::vec3 forward = glm::normalize(glm::vec3(m[2].x, m[2].y, m[2].z));
		glm::vec3 right = glm::cross(up, forward);
		up = glm::cross(forward, right);
		return lookRotation(forward, up);
	}

	Transform ToTransform(const glm::mat4& mat)
	{
		Transform out;
		
		float m[16] = { 0.0 };

		const float* pSource = (const float*)glm::value_ptr(mat);

		memcpy(m, pSource, sizeof(float) * 16);

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

#ifdef R2_DEBUG
	void PrintTransform(const Transform& t)
	{
		printf("Position - x: %f, y: %f, z: %f\n", t.position.x, t.position.y, t.position.z);
		printf("Scale - x: %f, y: %f, z: %f\n", t.scale.x, t.scale.y, t.scale.z);
		printf("Rotation - x: %f, y: %f, z: %f, w: %f\n", t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w);
	}
#endif
}