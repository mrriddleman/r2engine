#include <iostream>


#include "assetlib/TextureAsset.h"
#include "assetlib/ImageConvert.h"
#include "TexturePackMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"



#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <string>

#include <fstream>

#include "nvtt/nvtt.h"
#include <algorithm>

namespace fs = std::filesystem;


namespace
{
	std::string PNG_EXTENSION = ".png";
	std::string JPG_EXTENSION = ".jpg";
	std::string TIF_EXTENSION = ".tif";
	std::string TGA_EXTENSION = ".tga";
	std::string DDS_EXTENSION = ".dds";
	std::string HDR_EXTENSION = ".hdr";
	std::string RTEX_EXTENSION = ".rtex";
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

bool SkipDirectory(const fs::path& p)
{
	return p.stem().string() == "hdr";
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

int RunSystemCommand(const char* command)
{
	int result = system(command);
	if (result != 0)
	{
		printf("Failed to run command: %s\n\n with result: %i\n", command, result);
	}
	return result;
}

bool GenerateFlatbufferBinaryFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
{
	char command[2048];
	std::string flatc = R2_FLATC;

	fs::path flatcPath = flatc;
	flatcPath.make_preferred();

	fs::path sanitizedSourcePath = sourcePath;
	sanitizedSourcePath.make_preferred();

	fs::path sanitizedOutputPath = outputDir;
	sanitizedOutputPath.make_preferred();

	sprintf_s(command, 2048, "%s -b -o %s %s %s", flatcPath.string().c_str(), sanitizedOutputPath.string().c_str(), fbsPath.c_str(), sanitizedSourcePath.string().c_str());

	return RunSystemCommand(command) == 0;
}

const std::string TEXTURE_PACK_META_DATA_NAME_FBS = "TexturePackMetaData.fbs";

bool GenerateTexturePackMetaDataFromJSON(const std::string& jsonFile, const std::string& outputDir)
{
	fs::path flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

	flatbufferSchemaPath.make_preferred();

	fs::path texturePackMetaDataSchemaPath = flatbufferSchemaPath / TEXTURE_PACK_META_DATA_NAME_FBS;

	return GenerateFlatbufferBinaryFile(outputDir, texturePackMetaDataSchemaPath.string(), jsonFile);
}

void ReadMipMapData(const std::string& path, uint32_t& desiredMipLevels, flat::MipMapFilter& filter)
{
	std::ifstream ddsFile;

	ddsFile.open(path, std::ios::in | std::ios::binary);

	ddsFile.seekg(0, std::ios::end);

	std::streampos fSize = ddsFile.tellg();

	ddsFile.seekg(0, std::ios::beg);

	void* data = malloc(fSize);

	ddsFile.read((char*)data, fSize);

	ddsFile.close();

	const auto texturePackMetaData = flat::GetTexturePackMetaData(data);

	desiredMipLevels = texturePackMetaData->desiredMipLevels();
	filter = texturePackMetaData->mipMapFilter();

	free(data);
}


void CreateMetaDataFile(fs::path metaDataJSONPath, fs::path inputPath, fs::path outputPath, fs::path& currentMetaPath, uint32_t& desiredMipLevels, flat::MipMapFilter& mipMapFilter)
{
	fs::path newOutputPath = GetOutputPathForInputDirectory(outputPath, inputPath, metaDataJSONPath);

	fs::path binMetaFilePath = newOutputPath / "meta.tmet";

	if (!fs::exists(binMetaFilePath))
	{
		bool result = GenerateTexturePackMetaDataFromJSON(metaDataJSONPath.string(), newOutputPath.string());

		if (!result)
		{
			printf("Couldn't make the tmet file: %s!\n", binMetaFilePath.string().c_str());
		}
		else
		{
			currentMetaPath = binMetaFilePath;

			ReadMipMapData(currentMetaPath.string(), desiredMipLevels, mipMapFilter);
		}
	}
	else if (currentMetaPath != binMetaFilePath)
	{
		currentMetaPath = binMetaFilePath;

		ReadMipMapData(currentMetaPath.string(), desiredMipLevels, mipMapFilter);
	}
}

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

	fs::path currentMetaPath = "";

	uint32_t desiredMipLevels = 1;
	flat::MipMapFilter mipMapFilter = flat::MipMapFilter_BOX;


	for (auto& p : fs::directory_iterator(inputPath))
	{
		if (p.is_regular_file() && p.file_size() > 0 && p.path().filename().string() == "meta.json")
		{
			CreateMetaDataFile(p.path(), inputPath, outputPath, currentMetaPath, desiredMipLevels, mipMapFilter);
		}
	}

	for (auto& p : fs::recursive_directory_iterator(inputPath))
	{
		fs::path newOutputPath = GetOutputPathForInputDirectory(outputPath, inputPath, p.path());

		std::string extension = p.path().extension().string();

	//	printf("Next file or folder: %s\n", p.path().string().c_str());

		if (p.is_directory())
		{
			if(SkipDirectory(p.path()))
				continue;

			if (!fs::exists(newOutputPath))
			{
				printf("Making new directory: %s\n", newOutputPath.string().c_str());

				fs::create_directory(newOutputPath);
			}

			//make the meta.tmet file 
			for (auto& sp : fs::directory_iterator(p.path()))
			{
				if (fs::is_regular_file(sp) && sp.path().filename().string() == "meta.json" )
				{
					CreateMetaDataFile(sp.path(), inputPath, outputPath, currentMetaPath, desiredMipLevels, mipMapFilter);
				}
			}
		}
		else if (p.is_regular_file() && p.file_size() > 0)
		{
			

			if (IsImage(extension) && !SkipDirectory(p.path().parent_path()))
			{
				r2::assets::assetlib::ConvertImage(p.path(), newOutputPath, extension, std::max(desiredMipLevels, (uint32_t)1), mipMapFilter);
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

