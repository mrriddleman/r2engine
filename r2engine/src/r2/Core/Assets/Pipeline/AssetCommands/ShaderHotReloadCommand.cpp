#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/ShaderHotReloadCommand.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset::pln
{

	void ShaderHotReloadCommand::Init(Milliseconds delay)
	{
		BuildInternalShaderPassesIfNeeded();
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

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> ShaderHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		for (size_t i = 0; i < mInternalShaderPassesBuildData.size(); i++)
		{
			createDirCmd.pathsToCreate.push_back(mInternalShaderPassesBuildData[i].internalBinShaderManifestPath);
		}

		createDirCmd.startAtParent = true;
		createDirCmd.startAtOne = false;

		return {createDirCmd};
	}

	void ShaderHotReloadCommand::AddShaderWatchPaths(const std::vector<std::string>& watchPaths)
	{
		mWatchPaths.insert(mWatchPaths.end(), watchPaths.begin(), watchPaths.end());
	}

	void ShaderHotReloadCommand::AddManifestFilePaths(const std::vector<std::string>& manifestFilePaths)
	{
		mManifestFilePaths.insert(mManifestFilePaths.end(), manifestFilePaths.begin(), manifestFilePaths.end());
	}

	void ShaderHotReloadCommand::AddInternalShaderPassesBuildDescription(const std::string& rawManifestPath, const std::string& binManifestPath, InternalShaderPassesBuildFunc func)
	{
		mInternalShaderPassesBuildData.push_back({ rawManifestPath, binManifestPath, func });
	}

	void ShaderHotReloadCommand::ReloadShaderManifests()
	{
		mShaderManifests.clear();

		size_t numManifestFilePaths = mManifestFilePaths.size();

		size_t numInternalManifestPaths = mInternalShaderPassesBuildData.size();

		R2_CHECK(numInternalManifestPaths == numManifestFilePaths, "?");

		for (size_t i = 0; i < numManifestFilePaths; ++i)
		{
			const auto& manifestFilePath = mManifestFilePaths.at(i);

			std::vector<ShaderManifest> shaderManifests = LoadAllShaderManifests(manifestFilePath);

			if (shaderManifests.size() == 0)
			{
				//@NOTE(Serge): this only works because of how we added the paths in Engine.cpp, r2 is first, then the app
				const auto& internalManifestFilePath = mInternalShaderPassesBuildData.at(i).internalBinShaderManifestPath;
				
				shaderManifests = LoadAllShaderManifests(internalManifestFilePath);

				R2_CHECK(shaderManifests.size() > 0, "We don't have any internal shader passes?");

				bool success = GenerateNonInternalShaderManifestsFromDirectories(shaderManifests, manifestFilePath, mWatchPaths.at(i));

				R2_CHECK(success, "We couldn't create the manifest!");

				//if (!success)
				//{
				//	R2_LOGE("Failed to build shader manifests");
				//}
				//else
				//{
				//	//r2::draw::shadersystem::ReloadManifestFile(manifestFilePath);
				//}
			}

			//BuildShaderManifestsIfNeeded(shaderManifests, manifestFilePath, mWatchPaths.at(i));

			

			mShaderManifests.push_back(shaderManifests);
		}
	}

	void ShaderHotReloadCommand::ShaderChangedRequest(const std::string& changedPath)
	{
		//look through the manifests and find which need to be re-compiled and linked
		size_t numShaderManifests = mShaderManifests.size();

		char changedPathSanitizedCString[fs::FILE_PATH_LENGTH];
		fs::utils::SanitizeSubPath(changedPath.c_str(), changedPathSanitizedCString);

		std::string changedPathSanitized = std::string(changedPathSanitizedCString);

		for (size_t i = 0; i < numShaderManifests; ++i)
		{
			auto& shaderManifests = mShaderManifests.at(i);
			for (const auto& shaderManifest : shaderManifests)
			{
				if (changedPathSanitized == shaderManifest.vertexShaderPath ||
					changedPathSanitized == shaderManifest.fragmentShaderPath ||
					changedPathSanitized == shaderManifest.geometryShaderPath ||
					changedPathSanitized == shaderManifest.computeShaderPath)
				{
					/*bool success = BuildShaderManifestsIfNeeded(shaderManifests, mManifestFilePaths[i], mWatchPaths[i]);

					if (!success)
					{
						R2_LOGE("Failed to build shader manifests");
					}*/

					r2::draw::shadersystem::ReloadShader(shaderManifest, false);
					break;
				}
				else if (changedPathSanitized == shaderManifest.partPath)
				{
					r2::draw::shadersystem::ReloadShader(shaderManifest, true);
					break;
				}
			}
		}
	}

	void ShaderHotReloadCommand::BuildInternalShaderPassesIfNeeded()
	{
		for (auto& buildDesc : mInternalShaderPassesBuildData)
		{
			if (std::filesystem::exists(buildDesc.internalBinShaderManifestPath))
			{
				continue;
			}
			bool result = buildDesc.func(buildDesc.internalRawShaderManifestPath, std::filesystem::path(buildDesc.internalBinShaderManifestPath).parent_path().string());
			R2_CHECK(result == true, "Should have built the manifest for: %s\n", buildDesc.internalBinShaderManifestPath.c_str());
		}
	}

}

#endif