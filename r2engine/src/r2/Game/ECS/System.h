#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	class ECSCoordinator;
	static const u32 MAX_NUM_SYSTEMS = 128;

	struct System
	{
		r2::SArray<Entity>* mEntities = nullptr;
		bool mKeepSorted = false;
		ECSCoordinator* mnoptrCoordinator = nullptr;

		virtual s32 FindSortedPlacement(Entity e) { return -1; }
	};
}

#endif // __SYSTEM_H__
