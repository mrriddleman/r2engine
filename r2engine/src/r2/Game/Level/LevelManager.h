#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"

namespace flat
{
	struct LevelPackData;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2
{
	struct LevelCache;
	class GameAssetManager;

	using LevelGroup = r2::SArray<Level*>*;

	class LevelManager
	{
	public:

		static const u32 MODEL_SYSTEM_SIZE;
		static const u32 ANIMATION_CACHE_SIZE;
		static const u32 MAX_NUM_MODELS;
		static const u32 MAX_NUM_ANIMATIONS;
		static const u32 MAX_NUM_TEXTURE_PACKS;
		static const u32 MAX_NUM_SOUND_BANKS;

		LevelManager();
		~LevelManager();

		bool Init(
			r2::mem::MemoryArea::Handle memoryAreaHandle,
			const char* areaName,
			u32 maxNumLevels);
		void Shutdown();

		Level* MakeNewLevel(const char* levelNameStr, const char* groupName, LevelName levelName, const r2::Camera& defaultCamera);

		Level* LoadLevel(const char* levelURI);
		Level* LoadLevel(LevelName levelName);

		Level* GetLevel(const char* levelURI);
		Level* GetLevel(LevelName levelName);

		void UnloadLevel(const Level* level);

		bool IsLevelLoaded(LevelName levelName);
		bool IsLevelLoaded(const char* levelURI);

		Level* ReloadLevel(const char* levelURI);
		Level* ReloadLevel(LevelName levelName);

		static LevelName MakeLevelNameFromPath(const char* levelPath);
		static u64 MemorySize(
			u32 maxNumLevels,
			u32 maxNumModels,
			u32 maxNumAnimations,
			u32 maxNumTexturePacks,
			u32 maxNumSoundBanks,
			u32 maxNumEntities,
			const r2::mem::utils::MemoryProperties& memProperties);

#ifdef R2_ASSET_PIPELINE
		void ImportSoundToLevel(Level* level, const r2::asset::AssetName& assetName);
		void ImportModelToLevel(Level* level, const r2::asset::AssetName& assetName);
		void ImportMaterialToLevel(Level* level, const r2::mat::MaterialName& materialName);

		void ImportMaterialsToLevel(Level* level, const std::vector<r2::mat::MaterialName>& materials);
		void ImportModelsToLevel(Level* level, const std::vector<r2::asset::AssetName>& models);
		void ImportSoundsToLevel(Level* level, const std::vector<r2::asset::AssetName>& sounds);
#endif

		void SetCurrentLevel(LevelName level);
		Level* GetCurrentLevel();
	private:
		
		Level* FindLoadedLevel(LevelName levelname, s32& index);
		void LoadLevelData(Level& level, const flat::LevelData* levelData);
		void UnLoadLevelData(const Level& level);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mMaxNumLevels;
		
		r2::mem::StackArena* mArena;
		r2::mem::FreeListArena* mLevelArena;

		r2::SArray<Level>* mLoadedLevels;

		s32 mCurrentLevel;
	};
}

#endif // __LEVEL_MANAGER_H__
