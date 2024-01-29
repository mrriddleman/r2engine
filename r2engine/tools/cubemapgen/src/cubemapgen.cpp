#include <iostream>
#include <ibl/Cubemap.h>
#include "cmdln/CommandLine.h"
#include <string>
#include "ibl/Image.h"
#include "ibl/Utils.h"
#include "ibl/CubemapUtils.h"
#include "ibl/CubemapIBL.h"
#include <filesystem>
#include "stb_image_write.h"
#include "TexturePackMetaData_generated.h"
#include "flatbuffers/flatbuffers.h"
#include <fstream>
#include <cstdlib>
#include <cstdio>



template<typename T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
constexpr inline T ctz(T x) noexcept {
	static_assert(sizeof(T) * CHAR_BIT <= 64, "details::ctz() only support up to 64 bits");
	T c = sizeof(T) * CHAR_BIT;
	x &= -x;    // equivalent to x & (~x + 1)
	if (x) c--;
	if (sizeof(T) * CHAR_BIT >= 64) {
		if (x & T(0x00000000FFFFFFFF)) c -= 32;
	}
	if (x & T(0x0000FFFF0000FFFF)) c -= 16;
	if (x & T(0x00FF00FF00FF00FF)) c -= 8;
	if (x & T(0x0F0F0F0F0F0F0F0F)) c -= 4;
	if (x & T(0x3333333333333333)) c -= 2;
	if (x & T(0x5555555555555555)) c -= 1;
	return c;
}

using namespace std;

namespace fs = std::filesystem;

namespace
{
	const size_t IBL_DEFAULT_SIZE = 512;
	const std::string EXT = ".hdr";
	const size_t IBL_MIN_LOD_SIZE = 16;
	static const size_t DFG_LUT_DEFAULT_SIZE = 128;
}


struct Arguments
{
	bool quiet = false;
	bool help = false;
	bool writeMipChain = false;
	bool prefilter = false;
	bool lutDFG = false;
	bool diffuseIrradiance = false;
	bool multiscatter = false;
	string inputFile = "";
	string iblOutputDir = "";
	string mipChainOutputDir = "";
	uint32_t iblsamples = 1024;
	uint32_t outputSize = 0;
	uint32_t numMipsToGenerate = 1;
};

void GenerateMipMaps(std::vector<r2::ibl::Cubemap>& levels, std::vector<r2::ibl::Image>& images);
void IBLDiffuseIrradiance(
	const std::string& inputFile,
	const std::vector<r2::ibl::Cubemap>& levels,
	const std::string& outputDir,
	uint32_t outputSize,
	bool quiet,
	uint32_t numSamples);

float LodToPerceptualRoughness(float lod);

void IBLRoughnessPrefilter(
	const std::string& inputFile,
	const std::vector<r2::ibl::Cubemap>& levels,
	bool prefilter,
	const std::string& outputDir,
	uint32_t outputSize,
	uint32_t minLodSize,
	uint32_t maxNumSamples,
	bool quiet);

void IBLMipmapPrefilter(
	const std::string& inputFile,
	const std::vector<r2::ibl::Image>& images,
	const std::vector<r2::ibl::Cubemap>& levels,
	const std::string& outputDir);

void IBLLutDFG(
	const std::string& inputFile,
	const std::string& outputDir,
	size_t size,
	bool multiscatter,
	bool cloth);

void SaveCubemap(const std::string& filename, const r2::ibl::Image& image);

bool WriteFile(const std::string& filePath, void* data, size_t size);
flat::CubemapSide GetCubemapSide(r2::ibl::Cubemap::Face face);
bool WriteAssetDirectory(const std::string& outputDir, const std::vector<std::string>& name, const std::vector<r2::ibl::Cubemap>& cubemap, bool writeMetaData = true, flat::MipMapFilter = flat::MipMapFilter_BOX, flat::TextureProcessType = flat::TextureProcessType_NONE);


int RunSystemCommand(const char* command)
{
	int result = system(command);
	if (result != 0)
	{
		printf("Failed to run command: %s\n\n with result: %i\n", command, result);
	}

	return result;
}

bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
{
	char command[2048];
	std::string flatc = R2_FLATC;

	sprintf_s(command, 2048, "%s -t -o %s %s -- %s --raw-binary", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sourcePath.c_str());

	return RunSystemCommand(command) == 0;
}

