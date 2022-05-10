#include <iostream>

#include "AssetConverterAssetFile.h"
#include "assetlib/TextureAsset.h"
#include "assetlib/TextureMetaData_generated.h"
#include "TexturePackMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

#define dds_imax(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) \
                  (((d & 0xFF) << 24) \
                |  ((c & 0xFF) << 16) \
                |  ((b & 0xFF) <<  8) \
                |  ((a & 0xFF) <<  0))
#endif

	typedef uint8_t BYTE;
	typedef uint32_t  UINT;
	typedef uint32_t DWORD;
	typedef int errno_t;


	//from stb image
	void VerticalFlip(void* image, int w, int h, int bytes_per_pixel)
	{
		int row;
		size_t bytes_per_row = (size_t)w * bytes_per_pixel;
		stbi_uc temp[2048];
		stbi_uc* bytes = (stbi_uc*)image;

		for (row = 0; row < (h >> 1); row++) {
			stbi_uc* row0 = bytes + row * bytes_per_row;
			stbi_uc* row1 = bytes + (h - row - 1) * bytes_per_row;
			// swap row0 with row1
			size_t bytes_left = bytes_per_row;
			while (bytes_left) {
				size_t bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
				memcpy(temp, row0, bytes_copy);
				memcpy(row0, row1, bytes_copy);
				memcpy(row1, temp, bytes_copy);
				row0 += bytes_copy;
				row1 += bytes_copy;
				bytes_left -= bytes_copy;
			}
		}
	}

	struct DDSTextureDetails
	{
		DWORD width = 0;
		DWORD height = 0;

		int mipMapCount = 0;
		void* pixels = nullptr;
		int* sizes = nullptr;
		int* pitches = nullptr;

		uint64_t totalSize = 0;
		int glFormat = 0;
		DWORD d3dFormat = 0;
		bool compressed = false;
	};

	int MipMapHeight(const DDSTextureDetails& ddsTexture, int mipMap) { return dds_imax(1, ddsTexture.height >> mipMap); }
	int MipMapWidth(const DDSTextureDetails& ddsTexture, int mipMap) { return dds_imax(1, ddsTexture.width >> mipMap); }

	// Implementation
	struct DDS_PIXELFORMAT
	{
		DWORD dwSize;
		DWORD dwFlags;
		DWORD dwFourCC;
		DWORD dwRGBBitCount;
		DWORD dwRBitMask;
		DWORD dwGBitMask;
		DWORD dwBBitMask;
		DWORD dwABitMask;
	};

	// ------------------------------------------------------------------------------------------------
	struct DDS_HEADER
	{
		DWORD           dwSize;
		DWORD           dwFlags;
		DWORD           dwHeight;
		DWORD           dwWidth;
		DWORD           dwPitchOrLinearSize;
		DWORD           dwDepth;
		DWORD           dwMipMapCount;
		DWORD           dwReserved1[11];
		DDS_PIXELFORMAT ddspf;
		DWORD           dwCaps;
		DWORD           dwCaps2;
		DWORD           dwCaps3;
		DWORD           dwCaps4;
		DWORD           dwReserved2;

		DDS_HEADER()
		{
			memset(this, 0, sizeof(*this));
		}
	};

