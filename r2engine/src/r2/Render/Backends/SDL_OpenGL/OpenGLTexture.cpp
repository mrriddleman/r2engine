#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLTextureSystem.h"
#include "stb_image.h"
#include "glad/glad.h"
#include "SDL_image.h"
#include "r2/Core/Memory/InternalEngineMemory.h"


#include <errno.h>


#define dds_imax(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) \
                  (((d & 0xFF) << 24) \
                |  ((c & 0xFF) << 16) \
                |  ((b & 0xFF) <<  8) \
                |  ((a & 0xFF) <<  0))
#endif

typedef u8 BYTE;
typedef u32  UINT;
typedef u32 DWORD;
typedef s32 errno_t;

namespace
{
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

		GLsizei mipMapCount = 0;
		const void* pixels = nullptr;
		GLsizei* sizes = nullptr;
		GLsizei* pitches = nullptr;

		GLenum glFormat = 0;
		DWORD d3dFormat = 0;
		b32 compressed = false;
	};

	GLint MipMapHeight(const DDSTextureDetails& ddsTexture, GLint mipMap) { return dds_imax(1, ddsTexture.height >> mipMap); }
	GLint MipMapWidth(const DDSTextureDetails& ddsTexture, GLint mipMap) { return dds_imax(1, ddsTexture.width >> mipMap); }

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


	bool ReadDDSFileData(const void* imageData, u64 size, DDSTextureDetails& ddsTextureDetails);
	bool ReadDDSFromFileDXT1(const DDS_HEADER& header, const void* imageData, u64 size, DDSTextureDetails& ddsTextureDetails);
	void CleanupDDSTextureDetails(const DDSTextureDetails& ddsTextureDetails);
}


namespace r2::draw::tex
{
	
