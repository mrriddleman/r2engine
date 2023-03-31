#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Memory/Memory.h"

namespace flat
{
	struct LevelData;
}


namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2
{
	class Level
	{
	public:
		Level();
		~Level();

		bool Init(flat::LevelData* levelData, ecs::ECSCoordinator* coordinator);

		static u64 MemorySize(flat::LevelData* levelData, const r2::mem::utils::MemoryProperties& memoryProperties);

	private:
		ecs::ECSCoordinator* mnoptrCoordinator;
	};
}

#endif // __LEVEL_H__
