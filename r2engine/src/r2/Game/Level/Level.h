#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetTypes.h"

namespace flat
{
	struct LevelData;
}

namespace r2
{
	using LevelName = u64;
	using LevelHandle = r2::asset::AssetHandle;
	const LevelName INVALID_LEVEL = 0;

	class Level
	{
	public:
		Level();
		~Level();

		bool Init(const flat::LevelData* levelData, LevelHandle levelHandle);
		void Shutdown();

		const flat::LevelData* GetLevelData() const;

		LevelHandle GetLevelHandle() const;

		const char* GetLevelName() const;
		LevelName GetLevelHashName() const;

		const char* GetGroupName() const;
		LevelName GetGroupHashName() const;

		static u64 MemorySize(const r2::mem::utils::MemoryProperties& memoryProperties);

	private:
		const flat::LevelData* mnoptrLevelData;
		LevelHandle mLevelHandle;
	};
}

#endif // __LEVEL_H__
