#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Game/ECS/Entity.h"

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

		bool Init(
			const flat::LevelData* levelData,
			LevelHandle levelHandle,
			r2::SArray<r2::asset::AssetHandle>* modelAssets,
			r2::SArray<r2::asset::AssetHandle>* animationAssets,
			r2::SArray<u64>* texturePacks,
			r2::SArray<ecs::Entity>* entities);

		void Shutdown();

		const flat::LevelData* GetLevelData() const;

		LevelHandle GetLevelHandle() const;

		const char* GetLevelName() const;
		LevelName GetLevelAssetName() const;

		const char* GetGroupName() const;
		LevelName GetGroupHashName() const;

		const r2::SArray<r2::asset::AssetHandle>* GetModelAssets() const;
		const r2::SArray<r2::asset::AssetHandle>* GetAnimationAssets() const;
		const r2::SArray<u64>* GetTexturePacks() const;
		const r2::SArray<ecs::Entity>* GetEntities() const;

		void AddEntity(ecs::Entity e) const;
		void RemoveEntity(ecs::Entity e) const;
		void ClearAllEntities() const;

#ifdef R2_EDITOR
		void ResetLevelName(LevelName levelName)const ;
#endif

		static u64 MemorySize(u32 numModelAssets, u32 numAnimationAssets, u32 numTexturePacks, u32 numEntities, const r2::mem::utils::MemoryProperties& memoryProperties);

	private:
		friend class LevelManager;
		const flat::LevelData* mnoptrLevelData;
		mutable LevelHandle mLevelHandle;

		//we're going to add in arrays for each type of asset handle
		r2::SArray<r2::asset::AssetHandle>* mModelAssets;
		r2::SArray<r2::asset::AssetHandle>* mAnimationAssets;
		//we'll need to do something special for the materials/textures
		r2::SArray<u64>* mTexturePackAssets;

		//@TODO(Serge): sound files

		r2::SArray<ecs::Entity>* mEntities;
	};
}

#endif // __LEVEL_H__
