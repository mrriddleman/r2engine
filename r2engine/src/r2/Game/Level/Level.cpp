#include "r2pch.h"
#include "r2/Game/Level/Level.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "R2/Game/Level/LevelData_generated.h"

namespace r2
{
	Level::Level()
		:mnoptrCoordinator(nullptr)
	{

	}

	Level::~Level()
	{

	}

	bool Level::Init(flat::LevelData* levelData, ecs::ECSCoordinator* coordinator)
	{
		mnoptrCoordinator = coordinator;
		return 0;
	}

	u64 Level::MemorySize(flat::LevelData* levelData, const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return 0;
	}

}