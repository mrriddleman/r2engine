#include <iostream>

#include "AssetConverterAssetFile.h"
#include "assetlib/TextureAsset.h"
#include "assetlib/TextureMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"

#define STB_IMAGE_IMPLEMENTATION
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
			if (!fs::exists(newOutputPath) && !SkipDirectory(p.path()))
			{
				printf("Making new directory: %s\n", newOutputPath.string().c_str());

				fs::create_directory(newOutputPath);
			}

			continue;
		}
		else if (p.is_regular_file() && p.file_size() > 0)
		{
			if (IsImage(extension) && !SkipDirectory(p.path().parent_path()))
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
//	printf("Converting file: %s, and putting it in directory: %s, extension: %s\n", inputFilePath.string().c_str(), parentOutputDir.string().c_str(), extension.c_str());

	void* pixels = nullptr;
	int width = 0;
	int height = 0;
	int channels = 0;

	flat::TextureFormat textureFormat;

	nvtt::Surface image0;
	nvtt::SurfaceSet images;

	if (extension == DDS_EXTENSION)
	{
		bool result = images.loadDDS(inputFilePath.string().c_str());

		height = images.GetHeight();
		width = images.GetWidth();
		
		images.GetSurface(0, 0, image0);

		auto alphaMode = image0.alphaMode();

		if (alphaMode == nvtt::AlphaMode_None)
		{
			image0.addChannel(image0, 0, 3, 1);
		}

		pixels = image0.data();
		textureFormat = flat::TextureFormat_RGBA8;
	}
	else if (extension == HDR_EXTENSION)
	{
		stbi_set_flip_vertically_on_load(false);
		pixels = stbi_loadf(inputFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		textureFormat = flat::TextureFormat_RGBA32;
	}
	else
	{
		stbi_set_flip_vertically_on_load(true);
		pixels = stbi_load(inputFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		textureFormat = flat::TextureFormat_RGBA8;
	}


	if (pixels == nullptr)
	{
		printf("Failed to load the file: %s\n", inputFilePath.string().c_str());
		return 0;
	}


	struct DumbHandler : nvtt::OutputHandler
	{
		virtual bool writeData(const void* data, int size)
		{
			for (int i = 0; i < size; ++i)
			{
				buffer.push_back(((char*)data)[i]);
			}

			return true;
		}

		virtual void beginImage(int size, int width, int height, int depth, int face, int mipLevel) {};

		virtual void endImage() {};

		std::vector<char> buffer;
	};

	nvtt::Context context;
	context.enableCudaAcceleration(true);

	nvtt::CompressionOptions compressorOptions;
	compressorOptions.setFormat(nvtt::Format::Format_RGBA);
	compressorOptions.setPixelType(nvtt::PixelType_UnsignedNorm);

	nvtt::OutputOptions outputOptions;

	nvtt::Surface outputSurface;

	DumbHandler handler;
	outputOptions.setOutputHandler(&handler);

	nvtt::InputFormat inputFormat = nvtt::InputFormat_BGRA_8UB;
	
	uint64_t textureSize = width * height * 4;

	if (textureFormat == flat::TextureFormat_RGBA32)
	{
		inputFormat = nvtt::InputFormat_RGBA_32F;
		textureSize = width * height * sizeof(float) * 4;
	}

	std::vector<flatbuffers::Offset<flat::MipInfo>> metaMipInfo;
	std::vector<char> all_buffer;
	
	all_buffer.reserve(textureSize);

	flatbuffers::FlatBufferBuilder builder;

	outputSurface.setImage(inputFormat, width, height, 1, pixels);
	const auto numMipMaps = outputSurface.countMipmaps();
	
//	printf("Number of mips calculated: %i\n", numMipMaps);

	for(int m = 0; m < numMipMaps; ++m)
	{
		context.compress(outputSurface, 0, m, compressorOptions, outputOptions);

		metaMipInfo.push_back( flat::CreateMipInfo(builder, outputSurface.width(), outputSurface.height(), 0, handler.buffer.size()));
		
		all_buffer.insert(all_buffer.end(), handler.buffer.begin(), handler.buffer.end());

		handler.buffer.clear();

		if (m == numMipMaps - 1) break;
		
		outputSurface.buildNextMipmap(nvtt::MipmapFilter_Kaiser);
	}
	
	auto textureMetaData = flat::CreateTextureMetaData(builder, builder.CreateString(inputFilePath.string()), all_buffer.size(), textureFormat, flat::CompressionMode_LZ4, builder.CreateVector(metaMipInfo));
	builder.Finish(textureMetaData, "rtex");

	uint8_t* buf = builder.GetBufferPointer();
	size_t size = builder.GetSize();
	flat::TextureMetaData* metaData = flat::GetMutableTextureMetaData(buf);

	AssetConverterAssetFile assetFile;

	assetFile.metaData.data = (char*)buf;
	assetFile.metaData.size = size;

	assetFile.binaryBlob.size = all_buffer.size();

	assetFile.AllocateForBlob(assetFile.binaryBlob);

	//printf("Before packing texture\n");

	r2::assets::assetlib::pack_texture(assetFile, metaData, all_buffer.data());

	fs::path filenamePath = inputFilePath.filename();
	
	fs::path outputFilePath = parentOutputDir / filenamePath.replace_extension(RTEX_EXTENSION);

	if (extension != DDS_EXTENSION)
	{
		stbi_image_free(pixels);
	}

//	printf("Before saving binary file\n");

	bool result = r2::assets::assetlib::save_binaryfile(outputFilePath.string().c_str(), assetFile);

	if (!result)
	{
		printf("Failed to output file: %s\n", outputFilePath.string().c_str());
		return false;
	}

	//printf("Finished outputting: %s\n", outputFilePath.string().c_str());
	return result;
}