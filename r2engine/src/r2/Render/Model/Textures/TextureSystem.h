#ifndef __TEXTURE_SYSTEM_H__
#define __TEXTURE_SYSTEM_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/File/PathUtils.h"
#include "assetlib/TextureMetaData_generated.h"

namespace r2::draw::texsys
{

	struct InitialTextureFormat
	{
		flat::TextureFormat textureFormat;
		u32 numPages;
		u32 width;
		u32 height;
		u32 numMips;
		bool isCubemap;
		bool isAnisotropic;
	};

	//@NOTE: this system holds map of the gpu texture handles to the texture asset
	//		 The texture data should be loaded into memory already
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 maxNumTextures, const r2::SArray<InitialTextureFormat>* formatsToMake, const char* systemName);
	void Shutdown();

	//Maybe we have entries of texture names based on the hash

	//@NOTE: we don't want to return a new opengl texture id because we don't want
	//the client to have a handle to the texture since it can change due to the asset system
	//So we'll need to keep a mapping of that handle with the Texture we pass back from LoadTexture
	void UploadToGPU(const r2::asset::AssetHandle& texture, tex::TextureType type, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter);
	void UploadToGPU(const r2::draw::tex::CubemapTexture& cubemap, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter);
	void ReloadTexture(const r2::asset::AssetHandle& texture);
	void UnloadFromGPU(const r2::asset::AssetHandle& texture);

	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::Texture& texture);
	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::TextureHandle& textureHandle);
	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::CubemapTexture& cubemap);

	u32 GetNumberOfMipMaps(const r2::draw::tex::CubemapTexture& cubemap);
	
	
	u64 MemorySize(u64 maxNumTextures);
}

#endif // __TEXTURE_SYSTEM_H__
