#include <iostream>

#include "assetlib/AssetFile.h"
#include "assetlib/TextureAsset.h"
#include "assetlib/TextureMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <string>

#include "nvtt/nvtt.h"

namespace fs = std::filesystem;


namespace
{
	std::string PNG_EXTENSION = ".png";
	std::string JPG_EXTENSION = ".jpg";
	std::string TIF_EXTENSION = ".tif";
	std::string TGA_EXTENSION = ".tga";
	std::string DDS_EXTENSION = ".dds";
	std::string HDR_EXTENSION = ".hdr";
}

struct Arguments
{
	std::string inputDir;
	std::string outputDir;
};


bool IsImage(const std::string& extension)
{
	std::string theExtension = extension;

	std::transform(theExtension.begin(), theExtension.end(), theExtension.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return 
		theExtension == PNG_EXTENSION || 
		theExtension == JPG_EXTENSION || 
		theExtension == TIF_EXTENSION || 
		theExtension == TGA_EXTENSION || 
		theExtension == DDS_EXTENSION ||
		theExtension == HDR_EXTENSION;
}

bool IsModel(const std::string& extension)
{
	return false;
}

bool IsAnimation(const std::string& extension)
{
	return false;
}

fs::path GetOutputPathForInputDirectory(const fs::path& outputPath, const fs::path& inputPath, const fs::path& path)
{
	std::vector<fs::path> pathsSeen;

	fs::path thePath = path;

	if (fs::is_regular_file(path))
	{
		thePath = path.parent_path();
	}

	bool found = false;
	while (!found && !thePath.empty() && thePath != thePath.root_path())
	{
		if (inputPath == thePath)
		{
			found = true;
			break;
		}

		pathsSeen.push_back(thePath.stem());
		thePath = thePath.parent_path();
	}

	if (!found)
	{
		assert(false && "Didn't find the path!");
		return "";
	}

	fs::path output = outputPath;

	auto rIter = pathsSeen.rbegin();

	for (; rIter != pathsSeen.rend(); ++rIter)
	{
		output = output / *rIter;
	}

	return output;
}

bool ConvertImage(const fs::path& parentOutputDir, const fs::path& inputFilePath, const std::string& extension);

int main(int agrc, char** argv)
{
	Arguments arguments;
	r2::cmdln::CommandLine args("Asset Converter will convert raw assets into assets r2 can use");
	args.AddArgument({ "-i", "--input" }, &arguments.inputDir, "Input Directory");
	args.AddArgument({ "-o", "--output" }, &arguments.outputDir, "Output Directory");
	args.Parse(agrc, argv);

	if (arguments.inputDir.empty())
	{
		printf("Failed to set an input directory\n");
		return 0;
	}

	if (arguments.outputDir.empty())
	{
		printf("Failed to set an output directory\n");
		return 0;
	}

	fs::path inputPath{ arguments.inputDir };
	fs::path outputPath{ arguments.outputDir };

	for (auto& p : fs::recursive_directory_iterator(inputPath))
	{
		fs::path newOutputPath = GetOutputPathForInputDirectory(outputPath, inputPath, p.path());

		std::string extension = p.path().extension().string();

		if (p.is_directory())
		{
			if (!fs::exists(newOutputPath) && p.path().stem().string() != "hdr")
			{
				printf("Making new directory: %s\n", newOutputPath.string().c_str());
			}

			continue;
		}
		else if (p.is_regular_file() && p.file_size() > 0)
		{
			if (IsImage(extension) && p.path().parent_path().stem().string() != "hdr")
			{
				ConvertImage(p.path(), newOutputPath, extension);
			}
			else if (IsAnimation(extension))
			{

			}
			else if (IsModel(extension))
			{

			}
		}
	}

	return 0;
}

bool ConvertImage(const fs::path& inputFilePath, const fs::path& parentOutputDir,  const std::string& extension)
{
	printf("Converting file: %s, and putting it in directory: %s\n", inputFilePath.string().c_str(), parentOutputDir.string().c_str());
	return false;
}