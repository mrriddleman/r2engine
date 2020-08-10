#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::draw::tex
{
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
		NUM_TEXTURE_TYPES
	};

	struct Texture
	{
		r2::asset::AssetHandle textureAssetHandle;
		TextureType type = TextureType::Diffuse;
	};

	struct TextureHandle
	{
		r2::draw::tex::TextureContainer* container = nullptr;
		f32 sliceIndex = -1.0f;
	};

	using GPUHandle = TextureHandle;

	struct TextureAddress
	{
		u64 containerHandle;
		f32 texPage;
	};

	struct TextureFormat
	{
		s32 mipLevels;
		u32 internalformat;
		s32 width;
		s32 height;
	};

	TextureHandle UploadToGPU(const r2::asset::AssetHandle& texture, bool generateMipMap);
	void UnloadFromGPU(TextureHandle& texture);
	TextureAddress GetTextureAddress(const TextureHandle& handle);
	bool TextureHandlesEqual(const TextureHandle& h1, const TextureHandle& h2);

	const char* TextureTypeToString(TextureType type);

	namespace impl
	{
		bool Init(const r2::mem::utils::MemBoundary& boundary, u32 numTextureContainers, u32 numTextureContainersPerFormat, bool useMaxNumLayers = false, s32 numTextureLayers = 1, bool sparse = true);
		void Shutdown();

		
		u64 MemorySize(u32 maxNumTextureContainers, u32 maxTextureContainersPerFormat, u32 maxTextureLayers, u64 alignment, u32 headerSize, u32 boundsChecking);
		u64 GetMaxTextureLayers(bool sparse);
	}
}


#endif // !__TEXTURE_H__
