#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Game/ECS/Entity.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"

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
			u32 version,
			const char* levelName,
			const char* groupName,
			LevelHandle levelHandle,
			r2::SArray<r2::asset::AssetHandle>* modelAssets,
			r2::SArray<r2::asset::AssetHandle>* animationAssets,
			r2::SArray<r2::mat::MaterialName>* materials,
			r2::SArray<ecs::Entity>* entities);

		void Shutdown();

		LevelHandle GetLevelHandle() const;

		const char* GetLevelName() const;
		LevelName GetLevelAssetName() const;
		void SetLevelName(const char* levelName);

		const char* GetGroupName() const;
		LevelName GetGroupHashName() const;
		void SetGroupName(const char* groupName);

		u32 GetVersion() const;
		void SetVersion(u32 version);

		r2::SArray<r2::asset::AssetHandle>* GetModelAssets() const;
		r2::SArray<r2::asset::AssetHandle>* GetAnimationAssets() const;
		r2::SArray<r2::mat::MaterialName>* GetMaterials() const;
		r2::SArray<ecs::Entity>* GetEntities() const;

		void AddEntity(ecs::Entity e) const;
		void RemoveEntity(ecs::Entity e) const;
		void ClearAllEntities() const;

		static u64 MemorySize(u32 numModelAssets, u32 numAnimationAssets, u32 numTexturePacks, u32 numEntities, const r2::mem::utils::MemoryProperties& memoryProperties);

	private:
		friend class LevelManager;

		mutable LevelHandle mLevelHandle;
		
		u32 mVersion;
		char mLevelName[r2::fs::FILE_PATH_LENGTH];
		char mGroupName[r2::fs::FILE_PATH_LENGTH];

		//we're going to add in arrays for each type of asset handle
		r2::SArray<r2::asset::AssetHandle>* mModelAssets;
		r2::SArray<r2::asset::AssetHandle>* mAnimationAssets;
		//we'll need to do something special for the materials/textures
		r2::SArray<r2::mat::MaterialName>* mMaterials;

		//@TODO(Serge): sound files

		r2::SArray<ecs::Entity>* mEntities;
	};
}

#endif // __LEVEL_H__