int main(int argc, char* argv[])
{

	//system("pause");

	Arguments arguments;
	r2::cmdln::CommandLine args("Cubemapgen will create a cubemap based on an input equirectangular HDR map");

	args.AddArgument({ "-q", "--quiet" }, &arguments.quiet, "Surpress warnings");
	args.AddArgument({ "-h", "--help" }, &arguments.help, "Show help");
	args.AddArgument({ "-i", "--input" }, &arguments.inputFile, "Input file");
	args.AddArgument({ "-o", "--iblOutputDir" }, &arguments.iblOutputDir, "Output directory of the convolution cubemap");
	args.AddArgument({ "-a", "--mipChainOutputDir" }, &arguments.mipChainOutputDir, "Output directory of the mipchain");
	args.AddArgument({ "-n", "--numSamples" }, &arguments.iblsamples, "The number of ibl samples to use. The default is 1024");
	args.AddArgument({ "-m", "--mipChain" }, &arguments.writeMipChain, "Writes the mip chain to the mip directory");
	args.AddArgument({ "-c", "--numMips" }, &arguments.numMipsToGenerate, "Writes up to the amount of mip levels in the chain");
	args.AddArgument({ "-p", "--prefilter" }, &arguments.prefilter, "Make the prefilter cubemap + mipchain");
	args.AddArgument({ "-l", "--lutDFG" }, &arguments.lutDFG, "Make the lut dfg image");
	args.AddArgument({ "-d", "--diffuseIrradiance" }, &arguments.diffuseIrradiance, "Make the diffuse irradiance cubemap");
	args.AddArgument({ "-s", "--multiScatter" }, &arguments.multiscatter, "Use multiscattering");
	args.Parse(argc, argv);

	if (arguments.help)
	{
		args.PrintHelp();
	}

	if (!arguments.inputFile.empty())
	{
		r2::ibl::Image inputImage;
		
		std::vector<r2::ibl::Image> images;
		std::vector<r2::ibl::Cubemap> levels;
		
		int numChannels =r2::ibl::Image::LoadImage(arguments.inputFile, inputImage);


		if (!inputImage.IsValid())
		{
			printf("Unable to open image: %s\n", arguments.inputFile);
			return 1;
		}

		if (numChannels != 3)
		{
			printf("Unable to read the file: %s. It doesn't have 3 channels it has: %d channels.\n", arguments.inputFile, numChannels);
			return 1;
		}

		const size_t width = inputImage.GetWidth();
		const size_t height = inputImage.GetHeight();

		if ((r2::ibl::isPOT(width) && (width * 3 == height * 4)) ||
			(r2::ibl::isPOT(height) && (height * 3 == width * 4)))
		{
			//this is a cross cubemap

			size_t dim = arguments.outputSize > 0 ? arguments.outputSize : IBL_DEFAULT_SIZE;

			if (!arguments.quiet)
			{
				printf("Loading cross cubemap....\n");
			}

			r2::ibl::Image temp;
			r2::ibl::Cubemap cml = r2::ibl::CubemapUtils::CreateCubemap(temp, dim);
			r2::ibl::CubemapUtils::CrossToCubemap(cml, inputImage);

			

			images.push_back(std::move(temp));
			levels.push_back(std::move(cml));

		}
		else if (width == 2 * height)
		{
			//equirectangle 

			size_t dim = arguments.outputSize > 0 ? arguments.outputSize : IBL_DEFAULT_SIZE;

			if (!arguments.quiet)
			{
				printf("Converting equirectangular image...\n");
			}

			r2::ibl::Image temp;
			r2::ibl::Cubemap cml (r2::ibl::CubemapUtils::CreateCubemap(temp, dim));

			r2::ibl::CubemapUtils::EquirectangularToCubemap(cml, inputImage);

			images.push_back(std::move(temp));
			levels.push_back(std::move(cml));

		}
		else
		{
			printf("Aspect ratio not supported: %dx%d\n",width, height);
			printf("Supported aspect ratios:\n");
			printf("  2:1, lat/long or equirectangular\n");
			printf("  3:4, vertical cross (height must be power of two)\n");
			printf("  4:3, horizontal cross (width must be power of two)\n");
			return 0;
		}

		//mirror the image since this is used as a skybox basically
		{
			if (!arguments.quiet)
			{
				printf("Mirroring image...\n");
			}

			r2::ibl::Image temp;
			r2::ibl::Cubemap cml = r2::ibl::CubemapUtils::CreateCubemap(temp, levels[0].GetDimensions());

			r2::ibl::CubemapUtils::MirrorCubemap(cml, levels[0]);

			std::swap(levels[0], cml);
			std::swap(images[0], temp);

		}
		
		//SaveCubemap("D:\\full.hdr", images[0]);

		levels[0].MakeSeamless();
		if (!arguments.quiet)
		{
			printf("Generating mipmaps...\n");
		}
		
		GenerateMipMaps(levels, images);

		if (arguments.writeMipChain)
		{
			const size_t numMipLevels = std::min(levels.size(), (size_t)arguments.numMipsToGenerate);

			std::vector<std::string> names;

			for (size_t mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
			{
				char mipLevelStr[128];
				sprintf_s(mipLevelStr, "%zu", mipLevel);

				std::string name = fs::path(arguments.mipChainOutputDir).stem().string() + mipLevelStr;

				names.push_back(name);
			}

			WriteAssetDirectory(arguments.mipChainOutputDir, names, levels, true);
		}

		if (arguments.prefilter)
		{
			if (!arguments.quiet)
			{
				printf("Generating prefilter map....\n");
			}

			IBLRoughnessPrefilter(
				arguments.inputFile,
				levels,
				true,
				arguments.iblOutputDir + "_prefilter",
				arguments.outputSize > 0 ? arguments.outputSize : IBL_DEFAULT_SIZE,
				IBL_MIN_LOD_SIZE, arguments.iblsamples, arguments.quiet);
		}

		if (arguments.lutDFG)
		{
			if (!arguments.quiet)
			{
				printf("Generating LUT DFG....\n");
			}

			size_t size = arguments.outputSize > 0 ? arguments.outputSize : DFG_LUT_DEFAULT_SIZE;
			IBLLutDFG(arguments.inputFile, arguments.iblOutputDir + "_lutDFG", size, arguments.multiscatter, false);
		}

		if(arguments.diffuseIrradiance)
		{
			if (!arguments.quiet)
			{
				printf("Generating diffuse irradiance...\n");
			}

			IBLDiffuseIrradiance(
				arguments.inputFile,
				levels,
				arguments.iblOutputDir + "_conv",
				arguments.outputSize > 0 ? arguments.outputSize : IBL_DEFAULT_SIZE,
				arguments.quiet,
				arguments.iblsamples);
		}
		

	}
	else
	{
		printf("We don't have a proper input file!");
	}
	

	return 0;
}

void GenerateMipMaps(std::vector<r2::ibl::Cubemap>& levels, std::vector<r2::ibl::Image>& images)
{
	r2::ibl::Image temp;
	const r2::ibl::Cubemap& base(levels[0]);
	size_t dim = base.GetDimensions();
	size_t mipLevel = 0;

	while (dim > 1)
	{
		dim >>= 1u;
		r2::ibl::Cubemap dst = r2::ibl::CubemapUtils::CreateCubemap(temp, dim);
		const r2::ibl::Cubemap& src(levels[mipLevel++]);
		r2::ibl::CubemapUtils::DownsampleBoxFilter(dst, src);
		dst.MakeSeamless();
		images.push_back(std::move(temp));
		levels.push_back(std::move(dst));
	}
}

void IBLDiffuseIrradiance(
	const std::string& inputFile,
	const std::vector<r2::ibl::Cubemap>& levels,
	const std::string& outputDir,
	uint32_t outputSize,
	bool quiet,
	uint32_t numSamples)
{
	const size_t baseExp = ctz(outputSize > 0 ? outputSize : IBL_DEFAULT_SIZE);
	const size_t dim = 1U << baseExp;
	r2::ibl::Image image;

	r2::ibl::Cubemap dst = r2::ibl::CubemapUtils::CreateCubemap(image, dim);

	r2::ibl::CubemapIBL::DiffuseIrradiance(dst, levels, numSamples, [quiet](size_t x, float progress, void* userdata)
	{
		if (!quiet)
		{
				//output

		}
	});

	dst.MakeSeamless();

	std::vector<std::string> names;
	names.push_back(fs::path(inputFile).stem().string() + "_conv");
	std::vector<r2::ibl::Cubemap> cubemaps;
	cubemaps.push_back(std::move(dst));

	WriteAssetDirectory(outputDir, names, cubemaps, true, flat::MipMapFilter_BOX, flat::TextureProcessType_CONVOLVED);
}

void IBLRoughnessPrefilter(
	const std::string& inputFile,
	const std::vector<r2::ibl::Cubemap>& levels,
	bool prefilter,
	const std::string& outputDir,
	uint32_t outputSize,
	uint32_t minLodSize,
	uint32_t maxNumSamples,
	bool quiet)
{
	const size_t baseExp = ctz(outputSize > 0 ? outputSize : IBL_DEFAULT_SIZE);
	size_t minLod = ctz(minLodSize > 0 ? minLodSize : IBL_MIN_LOD_SIZE);
	if (minLod >= baseExp)
	{
		minLod = 0;
	}

	size_t numSamples = maxNumSamples;

	const size_t numLevels = (baseExp + 1) - minLod;

	std::vector<std::string> levelNames;
	std::vector<r2::ibl::Cubemap> cubemapsToSave;


	for (int64_t i = baseExp; i >= int64_t((baseExp + 1) - numLevels); --i)
	{
		const size_t dim = 1U << i;
		const size_t level = baseExp - i;
		if (level >= 2)
		{
			numSamples *= 2;
		}

		const float lod = glm::clamp(level / (numLevels - 1.0f), 0.0f, 1.0f);
		const float perceptualRoughness = LodToPerceptualRoughness(lod);
		const float roughness = perceptualRoughness * perceptualRoughness;
		if (!quiet)
		{
			printf("Level: %zu, roughness: %f, perceptual roughness: %f\n", level, roughness, perceptualRoughness);
		}

		r2::ibl::Image image;
		r2::ibl::Cubemap dst = r2::ibl::CubemapUtils::CreateCubemap(image, dim);

		r2::ibl::CubemapIBL::RoughnessFilter(dst, levels, roughness, numSamples, glm::vec3(1), prefilter,
			[quiet](size_t index, float v, void* userdata){
			if (!quiet)
			{
				//todo
			}
		});

		dst.MakeSeamless();


		levelNames.push_back(fs::path(inputFile).stem().string() + std::to_string(level) + "_prefilter");
		cubemapsToSave.push_back(std::move(dst));
	}

	WriteAssetDirectory(outputDir, levelNames, cubemapsToSave, true, flat::MipMapFilter_BOX, flat::TextureProcessType_PREFILTER);
}

void IBLMipmapPrefilter(
	const std::string& inputFile,
	const std::vector<r2::ibl::Image>& images,
	const std::vector<r2::ibl::Cubemap>& levels,
	const std::string& outputDir)
{

}

void IBLLutDFG(
	const std::string& inputFile,
	const std::string& outputDir,
	size_t imageSize,
	bool multiscatter,
	bool cloth)
{
	r2::ibl::Image image(imageSize, imageSize);
	r2::ibl::CubemapIBL::DFG(image, multiscatter, cloth);

	if (!fs::exists(outputDir))
	{
		fs::create_directory(outputDir);
	}

	std::filesystem::path outputPath(outputDir);
	fs::path cubemapFilesPath = outputPath / fs::path("albedo");

	if (!fs::exists(cubemapFilesPath))
	{
		fs::create_directory(cubemapFilesPath);
	}

	std::string filename = fs::path(inputFile).stem().string() + "_LutDFG" + EXT;

	std::string filePath =
		(cubemapFilesPath / fs::path(filename)).string();

	SaveCubemap(filePath, image);

	auto textureType = flat::TextureType_TEXTURE;
	flatbuffers::FlatBufferBuilder builder;

	auto data = flat::CreateTexturePackMetaData(builder, textureType, 0, 0, flat::MipMapFilter_BOX, flat::TextureProcessType_LUT_DFG);

	builder.Finish(data);

	uint8_t* buf = builder.GetBufferPointer();
	auto size = builder.GetSize();

	fs::path binPath = (fs::path(outputDir) / fs::path("meta.bin"));
	bool wroteCubemapData = WriteFile(binPath.string(), buf, size);
	assert(wroteCubemapData);
	std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

	fs::path schemaPath = fs::path(flatbufferSchemaPath) / fs::path("TexturePackMetaData.fbs");

	bool jsonResult = GenerateFlatbufferJSONFile(outputDir, schemaPath.string(), binPath.string());
	assert(jsonResult);

	fs::remove(binPath);
}

bool WriteAssetDirectory(const std::string& outputDir, const std::vector<std::string>& names, const std::vector<r2::ibl::Cubemap>& cubemaps, bool writeMetaData, flat::MipMapFilter mipMapFilter, flat::TextureProcessType processType)
{
	assert(names.size() <= cubemaps.size());

	if (!fs::exists(outputDir))
	{
		fs::create_directory(outputDir);
	}

	std::filesystem::path outputPath(outputDir);
	fs::path cubemapFilesPath = outputPath / fs::path("albedo");

	if (!fs::exists(cubemapFilesPath))
	{
		fs::create_directory(cubemapFilesPath);
	}

	auto textureType = flat::TextureType_CUBEMAP;
	flatbuffers::FlatBufferBuilder builder;

	std::vector<flatbuffers::Offset<flat::MipLevel>> mipLevels;
	std::string cubemapOutputDir = cubemapFilesPath.string();

	for (size_t n = 0; n < names.size(); ++n)
	{
		const std::string& name = names[n];
		const r2::ibl::Cubemap& cubemap = cubemaps[n];

		std::vector<flatbuffers::Offset<flat::CubemapSideEntry>> sides;

		for (size_t j = 0; j < 6; ++j)
		{
			r2::ibl::Cubemap::Face face = (r2::ibl::Cubemap::Face)j;

			flat::CubemapSide side = GetCubemapSide(face);

			std::string filename = name + "_" + std::string(r2::ibl::CubemapUtils::GetFaceName(face)) + EXT;

			std::string filePath =
				(fs::path(cubemapOutputDir) / fs::path(filename)).string();

			if (writeMetaData)
			{
				sides.push_back(flat::CreateCubemapSideEntry(builder, builder.CreateString(filename), side));
			}

			SaveCubemap(filePath, cubemap.GetImageCopyForFace(face));
		}

		mipLevels.push_back(flat::CreateMipLevel(builder, n, builder.CreateVector(sides)));
	}

	if (writeMetaData)
	{
		auto data = flat::CreateTexturePackMetaData(builder, textureType, builder.CreateVector(mipLevels), mipLevels.size(), mipMapFilter, processType);

		builder.Finish(data);

		uint8_t* buf = builder.GetBufferPointer();
		auto size = builder.GetSize();

		fs::path binPath = (fs::path(outputDir) / fs::path("meta.bin"));
		bool wroteCubemapData = WriteFile(binPath.string(), buf, size);
		assert(wroteCubemapData);
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		fs::path schemaPath = fs::path(flatbufferSchemaPath) / fs::path("TexturePackMetaData.fbs");

		bool jsonResult = GenerateFlatbufferJSONFile(outputDir, schemaPath.string(), binPath.string());
		assert(jsonResult);

		fs::remove(binPath);
	}
	
	return true;
}

flat::CubemapSide GetCubemapSide(r2::ibl::Cubemap::Face face)
{
	//@NOTE: we're remapping this to the internal engine layout. ie. reversing right and left, front and back
	switch (face)
	{
	case r2::ibl::Cubemap::Face::PX:
		 return flat::CubemapSide_RIGHT;
		break;
	case r2::ibl::Cubemap::Face::NX:
		return flat::CubemapSide_LEFT;
		break;
	case r2::ibl::Cubemap::Face::PY:
		return flat::CubemapSide_TOP;
		break;
	case r2::ibl::Cubemap::Face::NY:
		return flat::CubemapSide_BOTTOM;
		break;
	case r2::ibl::Cubemap::Face::PZ:
		return flat::CubemapSide_FRONT;
		break;
	case r2::ibl::Cubemap::Face::NZ:
		
		return flat::CubemapSide_BACK;
		break;
	default:
		assert(false);
		break;
	}
}

void SaveCubemap(const std::string& filename, const r2::ibl::Image& image)
{
	stbi_write_hdr(filename.c_str(), image.GetWidth(), image.GetHeight(), 3, (float*)image.GetData());
}

bool WriteFile(const std::string& filePath, void* data, size_t size)
{
	//@TODO(Serge): we need to write out the asset stuff for a cubemap meta data here
	std::fstream stream;
	stream.open(filePath, std::fstream::out | std::fstream::binary);
	if (stream.good())
	{
		stream.write((const char*)data, size);

		assert(stream.good() && "Failed to write out the buffer!");
		if (!stream.good())
		{
			return false;
		}
	}

	stream.close();
	return true;
}

float LodToPerceptualRoughness(float lod)
{
	const float a = 2.0f;
	const float b = -1.0f;
	return (lod != 0) ?
		glm::clamp((std::sqrtf(a * a + 4.0f * b * lod) - a) / (2.0f * b), 0.0f, 1.0f)
		: 0.0f;
} 