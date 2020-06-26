#ifndef __TEXTURE_SYSTEM_H__
#define __TEXTURE_SYSTEM_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::draw::texsys
{
	//@NOTE: this system holds map of the gpu texture handles to the texture asset
	//		 The texture data should be loaded into memory already
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 maxNumTextures, const char* systemName);
	void Shutdown();

	//Maybe we have entries of texture names based on the hash

	//@NOTE: we don't want to return a new opengl texture id for example because we don't want
	//the client to have a handle to the texture since it can change due to the asset system
	//So we'll need to keep a mapping of that handle with the Texture we pass back from LoadTexture
	void UploadToGPU(const r2::asset::AssetHandle& texture);
	void ReloadTexture(const r2::asset::AssetHandle& texture);
	void UnloadFromGPU(const r2::asset::AssetHandle& texture);
	r2::draw::tex::GPUHandle GetGPUHandle(const r2::asset::AssetHandle& texture);
	u64 MemorySize(u64 maxNumTextures);
}

#endif // __TEXTURE_SYSTEM_H__