	TextureHandle UploadToGPU(const r2::asset::AssetHandle& texture, bool generateMipMap)
	{
		stbi_set_flip_vertically_on_load(true);

		r2::asset::AssetCache* assetCache = r2::asset::lib::GetAssetCache(texture.assetCache);

		if (!assetCache)
		{
			return r2::draw::tex::GPUHandle{};
		}

		r2::asset::AssetCacheRecord assetCacheRecord = assetCache->GetAssetBuffer(texture);
		//@TODO(Serge): we have the asset type now from the record


		int texWidth;
		int texHeight;
		int channels;
		int mipLevels = 1;
		bool compressed = false;
		u8* imageData = stbi_load_from_memory(
			assetCacheRecord.buffer->Data(),
			static_cast<int>(assetCacheRecord.buffer->Size()), &texWidth, &texHeight, &channels, 0);
		bool usedSTBI = true;
		bool usedTiff = false;
		bool usedDDS = false;

		DDSTextureDetails ddsTextureDetails;
		SDL_Surface* imageSurface = nullptr;

		if (!imageData)
		{
			usedSTBI = false;
			

			bool ddsSuccess = ReadDDSFileData(assetCacheRecord.buffer->Data(), assetCacheRecord.buffer->Size(), ddsTextureDetails);


			if (ddsSuccess)
			{
				texWidth = ddsTextureDetails.width;
				texHeight = ddsTextureDetails.height;
				mipLevels = ddsTextureDetails.mipMapCount;

				usedDDS = true;
				compressed = true;
				channels = ddsTextureDetails.glFormat;
			}
			else
			{

				SDL_RWops* ops = SDL_RWFromConstMem(assetCacheRecord.buffer->Data(), assetCacheRecord.buffer->Size());

				R2_CHECK(ops != nullptr, "We should be able to get the RWOps from memory");

				imageSurface = IMG_LoadTIF_RW(ops);

				if (!imageSurface)
				{
					R2_CHECK(false, "We couldn't read the tiff File!");
					return TextureHandle{};
				}

				usedTiff = true;
				imageData = (u8*)imageSurface->pixels;

				texWidth = imageSurface->w;
				texHeight = imageSurface->h;

				channels = imageSurface->format->BytesPerPixel;

				VerticalFlip(imageData, texWidth, texHeight, channels);
			}
		}
		
		GLenum format;
		GLenum internalFormat;
		if (channels == 1)
		{
			format = GL_RED;
			internalFormat = GL_R8;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			format = GL_RGB;
		}
		else if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			format = GL_RGBA;
		}
		else if(channels == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		{
			internalFormat = channels;
			format = channels;
		}
		else
		{
			R2_CHECK(false, "Unknown image format!");
		}

		r2::draw::tex::GPUHandle newHandle;
		r2::draw::tex::TextureFormat textureFormat;

		textureFormat.internalformat = internalFormat;
		textureFormat.width = texWidth;
		textureFormat.height = texHeight;
		textureFormat.mipLevels = mipLevels;
		textureFormat.compressed = compressed;

		r2::draw::gl::texsys::MakeNewGLTexture(newHandle, textureFormat);

		if (compressed && usedDDS)
		{
			size_t offset = 0;
			for (int mip = 0; mip < mipLevels; ++mip)
			{
				r2::draw::gl::tex::CompressedTexSubImage2D(newHandle, mip, 0, 0, MipMapWidth(ddsTextureDetails, mip), MipMapHeight(ddsTextureDetails, mip), ddsTextureDetails.glFormat, ddsTextureDetails.sizes[mip], r2::mem::utils::PointerAdd(ddsTextureDetails.pixels, offset));
				offset += ddsTextureDetails.sizes[mip];
			}
		}
		else
		{
			r2::draw::gl::tex::TexSubImage2D(newHandle, 0, 0, 0, texWidth, texHeight, format, GL_UNSIGNED_BYTE, imageData);
		}
		

		if (usedSTBI)
		{
			stbi_image_free(imageData);
		}

		if(usedTiff)
		{
			SDL_FreeSurface(imageSurface);
		}

		if (usedDDS)
		{
			CleanupDDSTextureDetails(ddsTextureDetails);
		}

		assetCache->ReturnAssetBuffer(assetCacheRecord);

		return newHandle;
	}

	TextureHandle UploadToGPU(const CubemapTexture& cubemap)
	{
		stbi_set_flip_vertically_on_load(false);

		r2::asset::AssetCache* assetCache = r2::asset::lib::GetAssetCache(cubemap.sides[0].textureAssetHandle.assetCache);

		if (!assetCache)
		{
			return r2::draw::tex::GPUHandle{};
		}

		r2::draw::tex::GPUHandle newHandle;
		r2::draw::tex::TextureFormat textureFormat;
		GLenum format;
		GLenum internalFormat;

		for (u32 i = 0; i < r2::draw::tex::NUM_SIDES; ++i)
		{
			r2::asset::AssetCacheRecord assetCacheRecord = assetCache->GetAssetBuffer(cubemap.sides[CubemapSide::RIGHT + i].textureAssetHandle);
		
			R2_CHECK(assetCacheRecord.type == r2::asset::CUBEMAP_TEXTURE, "This better be a cubemap!");

			int texWidth;
			int texHeight;
			int channels;
			u8* imageData = stbi_load_from_memory(
				assetCacheRecord.buffer->Data(),
				static_cast<int>(assetCacheRecord.buffer->Size()), &texWidth, &texHeight, &channels, 0);

			if (i == 0)
			{
				if (channels == 1)
				{
					format = GL_RED;
					internalFormat = GL_R8;
				}
				else if (channels == 3)
				{
					internalFormat = GL_RGB8;
					format = GL_RGB;
				}
				else if (channels == 4)
				{
					internalFormat = GL_RGBA8;
					format = GL_RGBA;
				}
				else
				{
					R2_CHECK(false, "UNKNOWN image format");
				}

				textureFormat.internalformat = internalFormat;
				textureFormat.width = texWidth;
				textureFormat.height = texHeight;
				textureFormat.mipLevels = 1;
				textureFormat.isCubemap = true;

				r2::draw::gl::texsys::MakeNewGLTexture(newHandle, textureFormat);
			}
			
			r2::draw::gl::tex::TexSubCubemapImage2D(newHandle, static_cast<CubemapSide>(CubemapSide::RIGHT + i), 0, 0, 0, texWidth, texHeight, format, GL_UNSIGNED_BYTE, imageData);


			stbi_image_free(imageData);
			assetCache->ReturnAssetBuffer(assetCacheRecord);
		}


		return newHandle;
	}

