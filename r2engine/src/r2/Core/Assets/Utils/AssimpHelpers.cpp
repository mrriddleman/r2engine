#include "r2pch.h"
#include "r2/Core/Assets/Utils/AssimpHelpers.h"


glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat)
{
	//result[col][row]
	//mat[row][col] a, b, c, d - rows
	//              1, 2, 3, 4 - cols
	glm::mat4 result = glm::mat4();

	result[0][0] = mat.a1; result[1][0] = mat.a2; result[2][0] = mat.a3; result[3][0] = mat.a4;
	result[0][1] = mat.b1; result[1][1] = mat.b2; result[2][1] = mat.b3; result[3][1] = mat.b4;
	result[0][2] = mat.c1; result[1][2] = mat.c2; result[2][2] = mat.c3; result[3][2] = mat.c4;
	result[0][3] = mat.d1; result[1][3] = mat.d2; result[2][3] = mat.d3; result[3][3] = mat.d4;

	return result;
}

glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D)
{
	return glm::vec3(vec3D.x, vec3D.y, vec3D.z);
}

glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat)
{
	return glm::quat(quat.w, quat.x, quat.y, quat.z);
}

r2::math::Transform AssimpMat4ToTransform(const aiMatrix4x4& mat)
{
	return r2::math::ToTransform(AssimpMat4ToGLMMat4(mat));
}