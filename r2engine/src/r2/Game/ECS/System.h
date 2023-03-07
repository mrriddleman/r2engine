#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	class ECSCoordinator;


	struct System
	{
		r2::SArray<Entity>* mEntities = nullptr;
		bool mKeepSorted = false;
		ECSCoordinator* mnoptrCoordinator = nullptr;
	};
}

#endif // __SYSTEM_H__
