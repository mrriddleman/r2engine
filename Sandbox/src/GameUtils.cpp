#include "GameUtils.h"

namespace utils
{
	glm::ivec3 CalculateGridPosition(const glm::vec3& position, const glm::vec3& pivotOffset)
	{
		return static_cast<glm::ivec3>((position - pivotOffset) / (float)constants::GAME_BLOCK_SIZE);
	}

	glm::vec3 CalculateWorldPositionFromGridPosition(const glm::ivec3& position, const glm::vec3& pivotOffset)
	{
		return static_cast<glm::vec3>(position) * (float)constants::GAME_BLOCK_SIZE + pivotOffset;
	}

}