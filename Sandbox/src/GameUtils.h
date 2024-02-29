#ifndef __GAME_UTILS_H__
#define __GAME_UTILS_H__

#include <glm/glm.hpp>
namespace constants
{
	constexpr unsigned int GAME_BLOCK_SIZE = 2;
}

namespace utils
{
	glm::ivec3 CalculateGridPosition(const glm::vec3& position);
}

#endif // __GAME_UTILS_H__
