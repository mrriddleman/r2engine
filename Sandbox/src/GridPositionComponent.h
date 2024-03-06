#ifndef __GRID_POSITION_COMPONENT_H__
#define __GRID_POSITION_COMPONENT_H__

#include <glm/glm.hpp>

struct GridPositionComponent
{
	glm::ivec3 localGridPosition;
	glm::ivec3 globalGridPosition;
	glm::vec3 pivotOffset; //essentially for pivot point of model - eg. pivot point of cube is in the middle whereas pivot point of character is at the bottom
};

#endif // __GRID_POSITION_COMPONENT_H__
