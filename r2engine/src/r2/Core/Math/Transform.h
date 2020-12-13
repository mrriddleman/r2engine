#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#define GLM_FORCE_INLINE 
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace r2::math
{
	struct Transform
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = { 0,0,0,1 };
	};

	Transform Combine(const Transform& a, const Transform& b);
	Transform Inverse(const Transform& t);
	Transform Mix(const Transform& a, const Transform& b, float t);
	glm::mat4 ToMatrix(const Transform& t);
	Transform ToTransform(const glm::mat4& m);
}


#endif // 
