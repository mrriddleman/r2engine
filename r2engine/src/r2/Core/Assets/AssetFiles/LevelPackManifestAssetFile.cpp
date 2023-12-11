#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/LevelPackManifestAssetFile.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#include <glm/glm.hpp> //for max
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
		FillLevelAssetFiles();

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

	AssetFile* LevelPackManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		R2_CHECK(mAssetFiles != nullptr, "We should always have mAssetFiles set for this to work!");
		R2_CHECK(asset.GetType() == r2::asset::LEVEL, "This is the only type that is supported");

		for (u32 i = 0; i < r2::sarr::Size(*mAssetFiles); ++i)
		{
			AssetFile* assetFile = r2::sarr::At(*mAssetFiles, i);
			//@TODO(Serge): UUID
			if (assetFile->GetAssetHandle(0) == asset.HashID())
			{
				return assetFile;
			}
		}

		return nullptr;
	}

	void LevelPackManifestAssetFile::DestroyAssetFiles()
	{
		s32 numFiles = r2::sarr::Size(*mAssetFiles);

		for (s32 i = numFiles - 1; i >= 0; --i)
		{
			RawAssetFile* rawAssetFile = (RawAssetFile*)(r2::sarr::At(*mAssetFiles, i));

			lib::FreeRawAssetFile(rawAssetFile);
		}

		lib::DestoryFileList(mAssetFiles);

		mAssetFiles = nullptr;
	}

	void LevelPackManifestAssetFile::FillLevelAssetFiles()
	{
		R2_CHECK(mAssetFiles == nullptr, "We shouldn't have any asset files yet");
		R2_CHECK(mLevelPackManifest != nullptr, "We haven't initialized the level pack manifest!");
		u32 numLevelFiles = 0;

		//calculate how many levels we have 
		for (flatbuffers::uoffset_t i = 0; i < mLevelPackManifest->levelGroups()->size(); ++i)
		{
			const auto* group = mLevelPackManifest->levelGroups()->Get(i);
			numLevelFiles += group->levels()->size();
		}

#ifdef R2_ASSET_PIPELINE
		numLevelFiles = glm::max(1000u, numLevelFiles); //I dunno just make a lot so we don't run out
#endif

		mAssetFiles = lib::MakeFileList(numLevelFiles);

		R2_CHECK(mAssetFiles != nullptr, "We couldn't create the asset files!");

		for (flatbuffers::uoffset_t i = 0; i < mLevelPackManifest->levelGroups()->size(); ++i)
		{
			const auto* group = mLevelPackManifest->levelGroups()->Get(i);
			
			const auto* levels = group->levels();

			flatbuffers::uoffset_t numLevelsInGroup = levels->size();

			for (flatbuffers::uoffset_t j = 0; j < numLevelsInGroup; ++j)
			{
				r2::asset::RawAssetFile* levelFile = r2::asset::lib::MakeRawAssetFile(levels->Get(j)->binPath()->str().c_str(), r2::asset::LEVEL);

				r2::sarr::Push(*mAssetFiles, (AssetFile*) levelFile);
			}
		}
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
		FillLevelAssetFiles();
		
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