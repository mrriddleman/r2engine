#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "r2/Game/ECS/Entity.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Utils/Utils.h"

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
		virtual void Update() {}
		virtual void Render() {}
	};
}

#endif // __SYSTEM_H__
