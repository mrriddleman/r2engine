#include <iostream>


#include "assetlib/TextureAsset.h"
#include "assetlib/ImageConvert.h"
#include "assetlib/ModelConvert.h"

#include "TexturePackMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"

#include <filesystem>
#include <string>

#include <fstream>

#include <algorithm>

namespace fs = std::filesystem;


namespace
{
	//textures
	std::string PNG_EXTENSION = ".png";
	std::string JPG_EXTENSION = ".jpg";
	std::string TIF_EXTENSION = ".tif";
	std::string TGA_EXTENSION = ".tga";
	std::string DDS_EXTENSION = ".dds";
	std::string HDR_EXTENSION = ".hdr";

	//models
	std::string GLTF_EXTENSION = ".gltf";
	std::string GLB_EXTENSION = ".glb";
}

struct Arguments
{
	std::string inputDir = "";
	std::string outputDir = "";
	std::string materialParamsManifestPath = "";
	std::string rawMaterialsParentPath = "";
	std::string engineTexturePacksManifestPath = "";
	std::string texturePacksManifestPath = "";
	uint32_t animationSamples = 60;
	bool forceMaterialRebuild = false;
};

bool SkipDirectory(const fs::path& p)
{
	return p.stem().string() == "hdr" || p.stem().string() == "Animations";
}

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
	std::string theExtension = extension;

	std::transform(theExtension.begin(), theExtension.end(), theExtension.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return	theExtension == GLTF_EXTENSION || theExtension == GLB_EXTENSION;
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
	args.AddArgument({ "-m", "--material" }, &arguments.materialParamsManifestPath, "Material Params Manifest Path");
	args.AddArgument({ "-r", "--rawmaterialsparent" }, &arguments.rawMaterialsParentPath, "Raw Material Parent Path");
	args.AddArgument({ "-e", "--enginetexturepacksmanifest" }, &arguments.engineTexturePacksManifestPath, "Engine Texture Packs Manifest Path");
	args.AddArgument({ "-t", "--texturepacksmanifest" }, &arguments.texturePacksManifestPath, "Texture Packs Manifest Path");
	args.AddArgument({ "-f", "--forcerematerialrebuild" }, &arguments.forceMaterialRebuild, "Force Material Rebuild");
	args.AddArgument({ "-s", "--animationsamples" }, &arguments.animationSamples, "Number of animation samples");
	args.Parse(agrc, argv);

	//arguments.inputDir = "D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Models\\truffleman\\truffleman.gltf";
	//arguments.outputDir = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Models\\truffleman";
	//arguments.materialParamsManifestPath = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Materials\\manifests\\SandboxMaterialPack.mpak";
	//arguments.rawMaterialsParentPath = "D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Materials\\generated_materials_test";
	//arguments.engineTexturePacksManifestPath = "D:\\Projects\\r2engine\\r2engine\\assets_bin\\textures\\manifests\\engine_texture_pack.tman";
	//arguments.texturePacksManifestPath = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Textures\\manifests\\SandboxTexturePack.tman";
	//arguments.forceMaterialRebuild = false;

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
	fs::path materialParamsManifestPath{ arguments.materialParamsManifestPath };
	fs::path rawMaterialsParentPath{ arguments.rawMaterialsParentPath };
	fs::path engineTexturePacksManifestPath{ arguments.engineTexturePacksManifestPath };
	fs::path texturePacksManifestPath{ arguments.texturePacksManifestPath };
	uint32_t numberOfAnimationSamples = arguments.animationSamples;
	bool forceRebuild = arguments.forceMaterialRebuild;

	fs::path currentMetaPath = "";

	uint32_t desiredMipLevels = 1;
	flat::MipMapFilter mipMapFilter = flat::MipMapFilter_BOX;


	if (fs::is_regular_file(inputPath))
	{
		std::string extension = inputPath.extension().string();

		if (IsModel(extension))
		{
			r2::assets::assetlib::ConvertModel(inputPath, outputPath, materialParamsManifestPath, rawMaterialsParentPath, engineTexturePacksManifestPath, texturePacksManifestPath, forceRebuild, numberOfAnimationSamples);
		}

		//@TODO(Serge): other types here
	}
	else if(fs::is_directory(inputPath))
	{
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
				if (SkipDirectory(p.path()))
					continue;

				if (!fs::exists(newOutputPath))
				{
					printf("Making new directory: %s\n", newOutputPath.string().c_str());

					fs::create_directory(newOutputPath);
				}

				//make the meta.tmet file 
				for (auto& sp : fs::directory_iterator(p.path()))
				{
					if (fs::is_regular_file(sp) && sp.path().filename().string() == "meta.json")
					{
						CreateMetaDataFile(sp.path(), inputPath, outputPath, currentMetaPath, desiredMipLevels, mipMapFilter);
					}
				}
			}
			else if (p.is_regular_file() && p.file_size() > 0)
			{
				if (p.path().parent_path().stem().string() == "Animations")
				{
					continue;
				}

				if (IsImage(extension) && !SkipDirectory(p.path().parent_path()))
				{
					r2::assets::assetlib::ConvertImage(p.path(), newOutputPath, extension, std::max(desiredMipLevels, (uint32_t)1), mipMapFilter);
				}
				else if (IsModel(extension))
				{
					r2::assets::assetlib::ConvertModel(p.path(), newOutputPath, materialParamsManifestPath, rawMaterialsParentPath, engineTexturePacksManifestPath, texturePacksManifestPath, forceRebuild, numberOfAnimationSamples);
				}
			}
		}
	}

	

	return 0;
}

