#include "r2pch.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelData_generated.h"


namespace r2
{
	Level::Level()
		:mVersion(1u)
		,mLevelHandle{}
		,mModelAssets(nullptr)
		,mMaterials(nullptr)
		,mSoundBanks(nullptr)
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
		r2::SArray<r2::asset::AssetName>* modelAssets,
		r2::SArray<r2::mat::MaterialName>* materials,
		r2::SArray<r2::asset::AssetName>* soundBanks,
		r2::SArray<ecs::Entity>* entities,
		const r2::LevelRenderSettings& levelRenderSettings)
	{
		
		mVersion = version;
		r2::util::PathCpy(mLevelName, levelName);
		r2::util::PathCpy(mGroupName, groupName);

		mLevelHandle = levelHandle;
		mModelAssets = modelAssets;
		mMaterials = materials;
		mSoundBanks = soundBanks;
		mEntities = entities;

		mLevelRenderSettings = levelRenderSettings;

		return true;
	}

	void Level::Shutdown()
	{
		
		mLevelHandle = {};
		mModelAssets = nullptr;
		mMaterials = nullptr;
		mSoundBanks = nullptr;
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
		return mLevelHandle;
	}

	const char* Level::GetGroupName() const
	{
		return mGroupName;
	}

	LevelName Level::GetGroupHashName() const
	{
		LevelName groupName = r2::asset::MakeAssetNameFromPath(mGroupName, r2::asset::LEVEL_GROUP);

		return groupName;
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

	u64 Level::MemorySize(u32 numModelAssets, u32 numTexturePacks, u32 numSoundBanks, u32 numEntities, const r2::mem::utils::MemoryProperties& memoryProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetName>::MemorySize(numModelAssets), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::mat::MaterialName>::MemorySize(numTexturePacks), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetName>::MemorySize(numSoundBanks), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ecs::Entity>::MemorySize(numEntities), memoryProperties.alignment, memoryProperties.headerSize, memoryProperties.boundsChecking) +
			LevelRenderSettings::MemorySize(memoryProperties);
	}

	r2::SArray<r2::asset::AssetName>* Level::GetModelAssets() const
	{
		return mModelAssets;
	}

	r2::SArray<r2::mat::MaterialName>* Level::GetMaterials() const
	{
		return mMaterials;
	}

	r2::SArray<r2::asset::AssetName>* Level::GetSoundBankAssetNames() const
	{
		return mSoundBanks;
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

	bool Level::AddCamera(const r2::Camera& camera)
	{
		return mLevelRenderSettings.AddCamera(camera);
	}

	bool Level::SetCameraIndex(s32 index)
	{
		return mLevelRenderSettings.SetCurrentCamera(index);
	}

	r2::Camera* Level::GetCurrentCamera()
	{
		return mLevelRenderSettings.GetCurrentCamera();
	}

}