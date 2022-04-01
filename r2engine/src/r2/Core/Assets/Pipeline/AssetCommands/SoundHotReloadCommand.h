#ifndef __SOUND_HOT_RELOAD_COMMAND_H__
#define __SOUND_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"

namespace r2::asset::pln
{
	class SoundHotReloadCommand : public AssetHotReloadCommand
	{
	public:
		SoundHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override;
		virtual CreateDirCmd DirectoriesToCreate() const override;

		void AddSoundDirectories(const std::vector<std::string>& soundDirectories);

		void SetSoundDefinitionFilePath(const std::string& soundDefinitionFilePath);

	private:
		void LoadSoundDefinitions();
		void SoundDefinitionsChanged(const std::string& changedPath);
		void AddNewSoundDefinitionFromFile(const std::string& addedPath);

		std::vector<std::string> mSoundDirectories;
		std::string mSoundDefinitionFilePath;

		std::vector<r2::audio::AudioEngine::SoundDefinition> mSoundDefinitions;

		bool mCallRebuiltSoundDefinitions;

	};
}

#endif

#endif // __SOUND_HOT_RELOAD_COMMAND_H__
