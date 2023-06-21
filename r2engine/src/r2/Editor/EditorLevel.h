#ifndef __EDITOR_LEVEL_H__
#define __EDITOR_LEVEL_H__

#ifdef R2_EDITOR
#include <vector>
#include <set>
#include <string>

namespace r2::asset
{
	class AssetFile;
}

namespace flat
{
	struct LevelData;
}

namespace r2
{
	class EditorLevel
	{
	public:
		EditorLevel();
		~EditorLevel();

		bool Load(const flat::LevelData* levelData);

		void SetLevelName(const std::string& levelName);
		void SetGroupName(const std::string& groupName);

		const std::string& GetLevelName() const;
		const std::string& GetGroupName() const;

		std::vector<std::string> GetModelAssetPaths() const;
		std::vector<std::string> GetAnimationAssetPaths() const;
		std::vector<std::string> GetTexturePackPaths() const;
		std::vector<std::string> GetSoundPaths() const;

		void AddModelFile(const std::string& modelFile);
		void AddAnimationFile(const std::string& animationFile);
		void AddTexturePackPath(const std::string& texturePackPath);
		void AddSoundPath(const std::string& soundPath);

		void RemoveModelFile(const std::string& modelFile);
		void RemoveAnimationFile(const std::string& animationFile);
		void RemoveTexturePackPath(const std::string& texturePackPath);
		void RemoveSoundPath(const std::string& soundPath);

	private:
		
		std::string mLevelName;
		std::string mGroupName;

		std::set<std::string> mModelFiles;
		std::set<std::string> mAnimationFiles;
		std::set<std::string> mTexturePackPaths;
		std::set<std::string> mSoundPaths;
	};
}

#endif

#endif // 
