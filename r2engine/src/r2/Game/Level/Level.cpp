#include "r2pch.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelData_generated.h"


namespace r2
{
	Level::Level()
		:mVersion(1u)
		,mLevelHandle{}
		,mModelAssets(nullptr)
		,mAnimationAssets(nullptr)
		,mMaterials(nullptr)
		,mEntities(nullptr)
	{
		r2::util::PathCpy(mLevelName, "");
		r2::util::PathCpy(mGroupName, "");
	}

	Level::~Level()
	{
		Shutdown();
	}

	bool Level::Init(
		u32 version,
		const char* levelName,
		const char* groupName,
		LevelHandle levelHandle,
		r2::SArray<r2::asset::AssetHandle>* modelAssets,
		r2::SArray<r2::asset::AssetHandle>* animationAssets,
		r2::SArray<r2::mat::MaterialName>* materials,
		r2::SArray<ecs::Entity>* entities)
	{
		
		mVersion = version;
		r2::util::PathCpy(mLevelName, levelName);
		r2::util::PathCpy(mGroupName, groupName);

		mLevelHandle = levelHandle;
		mModelAssets = modelAssets;
		mAnimationAssets = animationAssets;
		mMaterials = materials;
		mEntities = entities;

		return true;
	}

	void Level::Shutdown()
	{
		
		mLevelHandle = {};
		mModelAssets = nullptr;
		mAnimationAssets = nullptr;
		mMaterials = nullptr;
		mEntities = nullptr;

		r2::util::PathCpy(mLevelName, "");
		r2::util::PathCpy(mGroupName, "");
	}

	LevelHandle Level::GetLevelHandle() const
	{
		return mLevelHandle;
	}

	const char* Level::GetLevelName() const
	{
		return mLevelName;
	}

	void Level::SetLevelName(const char* levelName)
	{
		r2::util::PathCpy(mLevelName, levelName);
	}

	r2::LevelName Level::GetLevelAssetName() const
	{
		return mLevelHandle.handle;
	}

	const char* Level::GetGroupName() const
	{
		return mGroupName;
	}

	LevelName Level::GetGroupHashName() const
	{
		return r2::asset::GetAssetNameForFilePath(mGroupName, r2::asset::LEVEL_GROUP);
	}

	void Level::SetGroupName(const char* groupName)
	{
		r2::util::PathCpy(mGroupName, groupName);
	}

	u32 Level::GetVersion() const
	{
		return mVersion;
	}

	void Level::SetVersion(u32 version)
	{
		mVersion = version;
	}

	u64 Level::MemorySize(u32 numModelAssets, u32 numAnimationAssets, u32 numTexturePacks, u32 numEntities, const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetHandle>::MemorySize(numModelAssets), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetHandle>::MemorySize(numAnimationAssets), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::mat::MaterialName>::MemorySize(numTexturePacks), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ecs::Entity>::MemorySize(numEntities), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking);
	}

	r2::SArray<r2::asset::AssetHandle>* Level::GetModelAssets() const
	{
		return mModelAssets;
	}

	r2::SArray<r2::asset::AssetHandle>* Level::GetAnimationAssets() const
	{
		return mAnimationAssets;
	}

	r2::SArray<r2::mat::MaterialName>* Level::GetMaterials() const
	{
		return mMaterials;
	}

	r2::SArray<ecs::Entity>* Level::GetEntities() const
	{
		return mEntities;
	}

	void Level::AddEntity(ecs::Entity e) const
	{
		r2::sarr::Push(*mEntities, e);
	}

	void Level::RemoveEntity(ecs::Entity e) const
	{
		s64 index = r2::sarr::IndexOf(*mEntities, e);
		if (index != -1)
		{
			r2::sarr::RemoveAndSwapWithLastElement(*mEntities, index);
		}
	}

	void Level::ClearAllEntities() const
	{
		r2::sarr::Clear(*mEntities);
	}
}