#ifndef DXGI_FORMAT_DEFINED
	// ------------------------------------------------------------------------------------------------
	enum DXGI_FORMAT
	{
		DXGI_FORMAT_UNKNOWN = 0,
		DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
		DXGI_FORMAT_R32G32B32A32_UINT = 3,
		DXGI_FORMAT_R32G32B32A32_SINT = 4,
		DXGI_FORMAT_R32G32B32_TYPELESS = 5,
		DXGI_FORMAT_R32G32B32_FLOAT = 6,
		DXGI_FORMAT_R32G32B32_UINT = 7,
		DXGI_FORMAT_R32G32B32_SINT = 8,
		DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
		DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
		DXGI_FORMAT_R16G16B16A16_UNORM = 11,
		DXGI_FORMAT_R16G16B16A16_UINT = 12,
		DXGI_FORMAT_R16G16B16A16_SNORM = 13,
		DXGI_FORMAT_R16G16B16A16_SINT = 14,
		DXGI_FORMAT_R32G32_TYPELESS = 15,
		DXGI_FORMAT_R32G32_FLOAT = 16,
		DXGI_FORMAT_R32G32_UINT = 17,
		DXGI_FORMAT_R32G32_SINT = 18,
		DXGI_FORMAT_R32G8X24_TYPELESS = 19,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
		DXGI_FORMAT_R10G10B10A2_UNORM = 24,
		DXGI_FORMAT_R10G10B10A2_UINT = 25,
		DXGI_FORMAT_R11G11B10_FLOAT = 26,
		DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
		DXGI_FORMAT_R8G8B8A8_UNORM = 28,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
		DXGI_FORMAT_R8G8B8A8_UINT = 30,
		DXGI_FORMAT_R8G8B8A8_SNORM = 31,
		DXGI_FORMAT_R8G8B8A8_SINT = 32,
		DXGI_FORMAT_R16G16_TYPELESS = 33,
		DXGI_FORMAT_R16G16_FLOAT = 34,
		DXGI_FORMAT_R16G16_UNORM = 35,
		DXGI_FORMAT_R16G16_UINT = 36,
		DXGI_FORMAT_R16G16_SNORM = 37,
		DXGI_FORMAT_R16G16_SINT = 38,
		DXGI_FORMAT_R32_TYPELESS = 39,
		DXGI_FORMAT_D32_FLOAT = 40,
		DXGI_FORMAT_R32_FLOAT = 41,
		DXGI_FORMAT_R32_UINT = 42,
		DXGI_FORMAT_R32_SINT = 43,
		DXGI_FORMAT_R24G8_TYPELESS = 44,
		DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
		DXGI_FORMAT_R8G8_TYPELESS = 48,
		DXGI_FORMAT_R8G8_UNORM = 49,
		DXGI_FORMAT_R8G8_UINT = 50,
		DXGI_FORMAT_R8G8_SNORM = 51,
		DXGI_FORMAT_R8G8_SINT = 52,
		DXGI_FORMAT_R16_TYPELESS = 53,
		DXGI_FORMAT_R16_FLOAT = 54,
		DXGI_FORMAT_D16_UNORM = 55,
		DXGI_FORMAT_R16_UNORM = 56,
		DXGI_FORMAT_R16_UINT = 57,
		DXGI_FORMAT_R16_SNORM = 58,
		DXGI_FORMAT_R16_SINT = 59,
		DXGI_FORMAT_R8_TYPELESS = 60,
		DXGI_FORMAT_R8_UNORM = 61,
		DXGI_FORMAT_R8_UINT = 62,
		DXGI_FORMAT_R8_SNORM = 63,
		DXGI_FORMAT_R8_SINT = 64,
		DXGI_FORMAT_A8_UNORM = 65,
		DXGI_FORMAT_R1_UNORM = 66,
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
		DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
		DXGI_FORMAT_BC1_TYPELESS = 70,
		DXGI_FORMAT_BC1_UNORM = 71,
		DXGI_FORMAT_BC1_UNORM_SRGB = 72,
		DXGI_FORMAT_BC2_TYPELESS = 73,
		DXGI_FORMAT_BC2_UNORM = 74,
		DXGI_FORMAT_BC2_UNORM_SRGB = 75,
		DXGI_FORMAT_BC3_TYPELESS = 76,
		DXGI_FORMAT_BC3_UNORM = 77,
		DXGI_FORMAT_BC3_UNORM_SRGB = 78,
		DXGI_FORMAT_BC4_TYPELESS = 79,
		DXGI_FORMAT_BC4_UNORM = 80,
		DXGI_FORMAT_BC4_SNORM = 81,
		DXGI_FORMAT_BC5_TYPELESS = 82,
		DXGI_FORMAT_BC5_UNORM = 83,
		DXGI_FORMAT_BC5_SNORM = 84,
		DXGI_FORMAT_B5G6R5_UNORM = 85,
		DXGI_FORMAT_B5G5R5A1_UNORM = 86,
		DXGI_FORMAT_B8G8R8A8_UNORM = 87,
		DXGI_FORMAT_B8G8R8X8_UNORM = 88,
		DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
		DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
		DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
		DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
		DXGI_FORMAT_BC6H_TYPELESS = 94,
		DXGI_FORMAT_BC6H_UF16 = 95,
		DXGI_FORMAT_BC6H_SF16 = 96,
		DXGI_FORMAT_BC7_TYPELESS = 97,
		DXGI_FORMAT_BC7_UNORM = 98,
		DXGI_FORMAT_BC7_UNORM_SRGB = 99,
		DXGI_FORMAT_AYUV = 100,
		DXGI_FORMAT_Y410 = 101,
		DXGI_FORMAT_Y416 = 102,
		DXGI_FORMAT_NV12 = 103,
		DXGI_FORMAT_P010 = 104,
		DXGI_FORMAT_P016 = 105,
		DXGI_FORMAT_420_OPAQUE = 106,
		DXGI_FORMAT_YUY2 = 107,
		DXGI_FORMAT_Y210 = 108,
		DXGI_FORMAT_Y216 = 109,
		DXGI_FORMAT_NV11 = 110,
		DXGI_FORMAT_AI44 = 111,
		DXGI_FORMAT_IA44 = 112,
		DXGI_FORMAT_P8 = 113,
		DXGI_FORMAT_A8P8 = 114,
		DXGI_FORMAT_B4G4R4A4_UNORM = 115,
		DXGI_FORMAT_FORCE_UINT = 0xffffffffUL
	};
