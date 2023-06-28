#include "r2pch.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelData_generated.h"


namespace r2
{
	Level::Level()
		:mnoptrLevelData(nullptr)
		,mLevelHandle{}
		,mModelAssets(nullptr)
		,mAnimationAssets(nullptr)
		,mTexturePackAssets(nullptr)
		,mEntities(nullptr)
	{
	}

	Level::~Level()
	{
		Shutdown();
	}

	bool Level::Init(
		const flat::LevelData* levelData,
		LevelHandle levelHandle,
		r2::SArray<r2::asset::AssetHandle>* modelAssets,
		r2::SArray<r2::asset::AssetHandle>* animationAssets,
		r2::SArray<u64>* texturePacks,
		r2::SArray<ecs::Entity>* entities)
	{
		R2_CHECK(levelData != nullptr, "levelData is nullptr");
		mnoptrLevelData = levelData;
		mLevelHandle = levelHandle;
		mModelAssets = modelAssets;
		mAnimationAssets = animationAssets;
		mTexturePackAssets = texturePacks;
		mEntities = entities;

		return true;
	}

	void Level::Shutdown()
	{
		mnoptrLevelData = nullptr;
		mLevelHandle = {};
		mModelAssets = nullptr;
		mAnimationAssets = nullptr;
		mTexturePackAssets = nullptr;
		mEntities = nullptr;
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

	r2::LevelName Level::GetLevelAssetName() const
	{
		R2_CHECK(mnoptrLevelData != nullptr, "mnoptrLevelData is nullptr");

		if (!mnoptrLevelData)
		{
			return 0;
		}

		return mLevelHandle.handle;
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

	u64 Level::MemorySize(u32 numModelAssets, u32 numAnimationAssets, u32 numTexturePacks, u32 numEntities, const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetHandle>::MemorySize(numModelAssets), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetHandle>::MemorySize(numAnimationAssets), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u64>::MemorySize(numTexturePacks), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ecs::Entity>::MemorySize(numEntities), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking);
	}

	const r2::SArray<r2::asset::AssetHandle>* Level::GetModelAssets() const
	{
		return mModelAssets;
	}

	const r2::SArray<r2::asset::AssetHandle>* Level::GetAnimationAssets() const
	{
		return mAnimationAssets;
	}

	const r2::SArray<u64>* Level::GetTexturePacks() const
	{
		return mTexturePackAssets;
	}

	const r2::SArray<ecs::Entity>* Level::GetEntities() const
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