	void UnloadFromGPU(TextureHandle& texture)
	{
		r2::draw::gl::texsys::FreeGLTexture(texture);
	}

	TextureAddress GetTextureAddress(const TextureHandle& handle)
	{
		return r2::draw::gl::tex::GetAddress(handle);
	}
}

namespace
{
	bool ReadDDSFileData(const void* imageData, u64 size, DDSTextureDetails& ddsTextureDetails)
	{
		if (!imageData || size == 0)
		{
			R2_CHECK(false, "invalid data or size is 0");
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
				success = ReadDDSFromFileDXT1(fileHeader->header, r2::mem::utils::PointerAdd(imageData, sizeof(DDS_FILEHEADER)), size - sizeof(DDS_FILEHEADER), ddsTextureDetails);
			}
		}

		if (success)
		{
			ddsTextureDetails.height = fileHeader->header.dwHeight;
			ddsTextureDetails.width = fileHeader->header.dwWidth;
		}

		return success;
	}

	bool ReadDDSFromFileDXT1(const DDS_HEADER& header, const void* imageData, u64 size, DDSTextureDetails& ddsTextureDetails)
	{
		const DWORD kBlockSizeBytes = 8;
		const DWORD kColsPerBlock = 4;
		const DWORD kRowsPerBlock = 4;

		size_t totalSize = 0;
		DWORD mipmaps = dds_imax(1, header.dwMipMapCount);
		const u64 kAlignment = 16;

		ddsTextureDetails.mipMapCount = mipmaps;
		ddsTextureDetails.sizes = (GLsizei*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, mipmaps * sizeof(GLsizei), kAlignment);
		ddsTextureDetails.pitches = (GLsizei*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, mipmaps * sizeof(GLsizei), kAlignment);

		R2_CHECK(sizeof(BYTE) == 1, "?");

		DWORD mipWidth = header.dwWidth;
		DWORD mipHeight = header.dwHeight;

		for (DWORD mip = 0; mip < mipmaps; mip++)
		{
			DWORD pitchBlocks = (mipWidth + (kColsPerBlock - 1)) / kColsPerBlock;
			DWORD heightBlocks = (mipHeight + (kRowsPerBlock - 1)) / kRowsPerBlock;
			GLsizei mipmapSize = pitchBlocks * heightBlocks * kBlockSizeBytes;

			ddsTextureDetails.sizes[mip] = mipmapSize;
			ddsTextureDetails.pitches[mip] = pitchBlocks * kBlockSizeBytes;

			totalSize += mipmapSize;
			mipWidth = dds_imax(1, mipWidth >> 1);
			mipHeight = dds_imax(1, mipHeight >> 1);
		}

		ddsTextureDetails.pixels = imageData;
		ddsTextureDetails.compressed = true;
		ddsTextureDetails.glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		ddsTextureDetails.d3dFormat = DXGI_FORMAT_BC1_UNORM;

		return true;
	}


	void CleanupDDSTextureDetails(const DDSTextureDetails& ddsTextureDetails)
	{
		FREE(ddsTextureDetails.pitches, *MEM_ENG_SCRATCH_PTR);
		FREE(ddsTextureDetails.sizes, *MEM_ENG_SCRATCH_PTR);
	}
}

#endif