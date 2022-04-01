#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Audio/AudioEngine.h"

#include "r2/Core/Assets/Pipeline/AssetCommands/SoundHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/SoundDefinitionUtils.h"
#include "r2/Utils/Hash.h"

namespace r2::asset::pln
{
	SoundHotReloadCommand::SoundHotReloadCommand()
		: mCallRebuiltSoundDefinitions(false)
	{
	}

	void SoundHotReloadCommand::Init(Milliseconds delay)
	{
		LoadSoundDefinitions();

		for (const auto& path : mSoundDirectories)
		{
			FileWatcher fw;
			fw.Init(delay, path);
			fw.AddCreatedListener(std::bind(&SoundHotReloadCommand::AddNewSoundDefinitionFromFile, this, std::placeholders::_1));
			mFileWatchers.push_back(fw);
		}

		//for (const auto& path : mSoundDefinitionFilePaths)
		{
			std::filesystem::path p = mSoundDefinitionFilePath;
			std::string definitionDir = p.parent_path().string();

			FileWatcher fw;
			fw.Init(delay, definitionDir);
			fw.AddModifyListener(std::bind(&SoundHotReloadCommand::SoundDefinitionsChanged, this, std::placeholders::_1));
			mFileWatchers.push_back(fw);
		}

		
	}

	void SoundHotReloadCommand::Update()
	{
		for (auto& fw : mFileWatchers)
		{
			fw.Run();
		}

		if (mCallRebuiltSoundDefinitions)
		{
			r2::audio::AudioEngine::PushNewlyBuiltSoundDefinitions({ mSoundDefinitionFilePath});
			mCallRebuiltSoundDefinitions = false;
		}
	}

	bool SoundHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	r2::asset::pln::AssetHotReloadCommand::CreateDirCmd SoundHotReloadCommand::DirectoriesToCreate() const
	{
		return {};
	}

	void SoundHotReloadCommand::AddSoundDirectories(const std::vector<std::string>& soundDirectories)
	{
		mSoundDirectories.insert(mSoundDirectories.end(), soundDirectories.begin(), soundDirectories.end());
	}

	void SoundHotReloadCommand::SetSoundDefinitionFilePath(const std::string& soundDefinitionFilePath)
	{
		mSoundDefinitionFilePath = soundDefinitionFilePath;
	}

	void SoundHotReloadCommand::SoundDefinitionsChanged(const std::string& changedPath)
	{
		if (std::filesystem::path(changedPath).extension() == ".json")
		{
			bool result = r2::asset::pln::audio::GenerateSoundDefinitionsFromJson(changedPath);
			R2_CHECK(result, "Failed to generate sound definitions from: %s", changedPath.c_str());

			mSoundDefinitions = r2::asset::pln::audio::LoadSoundDefinitions(mSoundDefinitionFilePath);

			mCallRebuiltSoundDefinitions = true;
		}
	}

	void SoundHotReloadCommand::AddNewSoundDefinitionFromFile(const std::string& addedPath)
	{
		r2::audio::AudioEngine::SoundDefinition soundDefinition;
		r2::util::PathCpy(soundDefinition.soundName, addedPath.c_str());

		std::string fileName = std::filesystem::path(addedPath).filename().string();

		soundDefinition.soundKey = STRING_ID(fileName.c_str());

		r2::asset::pln::audio::AddNewSoundDefinition(mSoundDefinitions, soundDefinition);

		bool result = r2::asset::pln::audio::GenerateSoundDefinitionsFile(mSoundDefinitionFilePath, mSoundDefinitions);

		R2_CHECK(result, "Failed to write sound definition file");

		mCallRebuiltSoundDefinitions = true;
	}

	void SoundHotReloadCommand::LoadSoundDefinitions()
	{
		//check to see if we have a .sdef file
		//if we do, then load it
		//otherwise if we have a .json file generate the .sdef file from the json and load that
		//otherwise generate the file from the directories

		//for (const auto& soundDefinitionFilPath : mSoundDefinitionFilePaths)
		{
			std::filesystem::path p = mSoundDefinitionFilePath;
			std::string definitionDir = p.parent_path().string();

			std::string soundDefinitionFile = "";
			r2::asset::pln::audio::FindSoundDefinitionFile(definitionDir, soundDefinitionFile, true);

			if (soundDefinitionFile.empty())
			{
				if (r2::asset::pln::audio::FindSoundDefinitionFile(definitionDir, soundDefinitionFile, false))
				{
					if (!r2::asset::pln::audio::GenerateSoundDefinitionsFromJson(soundDefinitionFile))
					{
						R2_CHECK(false, "Failed to generate sound definition file from json!");
						return;
					}
				}
				else
				{
					if (!r2::asset::pln::audio::GenerateSoundDefinitionsFromDirectories(mSoundDefinitionFilePath, mSoundDirectories))
					{
						R2_CHECK(false, "Failed to generate sound definition file from directories!");
						return;
					}

				}

				mCallRebuiltSoundDefinitions = true;
			}
			else
			{
				std::vector<r2::audio::AudioEngine::SoundDefinition> soundDefinitions = r2::asset::pln::audio::LoadSoundDefinitions(soundDefinitionFile);
				mSoundDefinitions.insert(mSoundDefinitions.end(), soundDefinitions.begin(), soundDefinitions.end());
			}
		}
	}

}

#endif