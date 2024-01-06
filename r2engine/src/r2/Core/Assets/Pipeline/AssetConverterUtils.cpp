#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "AssetConverterUtils.h"
#include <cstdlib>
#include <cstdio>
#include <filesystem>

namespace r2::asset::pln::assetconvert
{
	int RunSystemCommand(const char* command)
	{
		int result = system(command);
		if (result != 0)
		{
			R2_LOGE("Failed to run command: %s\n\n with result: %i\n", command, result);
		}
		return result;
	}

	int RunConverter(const std::string& inputDir, const std::string& outputDir)
	{
		char command[Kilobytes(2)];
		std::string converterEXEPathStr = R2_ENGINE_ASSET_CONVERTER_EXE;

		std::filesystem::path converterEXEPath = std::filesystem::path(converterEXEPathStr);
		converterEXEPath.make_preferred();

		std::filesystem::path inputDirPath = inputDir;
		inputDirPath.make_preferred();

		std::filesystem::path outputDirPath = outputDir;
		outputDirPath.make_preferred();

		sprintf_s(command, Kilobytes(2), "%s -i %s -o %s", converterEXEPath.string().c_str(), inputDirPath.string().c_str(), outputDirPath.string().c_str());

		return RunSystemCommand(command);
	}

	int RunModelConverter(
		const std::string& inputDir,
		const std::string& outputDir,
		const std::string& materialManifestPath,
		const std::string& materialRawDirectory,
		const std::string& engineTexturePacksManifestPath,
		const std::string& appTexturePacksManifestPath,
		bool forceMaterialRecreation)
	{
		char command[Kilobytes(2)];
		std::string converterEXEPathStr = R2_ENGINE_ASSET_CONVERTER_EXE;

		std::filesystem::path converterEXEPath = std::filesystem::path(converterEXEPathStr);
		converterEXEPath.make_preferred();

		std::filesystem::path inputDirPath = inputDir;
		inputDirPath.make_preferred();

		std::filesystem::path outputDirPath = outputDir;
		outputDirPath.make_preferred();

		std::filesystem::path materialPath = materialManifestPath;
		materialPath.make_preferred();

		std::filesystem::path materialRawDirectoryArg = materialRawDirectory;
		materialRawDirectoryArg.make_preferred();

		std::filesystem::path engineTexturePacksManifestArg = engineTexturePacksManifestPath;
		engineTexturePacksManifestArg.make_preferred();

		std::filesystem::path appTexturePacksManifestArg = appTexturePacksManifestPath;
		appTexturePacksManifestArg.make_preferred();

		std::string forceMaterialRecreationString = forceMaterialRecreation ? "true" : "false";

		sprintf_s(command, Kilobytes(2), "%s -i %s -o %s -m %s -r %s -e %s -t %s -f %s",
			converterEXEPath.string().c_str(),
			inputDirPath.string().c_str(),
			outputDirPath.string().c_str(),
			materialPath.string().c_str(),
			materialRawDirectoryArg.string().c_str(),
			engineTexturePacksManifestArg.string().c_str(),
			appTexturePacksManifestArg.string().c_str(),
			forceMaterialRecreationString.c_str());

		return RunSystemCommand(command);
	}

	/*int RunAnimationConverter(const std::string& inputDir, const std::string& outputDir)
	{
		char command[Kilobytes(2)];
		std::string converterEXEPathStr = R2_ENGINE_ASSET_CONVERTER_EXE;

		std::filesystem::path converterEXEPath = std::filesystem::path(converterEXEPathStr);
		converterEXEPath.make_preferred();

		std::filesystem::path inputDirPath = inputDir;
		inputDirPath.make_preferred();

		std::filesystem::path outputDirPath = outputDir;
		outputDirPath.make_preferred();

		sprintf_s(command, Kilobytes(2), "%s -i %s -o %s -a true", converterEXEPath.string().c_str(), inputDirPath.string().c_str(), outputDirPath.string().c_str());

		return RunSystemCommand(command);
	}*/
}

#endif