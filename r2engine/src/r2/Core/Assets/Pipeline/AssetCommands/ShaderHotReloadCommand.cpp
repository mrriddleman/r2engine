#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/ShaderHotReloadCommand.h"
#include "r2/Render/Renderer/ShaderSystem.h"

namespace r2::asset::pln
{

	void ShaderHotReloadCommand::Init(Milliseconds delay)
	{
		ReloadShaderManifests();

		for (const auto& path : mWatchPaths)
		{
			FileWatcher fw;

			fw.Init(delay, path);
			fw.AddModifyListener(std::bind(&ShaderHotReloadCommand::ShaderChangedRequest, this, std::placeholders::_1));
			fw.AddCreatedListener(std::bind(&ShaderHotReloadCommand::ShaderChangedRequest, this, std::placeholders::_1));

			mFileWatchers.push_back(fw);
		}
	}

	void ShaderHotReloadCommand::Update()
	{
		for (auto& fw : mFileWatchers)
		{
			fw.Run();
		}
	}

	r2::asset::pln::AssetHotReloadCommand::CreateDirCmd ShaderHotReloadCommand::DirectoriesToCreate() const
	{
		return {};
	}

	void ShaderHotReloadCommand::AddShaderWatchPaths(const std::vector<std::string>& watchPaths)
	{
		mWatchPaths.insert(mWatchPaths.end(), watchPaths.begin(), watchPaths.end());
	}

	void ShaderHotReloadCommand::AddManifestFilePaths(const std::vector<std::string>& manifestFilePaths)
	{
		mManifestFilePaths.insert(mManifestFilePaths.end(), manifestFilePaths.begin(), manifestFilePaths.end());
	}

	void ShaderHotReloadCommand::ReloadShaderManifests()
	{
		mShaderManifests.clear();

		size_t numManifestFilePaths = mManifestFilePaths.size();
		for (size_t i = 0; i < numManifestFilePaths; ++i)
		{
			const auto& manifestFilePath = mManifestFilePaths.at(i);
			std::vector<ShaderManifest> shaderManifests = LoadAllShaderManifests(manifestFilePath);
			bool success = BuildShaderManifestsIfNeeded(shaderManifests, manifestFilePath, mWatchPaths.at(i));

			if (!success)
			{
				R2_LOGE("Failed to build shader manifests");
			}
			else
			{
				r2::draw::shadersystem::ReloadManifestFile(manifestFilePath);
			}

			mShaderManifests.push_back(shaderManifests);
		}
	}

	void ShaderHotReloadCommand::ShaderChangedRequest(const std::string& changedPath)
	{
		//look through the manifests and find which need to be re-compiled and linked
		size_t numShaderManifests = mShaderManifests.size();

		for (size_t i = 0; i < numShaderManifests; ++i)
		{
			auto& shaderManifests = mShaderManifests.at(i);
			for (const auto& shaderManifest : shaderManifests)
			{
				if (changedPath == shaderManifest.vertexShaderPath ||
					changedPath == shaderManifest.fragmentShaderPath ||
					changedPath == shaderManifest.geometryShaderPath ||
					changedPath == shaderManifest.computeShaderPath)
				{
					bool success = BuildShaderManifestsIfNeeded(shaderManifests, mManifestFilePaths[i], mWatchPaths[i]);

					if (!success)
					{
						R2_LOGE("Failed to build shader manifests");
					}

					r2::draw::shadersystem::ReloadShader(shaderManifest);
					break;
				}
			}
		}
	}

}

#endif