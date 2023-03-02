#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	struct System
	{
		r2::SArray<Entity>* mEntities;
	};
}

#endif // __SYSTEM_H__