#endif

	// ------------------------------------------------------------------------------------------------
	const DWORD DDPF_ALPHAPIXELS = 0x1;
	const DWORD DDPF_ALPHA = 0x2;
	const DWORD DDPF_FOURCC = 0x4;
	const DWORD DDPF_RGB = 0x40;
	const DWORD DDPF_YUV = 0x200;
	const DWORD DDPF_LUMINANCE = 0x20000;

#if !defined(D3D10_APPEND_ALIGNED_ELEMENT)
	// ------------------------------------------------------------------------------------------------
	enum D3D10_RESOURCE_DIMENSION
	{
		D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
		D3D10_RESOURCE_DIMENSION_BUFFER = 1,
		D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
		D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
		D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
	};
#endif

	// ------------------------------------------------------------------------------------------------
	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT              dxgiFormat;
		D3D10_RESOURCE_DIMENSION resourceDimension;
		UINT                     miscFlag;
		UINT                     arraySize;
		UINT                     reserved;
	};

	// ------------------------------------------------------------------------------------------------
	struct DDS_FILEHEADER
	{
		DWORD           dwMagicNumber;
		DDS_HEADER      header;

		DDS_FILEHEADER() { memset(this, 0, sizeof(*this)); }
	};

	const DWORD kDDS_MagicNumber = MAKEFOURCC('D', 'D', 'S', ' ');
	const DWORD kDDS_DX10 = MAKEFOURCC('D', 'X', '1', '0');

	void* PointerAdd(const void* p, uint64_t bytes);
	bool ReadDDSFileData(const void* imageData, uint64_t size, DDSTextureDetails& ddsTextureDetails);
	bool ReadDDSFromFileDXT1(const DDS_HEADER& header, void* imageData, uint64_t size, DDSTextureDetails& ddsTextureDetails);
	void CleanupDDSTextureDetails(const DDSTextureDetails& ddsTextureDetails);

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

bool ConvertImage(const fs::path& parentOutputDir, const fs::path& inputFilePath, const std::string& extension, uint32_t desiredMipLevels, flat::MipMapFilter filter);

