#ifndef __SELECTION_COMPONENT_H__
#define __SELECTION_COMPONENT_H__

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	struct SelectionComponent
	{
		r2::SArray<s32>* selectedInstances;
	};
}

#endif