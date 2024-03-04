#ifndef __MOVE_UPDATE_COMPONENT_H__
#define __MOVE_UPDATE_COMPONENT_H__

#include <glm/glm.hpp>

enum eMovementType
{
	MOVE_NONE = 0,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_UP,
	MOVE_DOWN
};


struct MoveUpdateComponent
{
	//@TODO(Serge): this may have to have an array of all of these

	glm::vec3 oldFacing;
	glm::vec3 newFacing;
	float facingTransitionTime;

	eMovementType movementType;

	glm::ivec3 startingGridPosition; //local?
	glm::ivec3 endGridPosition; //local?

	float gridMovementSpeed; //grid blocks per second

	float gridMovementTotalMovementTimeNeeded; //ms
	float gridMovementCurrentTime; //ms

	b32 hasArrived;
};


#endif