int main(int agrc, char** argv)
{
	Arguments arguments;
	r2::cmdln::CommandLine args("Asset Converter will convert raw assets into assets r2 can use");
	args.AddArgument({ "-i", "--input" }, &arguments.inputDir, "Input Directory");
	args.AddArgument({ "-o", "--output" }, &arguments.outputDir, "Output Directory");
	args.Parse(agrc, argv);


//	arguments.inputDir = "D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Textures\\packs";
//	arguments.outputDir = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Textures\\packs";


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
					fs::path newOutputPath = GetOutputPathForInputDirectory(outputPath, inputPath, sp.path());

					fs::path binMetaFilePath = newOutputPath / "meta.tmet";

					if(!fs::exists(binMetaFilePath))
					{ 
						bool result = GenerateTexturePackMetaDataFromJSON(sp.path().string(), newOutputPath.string());

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
					else if(currentMetaPath != binMetaFilePath)
					{
						currentMetaPath = binMetaFilePath;

						ReadMipMapData(currentMetaPath.string(), desiredMipLevels, mipMapFilter);
					}
				}
			}
		}
		else if (p.is_regular_file() && p.file_size() > 0)
		{
			if (IsImage(extension) && !SkipDirectory(p.path().parent_path()))
			{
				ConvertImage(p.path(), newOutputPath, extension, std::max(desiredMipLevels, (uint32_t)1), mipMapFilter);
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

bool ConvertImage(const fs::path& inputFilePath, const fs::path& parentOutputDir,  const std::string& extension, uint32_t desiredMipLevels, flat::MipMapFilter filter)
{
	printf("Converting file: %s, and putting it in directory: %s, extension: %s, desiredMipLevels: %lu, filter: %lu\n", inputFilePath.string().c_str(), parentOutputDir.string().c_str(), extension.c_str(), desiredMipLevels, filter);

	void* pixels = nullptr;
	void* imageData = nullptr;
	int width = 0;
	int height = 0;
	int channels = 0;

	flat::TextureFormat textureFormat;

	nvtt::Surface image0;
	nvtt::SurfaceSet images;

	uint64_t textureSize = 0;

	DDSTextureDetails ddsTextureDetails;
	
	std::vector<flatbuffers::Offset<flat::MipInfo>> metaMipInfo;

	flatbuffers::FlatBufferBuilder builder;

	if (extension == DDS_EXTENSION)
	{
		textureFormat = flat::TextureFormat_COMPRESSED_RGBA_S3TC_DXT1_EXT;

		//bool result = images.loadDDS(inputFilePath.string().c_str());

		//images.GetSurface(0, 0, image0);

		//auto alphaMode = image0.alphaMode();

		//if (alphaMode == nvtt::AlphaMode_None)
		//{
		////	image0.addChannel(image0, 0, 3, 1);
		//}

		//height = image0.height();
		//width = image0.width();
		//
		//textureSize = width * height * 4;

		//imageData = image0.data();
		std::ifstream ddsFile;

		ddsFile.open(inputFilePath.string(), std::ios::in | std::ios::binary);

		ddsFile.seekg(0, std::ios::end);

		std::streampos fSize = ddsFile.tellg();

		ddsFile.seekg(0, std::ios::beg);

		pixels = malloc(fSize);

		ddsFile.read((char*)pixels, fSize);

		ddsFile.close();

		bool result = ReadDDSFileData(pixels, fSize, ddsTextureDetails);

		

		width = ddsTextureDetails.width;
		height = ddsTextureDetails.height;
		imageData = ddsTextureDetails.pixels;
		textureSize = ddsTextureDetails.totalSize;

		DWORD mipWidth = ddsTextureDetails.width;
		DWORD mipHeight = ddsTextureDetails.height;

		desiredMipLevels = ddsTextureDetails.mipMapCount;

		for (int m = 0; m < ddsTextureDetails.mipMapCount; ++m)
		{
			metaMipInfo.push_back(flat::CreateMipInfo(builder, mipWidth, mipHeight, ddsTextureDetails.sizes[m], ddsTextureDetails.sizes[m]));

			mipWidth = dds_imax(1, mipWidth >> 1);
			mipHeight = dds_imax(1, mipHeight >> 1);
		}
	}
	else if (extension == HDR_EXTENSION)
	{
		stbi_set_flip_vertically_on_load(false);
		imageData = stbi_loadf(inputFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		textureFormat = flat::TextureFormat_RGBA32;

		textureSize = width * height * sizeof(float) * 4;

	//	imageData = pixels;


		metaMipInfo.push_back(flat::CreateMipInfo(builder, width, height, textureSize, textureSize));
	}
	else
	{
		stbi_set_flip_vertically_on_load(true);
		imageData = stbi_load(inputFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		textureFormat = flat::TextureFormat_RGBA8;

		
		
		textureSize = width * height * 4;

	//	imageData = pixels;

		metaMipInfo.push_back(flat::CreateMipInfo(builder, width, height, textureSize, textureSize));
	}

	if (imageData == nullptr)
	{
		printf("Failed to load the file: %s\n", inputFilePath.string().c_str());
		return 0;
	}


	//void* pixelData = malloc(textureSize);

	//struct DumbHandler : nvtt::OutputHandler
	//{
	//	virtual bool writeData(const void* data, int size)
	//	{
	//		for (int i = 0; i < size; ++i)
	//		{
	//			buffer.push_back(((char*)data)[i]);
	//		}

	//		return true;
	//	}

	//	virtual void beginImage(int size, int width, int height, int depth, int face, int mipLevel) {};

	//	virtual void endImage() {};

	//	std::vector<char> buffer;
	//};

	//nvtt::Context context;
	//context.enableCudaAcceleration(true);

	//nvtt::CompressionOptions compressorOptions;
	//
	//nvtt::Format format = nvtt::Format::Format_RGBA;

	//if (textureFormat == flat::TextureFormat_COMPRESSED_RGBA_S3TC_DXT1_EXT)
	//{
	//	format = nvtt::Format_BC1a;
	//}

	//compressorOptions.setFormat(format);

	//compressorOptions.setPixelType(nvtt::PixelType_UnsignedNorm);

	//if (textureFormat == flat::TextureFormat_RGBA32)
	//{
	//	compressorOptions.setPixelType(nvtt::PixelType_Float);
	//}

	//nvtt::OutputOptions outputOptions;

	//nvtt::Surface outputSurface;

	//DumbHandler handler;
	//outputOptions.setOutputHandler(&handler);

	//nvtt::InputFormat inputFormat = nvtt::InputFormat_BGRA_8UB;
	//
	//if (textureFormat == flat::TextureFormat_RGBA32)
	//{
	//	inputFormat = nvtt::InputFormat_RGBA_32F;		
	//}

	//outputSurface.setImage(inputFormat, width, height, 1, imageData);
	//
	//const auto numMipMaps = std::min(static_cast<uint32_t>(outputSurface.countMipmaps()), desiredMipLevels);

	//std::vector<char> all_buffer;
	//all_buffer.reserve(textureSize);

	//for(int m = 0; m < numMipMaps; ++m)
	//{
	//	
	//	

	//	context.compress(outputSurface, 0, m, compressorOptions, outputOptions);

	//	auto width = outputSurface.width();
	//	auto height = outputSurface.height();

	//	metaMipInfo.push_back( flat::CreateMipInfo(builder, width, height, handler.buffer.size(), handler.buffer.size()));
	//
	//	all_buffer.insert(all_buffer.end(), handler.buffer.begin(), handler.buffer.end());

	//	handler.buffer.clear();

	//	if (m == numMipMaps - 1) break;
	//	
	//	if (filter == flat::MipMapFilter_KAISER)
	//	{
	//		outputSurface.buildNextMipmap(nvtt::MipmapFilter_Kaiser);
	//	}
	//	else if (filter == flat::MipMapFilter_TRIANGLE)
	//	{
	//		outputSurface.buildNextMipmap(nvtt::MipmapFilter_Triangle);
	//	}
	//	else
	//	{
	//		outputSurface.buildNextMipmap(nvtt::MipmapFilter_Box);
	//	}
	//}
	
	auto textureMetaData = flat::CreateTextureMetaData(builder, builder.CreateString(inputFilePath.string()), textureSize, textureFormat, flat::CompressionMode_LZ4, builder.CreateVector(metaMipInfo));
	builder.Finish(textureMetaData, "rtex");

	uint8_t* buf = builder.GetBufferPointer();
	size_t size = builder.GetSize();
	flat::TextureMetaData* metaData = flat::GetMutableTextureMetaData(buf);

	AssetConverterAssetFile assetFile;

	assetFile.metaData.data = (char*)buf;
	assetFile.metaData.size = size;

	assetFile.binaryBlob.size = textureSize;

	assetFile.AllocateForBlob(assetFile.binaryBlob);

	//printf("Before packing texture\n");

	r2::assets::assetlib::pack_texture(assetFile, metaData, imageData);

	fs::path filenamePath = inputFilePath.filename();
	
	fs::path outputFilePath = parentOutputDir / filenamePath.replace_extension(RTEX_EXTENSION);

	if (extension != DDS_EXTENSION)
	{
		stbi_image_free(imageData);
	}
	else
	{

		CleanupDDSTextureDetails(ddsTextureDetails);
		free(pixels);
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


namespace
{
	void* PointerAdd(const void* p, uint64_t bytes)
	{
		return (void*)((uint8_t*)p + bytes);
	}

	bool ReadDDSFileData(const void* imageData, uint64_t size, DDSTextureDetails& ddsTextureDetails)
	{
		if (!imageData || size == 0)
		{
			assert(false && "invalid data or size is 0");
			return false;
		}

		DDS_FILEHEADER* fileHeader = (DDS_FILEHEADER*)imageData;
		bool success = false;

		if (fileHeader->dwMagicNumber != kDDS_MagicNumber || fileHeader->header.dwSize != 124 || fileHeader->header.ddspf.dwSize != 32)
		{
			return false;
		}

		if ((fileHeader->header.ddspf.dwFlags & DDPF_FOURCC) != 0)
		{
			if (fileHeader->header.ddspf.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
			{
				success = ReadDDSFromFileDXT1(fileHeader->header, PointerAdd(imageData, sizeof(DDS_FILEHEADER)), size - sizeof(DDS_FILEHEADER), ddsTextureDetails);
			}
		}

		if (success)
		{
			ddsTextureDetails.height = fileHeader->header.dwHeight;
			ddsTextureDetails.width = fileHeader->header.dwWidth;
		}

		return success;
	}

	bool ReadDDSFromFileDXT1(const DDS_HEADER& header, void* imageData, uint64_t size, DDSTextureDetails& ddsTextureDetails)
	{
		const DWORD kBlockSizeBytes = 8;
		const DWORD kColsPerBlock = 4;
		const DWORD kRowsPerBlock = 4;

		size_t totalSize = 0;
		DWORD mipmaps = dds_imax(1, header.dwMipMapCount);
		const uint64_t kAlignment = 16;

		ddsTextureDetails.mipMapCount = mipmaps;
		ddsTextureDetails.sizes = new int[mipmaps]; //(GLsizei*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, mipmaps * sizeof(int), kAlignment);
		ddsTextureDetails.pitches = new int[mipmaps];//(GLsizei*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, mipmaps * sizeof(GLsizei), kAlignment);

		assert(sizeof(BYTE) == 1 && "?");

		DWORD mipWidth = header.dwWidth;
		DWORD mipHeight = header.dwHeight;

		for (DWORD mip = 0; mip < mipmaps; mip++)
		{
			DWORD pitchBlocks = (mipWidth + (kColsPerBlock - 1)) / kColsPerBlock;
			DWORD heightBlocks = (mipHeight + (kRowsPerBlock - 1)) / kRowsPerBlock;
			int mipmapSize = pitchBlocks * heightBlocks * kBlockSizeBytes;

			ddsTextureDetails.sizes[mip] = mipmapSize;
			ddsTextureDetails.pitches[mip] = pitchBlocks * kBlockSizeBytes;

			totalSize += mipmapSize;
			mipWidth = dds_imax(1, mipWidth >> 1);
			mipHeight = dds_imax(1, mipHeight >> 1);
		}

		ddsTextureDetails.pixels = imageData;
		ddsTextureDetails.compressed = true;
		ddsTextureDetails.glFormat = flat::TextureFormat_COMPRESSED_RGBA_S3TC_DXT1_EXT;//  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		ddsTextureDetails.d3dFormat = DXGI_FORMAT_BC1_UNORM;
		ddsTextureDetails.totalSize = totalSize;

		return true;
	}


	void CleanupDDSTextureDetails(const DDSTextureDetails& ddsTextureDetails)
	{
		delete[] ddsTextureDetails.pitches;
		delete[] ddsTextureDetails.sizes;
	}
}