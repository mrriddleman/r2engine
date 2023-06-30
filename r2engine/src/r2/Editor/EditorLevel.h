#ifndef __EDITOR_LEVEL_H__
#define __EDITOR_LEVEL_H__

#ifdef R2_EDITOR
#include <vector>
#include <set>
#include <string>
#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Game/ECS/Entity.h"

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
	class Level;

	class EditorLevel
	{
	public:
		EditorLevel();
		~EditorLevel();

		
		void Save();
		bool Load(const Level* level);
		void UnloadLevel();
		void Reload();


		void SetVersion(u32 version);
		u32 GetVersion() const;

		void SetLevelName(const std::string& levelName);
		void SetGroupName(const std::string& groupName);

		const std::string& GetLevelName() const;
		const std::string& GetGroupName() const;

		std::vector<std::string> GetModelAssetPaths() const;
		std::vector<std::string> GetAnimationAssetPaths() const;
		std::vector<r2::mat::MaterialName> GetMaterialNames() const;
		std::vector<std::string> GetSoundPaths() const;

		void AddModelFile(const std::string& modelFile);
		void AddAnimationFile(const std::string& animationFile);
		void AddMaterialName(const r2::mat::MaterialName& materialName);
		void AddSoundPath(const std::string& soundPath);

		void RemoveModelFile(const std::string& modelFile);
		void RemoveAnimationFile(const std::string& animationFile);
		void RemoveMaterialName(const r2::mat::MaterialName& materialName);
		void RemoveSoundPath(const std::string& soundPath);

		void Clear();

		const Level* GetLevelPtr() const;
		void AddEntity(ecs::Entity e) const;
		void RemoveEntity(ecs::Entity e) const;
	private:
		

		void ClearAssetData();
		void ResetLevelName();
		u32 mVersion;

		std::string mLevelName;
		std::string mGroupName;

		std::set<std::string> mModelFiles;
		std::set<std::string> mAnimationFiles;
		std::set<r2::mat::MaterialName> mMaterialNames;
		std::set<std::string> mSoundPaths;

		const Level* mnoptrLevel;
	};
}

#endif

#endif // 
