#ifndef __ASSIMP_HELPERS_H__
#define __ASSIMP_HELPERS_H__

#include <assimp/scene.h>
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat);
glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D);
glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat);

#endif // __ASSIMP_HELPERS_H__
