#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/LevelPackManifestAssetFile.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Core/Assets/AssetBuffer.h"

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#endif

namespace r2::asset
{

	LevelPackManifestAssetFile::LevelPackManifestAssetFile()
		:mLevelPackManifest(nullptr)
	{

	}

	LevelPackManifestAssetFile::~LevelPackManifestAssetFile()
	{

	}

	bool LevelPackManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();
		if (!success)
		{
			R2_CHECK(false, "Failed to load the manifest file: %s\n", FilePath());
			return false;
		}

		mLevelPackManifest = flat::GetLevelPackData(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mLevelPackManifest != nullptr, "Should never happen");

#ifdef R2_ASSET_PIPELINE
		FillLevelPackVector();
#endif

		return success && !mLevelPackManifest;
	}

	bool LevelPackManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mLevelPackManifest = nullptr;

#ifdef R2_ASSET_PIPELINE
		mLevelPack.clear();
#endif

		return success;
	}

	bool LevelPackManifestAssetFile::HasAsset(const Asset& asset) const
	{
		R2_CHECK(mLevelPackManifest != nullptr, "Should never happen");

		const auto* flatLevelGroups = mLevelPackManifest->levelGroups();

		//We're splitting this on type to reduce operations - if possible
		if (asset.GetType() == r2::asset::LEVEL_GROUP)
		{
			for (flatbuffers::uoffset_t i = 0; i < flatLevelGroups->size(); ++i)
			{
				const auto* levelGroup = flatLevelGroups->Get(i);

				//@TODO(Serge): change this to UUID
				if (levelGroup->assetName()->assetName() == asset.HashID())
				{
					return true;
				}
			}
		}
		else if (asset.GetType() == r2::asset::LEVEL)
		{
			for (flatbuffers::uoffset_t i = 0; i < flatLevelGroups->size(); ++i)
			{
				const auto* levelGroup = flatLevelGroups->Get(i);
				const auto numLevels = levelGroup->levels()->size();

				for (flatbuffers::uoffset_t j = 0; j < numLevels; ++j)
				{
					const auto* level = levelGroup->levels()->Get(j);

					//@TODO(Serge): change this to UUID
					if (level->assetName()->assetName() == asset.HashID())
					{
						return true;
					}
				}
			}
		}

		return false;
	}

#ifdef R2_ASSET_PIPELINE
	bool LevelPackManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		//first we need to figure out the group name
		std::filesystem::path binPath = assetReference.binPath;

		std::string groupName = binPath.parent_path().stem().string();

		std::string lowerCaseGroupName = groupName;

		std::transform(std::begin(lowerCaseGroupName), std::end(lowerCaseGroupName), std::begin(lowerCaseGroupName), (int(*)(int))std::tolower);
		bool found = false;
		for (size_t i = 0; i < mLevelPack.size(); ++i)
		{
			std::string levelPackGroup = mLevelPack[i].groupName;

			std::transform(std::begin(levelPackGroup), std::end(levelPackGroup), std::begin(levelPackGroup), (int(*)(int))std::tolower);

			if (levelPackGroup == lowerCaseGroupName)
			{
				mLevelPack[i].levelReferences.push_back(assetReference);
				found = true;
				break;
			}
		}

		if (!found)
		{
			pln::LevelGroup newGroup;
			newGroup.groupName = groupName;
			newGroup.levelReferences.push_back(assetReference);
			mLevelPack.push_back(newGroup);
		}

		return SaveManifest(); //not sure if we should be saving here?
	}

	bool LevelPackManifestAssetFile::SaveManifest()
	{
		//save the manifest
		pln::SaveLevelPackData(1, mLevelPack, FilePath(), mRawPath);

		ReloadManifestFile(false);

		return mLevelPackManifest != nullptr;
	}

	void LevelPackManifestAssetFile::Reload()
	{
		ReloadManifestFile(true);
	}

	void LevelPackManifestAssetFile::ReloadManifestFile(bool fillVector)
	{
		ManifestAssetFile::Reload();

		mLevelPackManifest = flat::GetLevelPackData(mManifestCacheRecord.GetAssetBuffer()->Data());

		if (fillVector)
		{
			FillLevelPackVector();
		}
	}

	void LevelPackManifestAssetFile::FillLevelPackVector()
	{
		mLevelPack.clear();
		R2_CHECK(mLevelPackManifest != nullptr, "Should never happen");
		const auto* flatLevelGroups = mLevelPackManifest->levelGroups();

		const auto numGroups = flatLevelGroups->size();

		mLevelPack.reserve(numGroups);

		for (flatbuffers::uoffset_t i = 0; i < numGroups; ++i)
		{
			const auto* group = flatLevelGroups->Get(i);
			const auto* levelsInGroups = group->levels();

			const auto numLevels = levelsInGroups->size();

			pln::LevelGroup newGroup;
			newGroup.groupName = group->assetName()->stringName()->str();
			newGroup.levelReferences.reserve(numLevels);

			for (flatbuffers::uoffset_t j = 0; j < numLevels; ++j)
			{
				const flat::AssetRef* levelAssetRef = levelsInGroups->Get(j);

				AssetReference assetReference;
				MakeAssetReferenceFromFlatAssetRef(levelAssetRef, assetReference);

				newGroup.levelReferences.push_back(assetReference);
			}

			mLevelPack.push_back(newGroup);
		}
	}
#endif
}