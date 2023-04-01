#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"

namespace flat
{
	struct LevelPack;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2
{

	using LevelName = u64;

	const LevelName INVALID_LEVEL = 0;

	class LevelManager
	{
	public:
		LevelManager();
		~LevelManager();

		bool Init(const flat::LevelPack* levelPack);
		void Shutdown();

		LevelName LoadLevel(const char* level, ecs::ECSCoordinator* coordinator);
		void UnloadCurrentLevel(ecs::ECSCoordinator* coordinator);
		void UnloadLevel(LevelName level, ecs::ECSCoordinator* coordinator);

		static u64 MemorySizeForOneLevel(const flat::LevelPack* levelPack, const r2::mem::utils::MemoryProperties& properties);

	private:
		const flat::LevelPack* mnoptrLevelPack;
	};
}

#endif // __LEVEL_MANAGER_H__
