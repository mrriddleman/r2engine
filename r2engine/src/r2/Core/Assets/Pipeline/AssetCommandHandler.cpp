#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommandHandler.h"
#include "r2/Core/Assets/Pipeline/MakeEngineModels.h"
#include "r2/Core/Assets/Pipeline/FileSystemHelpers.h"

namespace r2::asset::pln
{
	void MakeEngineBinaryAssetFolders();
	
	AssetCommandHandler::AssetCommandHandler():
		mAssetCommands{},
		mLastTime(std::chrono::steady_clock::now()),
		mDelay(std::chrono::milliseconds(200)),
		mEnd(false)
	{
	}

	void AssetCommandHandler::Init(Milliseconds delay, std::vector<std::unique_ptr<AssetHotReloadCommand>> assetCommands)
	{
		mDelay = delay;

		mAssetCommands.insert(
			mAssetCommands.end(),
			std::make_move_iterator(assetCommands.begin()),
			std::make_move_iterator(assetCommands.end()));

		MakeEngineBinaryAssetFolders();
		MakeGameBinaryAssetFolders();

		auto makeMeshes = ShouldMakeEngineMeshes();
		if (makeMeshes.size() > 0)
		{
			MakeEngineMeshes(makeMeshes);
		}

		auto makeModels = ShouldMakeEngineModels();
		if (makeModels.size() > 0)
		{
			MakeEngineModels(makeModels);
		}

		for (std::unique_ptr<AssetHotReloadCommand>& cmd : mAssetCommands)
		{
			cmd->Init(mDelay);
		}
		

		mEnd = false;

		std::thread t(std::bind(&AssetCommandHandler::ThreadProc, this));

		mAssetWatcherThread = std::move(t);

	}

	void AssetCommandHandler::Update()
	{
		for (auto& cmd : mAssetCommands)
		{
			if (cmd->ShouldRunOnMainThread())
				cmd->Update();
		}
	}

	void AssetCommandHandler::Shutdown()
	{
		for (auto& cmd : mAssetCommands)
		{
			cmd->Shutdown();
		}

		mEnd = true;
		mAssetWatcherThread.join();
	}

	void AssetCommandHandler::ThreadProc()
	{
		while (!mEnd)
		{
			std::this_thread::sleep_for(mDelay);

			for (auto& cmd : mAssetCommands)
			{
				if(!cmd->ShouldRunOnMainThread())
					cmd->Update();
			}
		}
	}

	void MakeEngineBinaryAssetFolders()
	{
		std::filesystem::path assetBinPath = R2_ENGINE_ASSET_BIN_PATH;
		if (!std::filesystem::exists(assetBinPath))
		{
			std::filesystem::create_directory(assetBinPath);
		}

		std::filesystem::path modelsBinPath = R2_ENGINE_INTERNAL_MODELS_BIN;
		if (!std::filesystem::exists(modelsBinPath))
		{
			std::filesystem::create_directory(modelsBinPath);
		}

		std::filesystem::path texturesBinPath = R2_ENGINE_INTERNAL_TEXTURES_BIN;
		if (!std::filesystem::exists(texturesBinPath))
		{
			std::filesystem::create_directory(texturesBinPath);
		}

		std::filesystem::path texturesManifestsBinPath = R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS_BIN;
		if (!std::filesystem::exists(texturesManifestsBinPath))
		{
			std::filesystem::create_directory(texturesManifestsBinPath);
		}

		std::filesystem::path materialsBinPath = R2_ENGINE_INTERNAL_MATERIALS_DIR_BIN;
		if (!std::filesystem::exists(materialsBinPath))
		{
			std::filesystem::create_directory(materialsBinPath);
		}

		std::filesystem::path materialsManifestsBinPath = R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN;
		if (!std::filesystem::exists(materialsManifestsBinPath))
		{
			std::filesystem::create_directory(materialsManifestsBinPath);
		}

		std::filesystem::path materialsPacksBinPath = R2_ENGINE_INTERNAL_MATERIALS_PACKS_DIR_BIN;
		if (!std::filesystem::exists(materialsPacksBinPath))
		{
			std::filesystem::create_directory(materialsPacksBinPath);
		}

	}

	void AssetCommandHandler::MakeGameBinaryAssetFolders() const
	{
		for (auto& cmd : mAssetCommands)
		{
			const AssetHotReloadCommand::CreateDirCmd createDirCmd = cmd->DirectoriesToCreate();

			if (!createDirCmd.pathsToCreate.empty())
			{
				MakeDirectoriesRecursively(createDirCmd.pathsToCreate, createDirCmd.startAtOne, createDirCmd.startAtParent);
			}
		}
	}

}

#endif