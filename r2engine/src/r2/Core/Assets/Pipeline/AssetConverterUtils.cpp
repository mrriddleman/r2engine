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
}

#endif