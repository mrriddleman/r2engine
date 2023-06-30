#include "r2pch.h"

#ifdef  R2_EDITOR
#include "r2/Core/File/PathUtils.h"
#include "r2/Editor/EditorLevel.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Platform/Platform.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelManager.h"
#include <filesystem>
#include "r2/Core/File/PathUtils.h"

namespace r2 
{
	const std::string DEFAULT_LEVEL_NAME = "NewLevel";
	const std::string DEFAULT_GROUP_NAME = "NewGroup";

	EditorLevel::EditorLevel()
		:mVersion(1)
		,mLevelName(DEFAULT_LEVEL_NAME)
		,mGroupName(DEFAULT_GROUP_NAME)
		,mnoptrLevel(nullptr)
	{
	}

	EditorLevel::~EditorLevel()
	{
		ClearAssetData();

		if (mnoptrLevel)
		{
			mnoptrLevel = nullptr;
		}
	}

	void EditorLevel::Save()
	{
		//CENG.GetLevelManager().SaveNewLevelFile(*this);
	}

	void EditorLevel::Reload()
	{
		Load(mnoptrLevel);
	}

	bool EditorLevel::Load(const Level* level)
	{
		ClearAssetData();

		R2_CHECK(level != nullptr, "Should never happen");

		mnoptrLevel = level;

		const flat::LevelData* levelData = level->GetLevelData();
		
		if (levelData)
		{
			R2_CHECK(levelData != nullptr, "levelData is nullptr?");

			mLevelName = std::filesystem::path(levelData->levelNameString()->str()).stem().string();
			mGroupName = levelData->groupNameString()->str();
			mVersion = levelData->version();

			char filePath[r2::fs::FILE_PATH_LENGTH];

			u32 numModelFiles = levelData->modelFilePaths()->size();
			for (u32 i = 0; i < numModelFiles; ++i)
			{
				r2::fs::utils::BuildPathFromCategory(fs::utils::MODELS, levelData->modelFilePaths()->Get(i)->binPath()->str().c_str(), filePath);
				mModelFiles.insert(filePath);
			}

			u32 numAnimationFiles = levelData->animationFilePaths()->size();
			for (u32 i = 0; i < numAnimationFiles; ++i)
			{
				r2::fs::utils::BuildPathFromCategory(fs::utils::ANIMATIONS, levelData->animationFilePaths()->Get(i)->binPath()->str().c_str(), filePath);
				mAnimationFiles.insert(filePath);
			}

			u32 numTexturePackPaths = levelData->materialNames()->size();
			for (u32 i = 0; i < numTexturePackPaths; ++i)
			{
				r2::mat::MaterialName newMaterialName;
				newMaterialName.packName = levelData->materialNames()->Get(i)->materialPackName();
				newMaterialName.name = levelData->materialNames()->Get(i)->name();

				mMaterialNames.insert(newMaterialName);
			}

			u32 numSoundPaths = levelData->soundPaths()->size();
			for (u32 i = 0; i < numSoundPaths; ++i)
			{
				//@TODO(Serge): figure this out
				r2::fs::utils::BuildPathFromCategory(fs::utils::SOUND_FX, levelData->soundPaths()->Get(i)->binPath()->str().c_str(), filePath);
				mSoundPaths.insert(filePath);
			}
		}

		return true;
	}

	void EditorLevel::UnloadLevel()
	{
		Clear();
	}

	void EditorLevel::SetVersion(u32 version)
	{
		mVersion = version;
	}

	u32 EditorLevel::GetVersion() const
	{
		return mVersion;
	}

	void EditorLevel::SetLevelName(const std::string& levelName)
	{
		mLevelName = levelName;
		ResetLevelName();
	}

	void EditorLevel::SetGroupName(const std::string& groupName)
	{
		mGroupName = groupName;
		ResetLevelName();
	}

	const std::string& EditorLevel::GetLevelName() const
	{
		return mLevelName;
	}

	const std::string& EditorLevel::GetGroupName() const
	{
		return mGroupName;
	}

