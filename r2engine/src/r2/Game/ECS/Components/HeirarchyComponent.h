#ifndef __HEIRARCHY_COMPONENT_H__
#define __HEIRARCHY_COMPONENT_H__

#include "r2/Game/ECS/Entity.h"
#include "r2/Core/Math/Transform.h"

namespace r2::ecs
{
	struct HeirarchyComponent
	{
		Entity parent = INVALID_ENTITY;
		r2::math::Transform worldParentInverse; //...?
	};
}

#endif // __HEIRARCHY_COMPONENT_H__
