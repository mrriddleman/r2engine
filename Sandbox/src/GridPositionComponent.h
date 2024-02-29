#ifndef __GRID_POSITION_COMPONENT_H__
#define __GRID_POSITION_COMPONENT_H__

#include <glm/glm.hpp>

struct GridPositionComponent
{
	glm::ivec3 localGridPosition;
	glm::ivec3 globalGridPosition;
};

#endif // __GRID_POSITION_COMPONENT_H__