	std::vector<std::string> EditorLevel::GetModelAssetPaths() const
	{
		std::vector<std::string> vecAssetFiles(mModelFiles.begin(), mModelFiles.end());
		return vecAssetFiles;
	}

	std::vector<std::string> EditorLevel::GetAnimationAssetPaths() const
	{
		std::vector<std::string> vecAssetFiles(mAnimationFiles.begin(), mAnimationFiles.end());
		return vecAssetFiles;
	}

	std::vector<r2::mat::MaterialName> EditorLevel::GetMaterialNames() const
	{
		std::vector<r2::mat::MaterialName> vecAssetFiles(mMaterialNames.begin(), mMaterialNames.end());
		return vecAssetFiles;
	}

	std::vector<std::string> EditorLevel::GetSoundPaths() const
	{
		std::vector<std::string> vecAssetFiles(mSoundPaths.begin(), mSoundPaths.end());
		return vecAssetFiles;
	}

	void EditorLevel::AddModelFile(const std::string& modelFile)
	{
		mModelFiles.insert(modelFile);
	}

	void EditorLevel::AddAnimationFile(const std::string& animationFile)
	{
		mAnimationFiles.insert(animationFile);
	}

	void EditorLevel::AddMaterialName(const r2::mat::MaterialName& materialName)
	{
		mMaterialNames.insert(materialName);
	}

	void EditorLevel::AddSoundPath(const std::string& soundPath)
	{
		mSoundPaths.insert(soundPath);
	}

	void EditorLevel::RemoveModelFile(const std::string& modelFile)
	{
		mModelFiles.erase(modelFile);
	}

	void EditorLevel::RemoveAnimationFile(const std::string& animationFile)
	{
		mAnimationFiles.erase(animationFile);
	}

	void EditorLevel::RemoveMaterialName(const r2::mat::MaterialName& materialName)
	{
		mMaterialNames.erase(materialName);
	}

	void EditorLevel::RemoveSoundPath(const std::string& soundPath)
	{
		mSoundPaths.erase(soundPath);
	}

	void EditorLevel::Clear()
	{
		ClearAssetData();

		mnoptrLevel = nullptr;
		if (mnoptrLevel)
		{
			CENG.GetLevelManager().UnloadLevel(mnoptrLevel);
			mnoptrLevel = nullptr;
		}

		auto levelURIPath = std::filesystem::path(mGroupName) / std::filesystem::path(mLevelName);
		const Level* level = CENG.GetLevelManager().MakeNewLevel(r2::asset::GetAssetNameForFilePath(levelURIPath.string().c_str(), r2::asset::LEVEL));
		mnoptrLevel = level;
	}

	void EditorLevel::ClearAssetData()
	{
		mVersion = 1;
		mLevelName = DEFAULT_LEVEL_NAME;
		mGroupName = DEFAULT_GROUP_NAME;
		mModelFiles.clear();
		mAnimationFiles.clear();
		mMaterialNames.clear();
		mSoundPaths.clear();
	}

	const Level* EditorLevel::GetLevelPtr() const
	{
		return mnoptrLevel;
	}

	void EditorLevel::AddEntity(ecs::Entity e) const
	{
		if (mnoptrLevel)
		{
			mnoptrLevel->AddEntity(e);
		}
	}

	void EditorLevel::RemoveEntity(ecs::Entity e) const
	{
		if (mnoptrLevel)
		{
			mnoptrLevel->RemoveEntity(e);
		}
	}


	void EditorLevel::ResetLevelName()
	{
		if (mnoptrLevel)
		{
			auto levelURIPath = std::filesystem::path(mGroupName) / std::filesystem::path(mLevelName);
			char sanitizedLevelURI[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(levelURIPath.string().c_str(), sanitizedLevelURI);

			mnoptrLevel->ResetLevelName(r2::asset::GetAssetNameForFilePath(sanitizedLevelURI, r2::asset::LEVEL));
		}
	}
}

#endif //  R2_EDITOR

