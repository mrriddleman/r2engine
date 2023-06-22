#include "r2pch.h"

#ifdef  R2_EDITOR
#include "r2/Editor/EditorLevel.h"
#include "r2/Game/Level/LevelData_generated.h"

namespace r2 
{
	EditorLevel::EditorLevel()
		:mVersion(1)
		,mLevelName("")
		,mGroupName("")
	{

	}

	EditorLevel::~EditorLevel()
	{
		Clear();
	}

	bool EditorLevel::Load(const flat::LevelData* levelData)
	{
		Clear();

		mLevelName = levelData->name()->str();
		mGroupName = levelData->groupName()->str();
		mVersion = levelData->version();

		u32 numModelFiles = levelData->modelFilePaths()->size();
		for (u32 i = 0; i < numModelFiles; ++i)
		{
			mModelFiles.insert(levelData->modelFilePaths()->Get(i)->binPath()->str());
		}

		u32 numAnimationFiles = levelData->animationFilePaths()->size();
		for (u32 i = 0; i < numAnimationFiles; ++i)
		{
			mAnimationFiles.insert(levelData->animationFilePaths()->Get(i)->binPath()->str());
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
			mSoundPaths.insert(levelData->soundPaths()->Get(i)->binPath()->str());
		}

		return true;
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
	}

	void EditorLevel::SetGroupName(const std::string& groupName)
	{
		mGroupName = groupName;
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
		mModelFiles.clear();
		mAnimationFiles.clear();
		mMaterialNames.clear();
		mSoundPaths.clear();
	}
}

#endif //  R2_EDITOR

