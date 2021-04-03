#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::draw::tex
{

	extern s32 WRAP_MODE_CLAMP;
	extern s32 WRAP_MODE_REPEAT;
	extern s32 FILTER_LINEAR;
	extern s32 FILTER_NEAREST;
	extern s32 FILTER_NEAREST_MIP_MAP_LINEAR;
	extern u32 FORMAT_DEPTH;

	struct TextureContainer;

	enum TextureType
	{
		Diffuse = 0,
		Specular,
		Emissive,
		Normal,
		Metallic,
		Height,
		MicroFacet,
		Occlusion,
		Cubemap,
		HDR,
		NUM_TEXTURE_TYPES
	};

	enum CubemapSide
	{
		RIGHT = 0,
		LEFT,
		TOP,
		BOTTOM,
		FRONT,
		BACK,
		NUM_SIDES
	};

	struct Texture
	{
		r2::asset::AssetHandle textureAssetHandle;
		TextureType type = TextureType::Diffuse;

	};

	struct CubemapTexture
	{
		Texture sides[NUM_SIDES];
	};

	struct TextureHandle
	{
		r2::draw::tex::TextureContainer* container = nullptr;
		f32 sliceIndex = -1.0f;
	};

	using GPUHandle = TextureHandle;

	struct TextureAddress
	{
		u64 containerHandle = 0;
		f32 texPage = -1.0f;
	};

	struct TextureFormat
	{
		s32 mipLevels = 0;
		u32 internalformat = 0;
		s32 width = 0;
		s32 height = 0;
		b32 isCubemap = false;
		b32 compressed = false;

		s32 minFilter = FILTER_LINEAR;
		s32 magFilter = FILTER_LINEAR;
		s32 wrapMode = WRAP_MODE_REPEAT;
	};

	TextureHandle UploadToGPU(const r2::asset::AssetHandle& texture, TextureType type, bool generateMipMap);
	TextureHandle UploadToGPU(const CubemapTexture& cubemap);


	void UnloadFromGPU(TextureHandle& texture);
	TextureAddress GetTextureAddress(const TextureHandle& handle);
	bool TextureHandlesEqual(const TextureHandle& h1, const TextureHandle& h2);

	const char* TextureTypeToString(TextureType type);

	TextureHandle CreateTexture(const r2::draw::tex::TextureFormat& format);

	namespace impl
	{
		bool Init(const r2::mem::utils::MemBoundary& boundary, u32 numTextureContainers, u32 numTextureContainersPerFormat, bool useMaxNumLayers = false, s32 numTextureLayers = 1, bool sparse = true);
		void Shutdown();

		
		u64 MemorySize(u32 maxNumTextureContainers, u32 maxTextureContainersPerFormat, u32 maxTextureLayers, u64 alignment, u32 headerSize, u32 boundsChecking);
		u64 GetMaxTextureLayers(bool sparse);
	}
}


#endif // !__TEXTURE_H__
