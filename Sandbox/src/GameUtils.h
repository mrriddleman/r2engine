#ifndef __GAME_UTILS_H__
#define __GAME_UTILS_H__

#include <glm/glm.hpp>
namespace constants
{
	constexpr unsigned int GAME_BLOCK_SIZE = 2;
}

namespace utils
{
	glm::ivec3 CalculateGridPosition(const glm::vec3& position, const glm::vec3& pivotOffset);
	glm::vec3 CalculateWorldPositionFromGridPosition(const glm::ivec3& position, const glm::vec3& pivotOffset);
}

#endif // __GAME_UTILS_H__