#include "GameUtils.h"

namespace utils
{
	glm::ivec3 CalculateGridPosition(const glm::vec3& position)
	{
		return static_cast<glm::ivec3>(position / (float)constants::GAME_BLOCK_SIZE);
	}

	glm::vec3 CalculateWorldPositionFromGridPosition(const glm::ivec3& position)
	{
		return static_cast<glm::vec3>(position) * (float)constants::GAME_BLOCK_SIZE;
	}

}