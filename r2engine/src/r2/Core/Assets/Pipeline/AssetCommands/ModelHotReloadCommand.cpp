#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/ModelHotReloadCommand.h"

namespace r2::asset::pln
{

	ModelHotReloadCommand::ModelHotReloadCommand()
	{

	}

	void ModelHotReloadCommand::Init(Milliseconds delay)
	{

	}

	void ModelHotReloadCommand::Update()
	{

	}

	bool ModelHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> ModelHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		createDirCmd.pathsToCreate = mBinaryModelDirectories;
		createDirCmd.startAtParent = false;
		createDirCmd.startAtOne = false;

		return { createDirCmd };
	}

	void ModelHotReloadCommand::AddBinaryModelDirectories(const std::vector<std::string>& binaryModelDirectories)
	{
		mBinaryModelDirectories.insert(mBinaryModelDirectories.end(), binaryModelDirectories.begin(), binaryModelDirectories.end());
	}

}

#endif