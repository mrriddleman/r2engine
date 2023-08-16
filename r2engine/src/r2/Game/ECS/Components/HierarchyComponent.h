#ifndef __HEIRARCHY_COMPONENT_H__
#define __HEIRARCHY_COMPONENT_H__

#include "r2/Game/ECS/Entity.h"
#include "r2/Core/Math/Transform.h"

namespace r2::ecs
{
	struct HierarchyComponent
	{
		Entity parent = INVALID_ENTITY;
	};
}

#endif // __HEIRARCHY_COMPONENT_H__
