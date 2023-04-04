#include "r2pch.h"
#include "r2/Game/Level/Level.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "R2/Game/Level/LevelData_generated.h"

namespace r2
{
	Level::Level()
		:mnoptrLevelData(nullptr)
		,mnoptrEntitiesForLevel(nullptr)
	{
	}

	Level::~Level()
	{

	}

	bool Level::Init(flat::LevelData* levelData, r2::SArray<ecs::Entity>* entityArray)
	{
		R2_CHECK(levelData != nullptr, "levelData is nullptr");
		mnoptrLevelData = levelData;
		R2_CHECK(entityArray != nullptr, "entityArray is nullptr");
		mnoptrEntitiesForLevel = entityArray;

		return true;
	}

	void Level::Shutdown()
	{
		mnoptrEntitiesForLevel = nullptr;
		mnoptrLevelData = nullptr;
	}

	flat::LevelData* Level::GetLevelData() const
	{
		return mnoptrLevelData;
	}

	r2::SArray<ecs::Entity>* Level::GetLevelEntities()
	{
		return mnoptrEntitiesForLevel;
	}

	const char* Level::GetLevelName() const
	{
		R2_CHECK(mnoptrLevelData != nullptr, "mnoptrLevelData is nullptr");

		if (!mnoptrLevelData)
		{
			return "";
		}

		return mnoptrLevelData->name()->c_str();
	}

	r2::LevelName Level::GetLevelHashName() const
	{
		R2_CHECK(mnoptrLevelData != nullptr, "mnoptrLevelData is nullptr");

		if (!mnoptrLevelData)
		{
			return 0;
		}

		return mnoptrLevelData->hash();
	}

	const char* Level::GetGroupName() const
	{
		R2_CHECK(mnoptrLevelData != nullptr, "mnoptrLevelData is nullptr");

		if (!mnoptrLevelData)
		{
			return "";
		}

		return mnoptrLevelData->groupName()->c_str();
	}

	LevelName Level::GetGroupHashName() const
	{
		R2_CHECK(mnoptrLevelData != nullptr, "mnoptrLevelData is nullptr");

		if (!mnoptrLevelData)
		{
			return 0;
		}

		return mnoptrLevelData->groupHash();
	}

	u64 Level::MemorySize(u32 numEntitiesInLevel, const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ecs::Entity>::MemorySize(numEntitiesInLevel), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking);
	}

}