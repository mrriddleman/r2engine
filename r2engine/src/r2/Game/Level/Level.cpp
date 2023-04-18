#include "r2pch.h"
#include "r2/Game/Level/Level.h"

#include "r2/Game/ECS/ECSCoordinator.h"


namespace r2
{
	Level::Level()
		:mnoptrLevelData(nullptr)
		,mLevelHandle{}
	{
	}

	Level::~Level()
	{
		Shutdown();
	}

	bool Level::Init(const flat::LevelData* levelData, LevelHandle levelHandle)
	{
		R2_CHECK(levelData != nullptr, "levelData is nullptr");
		mnoptrLevelData = levelData;
		mLevelHandle = levelHandle;
		return true;
	}

	void Level::Shutdown()
	{
		mnoptrLevelData = nullptr;
		mLevelHandle = {};
	}

	const flat::LevelData* Level::GetLevelData() const
	{
		return mnoptrLevelData;
	}

	LevelHandle Level::GetLevelHandle() const
	{
		return mLevelHandle;
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

	u64 Level::MemorySize(const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking);
	}

}