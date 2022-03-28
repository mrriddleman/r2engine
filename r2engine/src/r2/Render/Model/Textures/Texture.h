#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "r2/Core/Assets/AssetTypes.h"
#include <glm/vec4.hpp>

namespace r2::draw::tex
{

	extern s32 WRAP_MODE_CLAMP_TO_EDGE;
	extern s32 WRAP_MODE_CLAMP_TO_BORDER;
	extern s32 WRAP_MODE_REPEAT;
	extern s32 WRAP_MODE_MIRRORED_REPEAT;
	extern s32 FILTER_LINEAR;
	extern s32 FILTER_NEAREST;
	extern s32 FILTER_NEAREST_MIP_MAP_LINEAR;
	extern u32 DEPTH_COMPONENT;

	extern s32 FILTER_NEAREST_MIPMAP_NEAREST;
	extern s32 FILTER_LINEAR_MIPMAP_NEAREST;
	extern s32 FILTER_LINEAR_MIPMAP_LINEAR;

	struct TextureContainer;

	const u32 MAX_MIP_LEVELS = 10;

	enum TextureType
	{
		Diffuse = 0,
		Emissive,
		Normal,
		Metallic,
		Height,
		Roughness,
		Occlusion,
		Anisotropy,
		Detail,
		ClearCoat,
		ClearCoatRoughness,
		ClearCoatNormal,
		Cubemap,
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

	struct MipLevel
	{
		u32 mipLevel;
		Texture sides[NUM_SIDES];
	};

	struct CubemapTexture
	{
		MipLevel mips[MAX_MIP_LEVELS];
		u32 numMipLevels = 0;
	};

	struct TextureHandle
	{
		r2::draw::tex::TextureContainer* container = nullptr;
		f32 sliceIndex = -1.0f;
		u32 numPages = 0;
	};

	using GPUHandle = TextureHandle;

	struct TextureAddress
	{
		u64 containerHandle = 0;
		f32 texPage = -1.0f;
		s32 channel = -1; //maybe -1 is full, then R = 0, G = 1, B = 2, A = 3 - set by the material, here for packing reasons
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
		glm::vec4 borderColor = glm::vec4(0);
		b32 isAnisotropic = false;
		float anisotropy = 0.0;
		//u32 numCommitLayers = 1;
	};

	struct TextureContainer
	{
		u64 handle;
		u32 texId;
		r2::SQueue<s32>* freeSpace;

		r2::draw::tex::TextureFormat format;
		u32 numSlices;
		s32 xTileSize;
		s32 yTileSize;
		b32 isSparse;
	};

	TextureHandle UploadToGPU(const r2::asset::AssetHandle& texture, TextureType type, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter);
	TextureHandle UploadToGPU(const CubemapTexture& cubemap, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter);


	void UnloadFromGPU(TextureHandle& texture);
	TextureAddress GetTextureAddress(const TextureHandle& handle);
	bool TextureHandlesEqual(const TextureHandle& h1, const TextureHandle& h2);
	bool TexturesEqual(const Texture& t1, const Texture& t2);
	bool TexturesEqualExcludeType(const Texture& t1, const Texture& t2);

	TextureHandle CreateTexture(const r2::draw::tex::TextureFormat& format, u32 numPages);
	TextureHandle AddTexturePages(const TextureHandle& textureHandle, u32 numPages);
	

	r2::asset::AssetHandle GetCubemapAssetHandle(const CubemapTexture& cubemap);

	bool IsInvalidTextureAddress(const TextureAddress& texAddress);
	bool AreTextureAddressEqual(const TextureAddress& ta1, const TextureAddress& ta2);

	const s32 GetMaxTextureSize();

	namespace impl
	{
		bool Init(const r2::mem::utils::MemBoundary& boundary, u32 numTextureContainers, u32 numTextureContainersPerFormat, bool useMaxNumLayers = false, s32 numTextureLayers = 1, bool sparse = true);
		void Shutdown();

		u32 GetNumberOfMipMaps(const TextureHandle& texture);
		
		u64 MemorySize(u32 maxNumTextureContainers, u32 maxTextureContainersPerFormat, u32 maxTextureLayers, u64 alignment, u32 headerSize, u32 boundsChecking);
		u64 GetMaxTextureLayers(bool sparse);
	}
}


#endif // !__TEXTURE_H__
