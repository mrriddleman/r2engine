#ifndef __RENDER_MATERIAL_CACHE_H__
#define __RENDER_MATERIAL_CACHE_H__

#include "r2/Render/Model/RenderMaterials/RenderMaterial.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace flat
{
	struct MaterialParams;
}

namespace r2::draw
{
	struct RenderMaterialCache
	{
		u32 mName = 0;
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		r2::mem::StackArena* mArena = nullptr;

		r2::SHashMap<s32>* mGPURenderMaterialIndices = nullptr;//materialName (u64) -> metadata
		
		r2::SQueue<s32>* mFreeIndices = nullptr;
		
		r2::mem::PoolArena* mGPURenderMaterialArena = nullptr;
		r2::SArray<RenderMaterialParams*>* mGPURenderMaterialArray = nullptr;
		
		r2::mem::FreeListArena* mAssetHandleArena = nullptr;
		r2::SHashMap<r2::SArray<r2::asset::AssetHandle>*>* mUploadedTextureForMaterialMap = nullptr;

		r2::draw::tex::Texture mMissingTexture;

		static u64 MemorySize(u32 numMaterials);
	};
}

namespace r2::draw::rmat
{
	

	RenderMaterialCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numMaterials, const char* areaName);
	void Shutdown(RenderMaterialCache* cache);

	bool UploadMaterialTextureParams(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture);
	bool UploadMaterialTextureParamsArray(RenderMaterialCache& renderMaterialCache, const flat::MaterialParamsPack* materialParamsPack, const r2::SArray<tex::Texture>* textures, const r2::SArray<tex::CubemapTexture>* cubemapTextures);

	bool UnloadMaterialParams(RenderMaterialCache& renderMaterialCache, u64 materialName);
	bool UnloadMaterialParamsArray(RenderMaterialCache& renderMaterialCache, const r2::SArray<u64>& materialNames);

	bool IsMaterialLoadedOnGPU(const RenderMaterialCache& renderMaterialCache, u64 materialName);

	const RenderMaterialParams* GetGPURenderMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName);

	bool GetGPURenderMaterials(RenderMaterialCache& renderMaterialCache, const r2::SArray<u64>* handles, r2::SArray<RenderMaterialParams>* gpuRenderMaterials);
}

#endif
