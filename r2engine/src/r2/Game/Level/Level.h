#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Game/ECS/Entity.h"

namespace flat
{
	struct LevelData;
}

namespace r2
{
	using LevelName = u64;

	const LevelName INVALID_LEVEL = 0;

	class Level
	{
	public:
		Level();
		~Level();

		bool Init(flat::LevelData* levelData, r2::SArray<ecs::Entity>* entityArray);
		void Shutdown();

		flat::LevelData* GetLevelData() const;
		r2::SArray<ecs::Entity>* GetLevelEntities();

		const char* GetLevelName() const;
		LevelName GetLevelHashName() const;

		const char* GetGroupName() const;
		LevelName GetGroupHashName() const;

		static u64 MemorySize(u32 numEntitiesInLevel, const r2::mem::utils::MemoryProperties& memoryProperties);

	private:
		flat::LevelData* mnoptrLevelData;
		r2::SArray<ecs::Entity>* mnoptrEntitiesForLevel;
	};
}

#endif // __LEVEL_H__
