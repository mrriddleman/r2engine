#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/AnimationHotReloadCommand.h"

namespace r2::asset::pln
{

	AnimationHotReloadCommand::AnimationHotReloadCommand()
	{

	}

	void AnimationHotReloadCommand::Init(Milliseconds delay)
	{

	}

	void AnimationHotReloadCommand::Update()
	{

	}

	bool AnimationHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> AnimationHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		createDirCmd.pathsToCreate = mBinaryAnimationDirectories;
		createDirCmd.startAtParent = false;
		createDirCmd.startAtOne = false;

		return { createDirCmd };
	}

	void AnimationHotReloadCommand::AddBinaryAnimationDirectories(const std::vector<std::string>& binaryAnimationDirectories)
	{
		mBinaryAnimationDirectories.insert(mBinaryAnimationDirectories.end(), binaryAnimationDirectories.begin(), binaryAnimationDirectories.end());
	}

}

#endif