#ifndef __LEVEL_PACK_HOT_RELOAD_COMMAND_H__
#define __LEVEL_PACK_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"

namespace r2::asset::pln
{
	class LevelPackHotReloadCommand : public AssetHotReloadCommand
	{
	public:
		LevelPackHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override;
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

	//	void AddSoundDirectories(const std::vector<std::string>& soundDirectories);

		void SetLevelPackBinFilePath(const std::string& levelPackFilePath);
		void SetLevelPackRawFilePath(const std::string& levelPackRawFilePath);

		//static bool SoundManifestHotReloaded(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, HotReloadType type);
		virtual AssetHotReloadCommandType GetAssetHotReloadCommandType() const override { return AHRCT_LEVEL_ASSET; }
	private:
		void GenerateLevelPackIfNecessary();
		//void LoadSoundDefinitions();
		//void SoundDefinitionsChanged(const std::string& changedPath);
		//void AddNewSoundDefinitionFromFile(const std::string& addedPath);

		//std::vector<std::string> mSoundDirectories;
		std::string mLevelPackBinFilePath;
		std::string mLevelPackRawFilePath;

		//	std::vector<r2::audio::AudioEngine::SoundDefinition> mSoundDefinitions;

		//bool mCallRebuiltSoundDefinitions;

	};
}

#endif

#endif