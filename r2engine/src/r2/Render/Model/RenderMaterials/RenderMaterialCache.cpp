#include "r2pch.h"

#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"
#include "r2/Render/Model/MaterialParams_generated.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Utils/Hash.h"

namespace r2::draw
{
	u64 RenderMaterialCache::MemorySize(u32 numMaterials)
	{
		constexpr u32 ALIGNMENT = 16;
		u64 memorySize = 0;
		const u32 freelistHeaderSize = r2::mem::FreeListAllocator::HeaderSize();
		const u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		const u32 poolHeaderSize = r2::mem::PoolAllocator::HeaderSize();

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderMaterialCache), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::FreeListArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(numMaterials * r2::SHashMap<s32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<s32>::MemorySize(numMaterials), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderMaterialParams*>::MemorySize(numMaterials), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::SArray<r2::asset::AssetHandle>*>::MemorySize(memorySize) * r2::SHashMap<r2::SArray<r2::asset::AssetHandle>*>::LoadFactorMultiplier(), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderMaterialParams), ALIGNMENT, poolHeaderSize, boundsChecking) * numMaterials;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetHandle>::MemorySize(tex::NUM_TEXTURE_TYPES), ALIGNMENT, freelistHeaderSize, boundsChecking) * numMaterials;

		return memorySize;
	}
}

namespace r2::draw::rmat
{
	const u64 EMPTY_TEXTURE = STRING_ID("");
	constexpr u32 ALIGNMENT = 16;

	void UpdateGPURenderMaterialForMaterialParams(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture);
	RenderMaterialParams* AddNewGPURenderMaterial(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams);
	void RemoveGPURenderMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName);
	tex::TextureType GetTextureTypeForPropertyType(flat::MaterialPropertyType propertyType);

	RenderMaterialCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numMaterials, const char* areaName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = RenderMaterialCache::MemorySize(numMaterials);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes, difference: %llu",
				memoryArea->UnAllocatedSpace(), subAreaSize, subAreaSize - memoryArea->UnAllocatedSpace());
			return nullptr;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
			return nullptr;
		}

		r2::mem::StackArena* renderMaterialCacheArena = EMPLACE_STACK_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(renderMaterialCacheArena != nullptr, "We couldn't emplace the stack arena - no way to recover!");

		RenderMaterialCache* newRenderMaterialCache = ALLOC(RenderMaterialCache, *renderMaterialCacheArena);

		R2_CHECK(newRenderMaterialCache != nullptr, "We couldn't create the RenderMaterialCache object");

		newRenderMaterialCache->mMemoryAreaHandle = memoryAreaHandle;
		newRenderMaterialCache->mSubAreaHandle = subAreaHandle;
		newRenderMaterialCache->mArena = renderMaterialCacheArena;

		newRenderMaterialCache->mGPURenderMaterialIndices = MAKE_SHASHMAP(*renderMaterialCacheArena, s32, numMaterials * r2::SHashMap<s32>::LoadFactorMultiplier());

		R2_CHECK(newRenderMaterialCache->mGPURenderMaterialIndices != nullptr, "Couldn't create the mGPURenderMaterialIndices hashmap");

		newRenderMaterialCache->mFreeIndices = MAKE_SQUEUE(*renderMaterialCacheArena, s32, numMaterials);

		R2_CHECK(newRenderMaterialCache->mFreeIndices != nullptr, "Couldn't create the mFreeIndices queue");

		for (s32 i = 0; i < numMaterials; ++i)
		{
			r2::squeue::PushBack(*newRenderMaterialCache->mFreeIndices, i);
		}

		newRenderMaterialCache->mGPURenderMaterialArena = MAKE_POOL_ARENA(*renderMaterialCacheArena, sizeof(RenderMaterialParams), ALIGNMENT, numMaterials);

		R2_CHECK(newRenderMaterialCache->mGPURenderMaterialArena != nullptr, "Couldn't create the mGPURenderMaterialArena");

		newRenderMaterialCache->mGPURenderMaterialArray = MAKE_SARRAY(*renderMaterialCacheArena, RenderMaterialParams*, numMaterials);

		R2_CHECK(newRenderMaterialCache->mGPURenderMaterialArray != nullptr, "Couldn't create the mGPURenderMaterialArray");

		RenderMaterialParams* nullRenderMaterial = nullptr;

		r2::sarr::Fill(*newRenderMaterialCache->mGPURenderMaterialArray, nullRenderMaterial);

		newRenderMaterialCache->mAssetHandleArena = MAKE_FREELIST_ARENA(*renderMaterialCacheArena, r2::SArray<r2::asset::AssetHandle>::MemorySize(tex::NUM_TEXTURE_TYPES) * numMaterials, r2::mem::FIND_BEST);

		R2_CHECK(newRenderMaterialCache->mAssetHandleArena != nullptr, "Couldn't create the mAssetHandleArena");

		newRenderMaterialCache->mUploadedTextureForMaterialMap = MAKE_SHASHMAP(*renderMaterialCacheArena, r2::SArray<r2::asset::AssetHandle>*, numMaterials * r2::SHashMap<s32>::LoadFactorMultiplier());

		R2_CHECK(newRenderMaterialCache->mUploadedTextureForMaterialMap != nullptr, "Couldn't create the mUploadedTextureForMaterialMap");

		return newRenderMaterialCache;
	}

	void Shutdown(RenderMaterialCache* cache)
	{
		if (!cache)
		{
			R2_CHECK(false, "Probably a bug");
			return;
		}

		r2::mem::StackArena* arena = cache->mArena;

		auto iter = r2::shashmap::Begin(*cache->mGPURenderMaterialIndices);
		for (; iter != r2::shashmap::End(*cache->mGPURenderMaterialIndices); ++iter)
		{
			UnloadMaterialParams(*cache, iter->key);
		}

		FREE(cache->mUploadedTextureForMaterialMap, *arena);

		FREE(cache->mAssetHandleArena, *arena);

		FREE(cache->mGPURenderMaterialArray, *arena);

		FREE(cache->mGPURenderMaterialArena, *arena);

		FREE(cache->mFreeIndices, *arena);

		FREE(cache->mGPURenderMaterialIndices, *arena);

		FREE(cache, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	const tex::Texture* FindTextureForTextureName(const r2::SArray<tex::Texture>* textures, u64 textureName, flat::MaterialPropertyType propertyType)
	{
		const u32 numTextures = r2::sarr::Size(*textures);

		for (u32 i = 0; i < numTextures; ++i)
		{
			const tex::Texture& nextTexture = r2::sarr::At(*textures, i);

			tex::TextureType textureType = GetTextureTypeForPropertyType(propertyType);
			if (nextTexture.textureAssetHandle.handle == textureName && nextTexture.type == textureType)
			{
				return &nextTexture;
			}
		}

		return nullptr;
	}

	bool IsTextureParamCubemapTexture(const tex::CubemapTexture* cubemapTexture, u64 textureName, flat::MaterialPropertyType propertyType)
	{
		return tex::GetCubemapAssetHandle(*cubemapTexture).handle == textureName && GetTextureTypeForPropertyType(propertyType) == tex::Diffuse;
	}

	s32 GetWrapMode(const flat::MaterialTextureParam* texParam)
	{
		R2_CHECK(texParam != nullptr, "We should have a proper texParam!");
		switch (texParam->wrapS())
		{
		case flat::TextureWrapMode_REPEAT:
			return tex::WRAP_MODE_REPEAT;
		case flat::TextureWrapMode_CLAMP_TO_BORDER:
			return tex::WRAP_MODE_CLAMP_TO_BORDER;
		case flat::TextureWrapMode_CLAMP_TO_EDGE:
			return tex::WRAP_MODE_CLAMP_TO_EDGE;
		case flat::TextureWrapMode_MIRRORED_REPEAT:
			return tex::WRAP_MODE_MIRRORED_REPEAT;
		default:
			R2_CHECK(false, "Unsupported wrap mode!");
			break;
		}
		return 0;
	}

	s32 GetMinFilter(const flat::MaterialTextureParam* texParam)
	{
		R2_CHECK(texParam != nullptr, "We should have a proper texParam!");
		switch (texParam->minFilter())
		{
		case flat::MinTextureFilter_LINEAR:
			return tex::FILTER_LINEAR;
		case flat::MinTextureFilter_NEAREST:
			return tex::FILTER_NEAREST;
		case flat::MinTextureFilter_LINEAR_MIPMAP_LINEAR:
			return tex::FILTER_LINEAR_MIPMAP_LINEAR;
		case flat::MinTextureFilter_LINEAR_MIPMAP_NEAREST:
			return tex::FILTER_LINEAR_MIPMAP_NEAREST;
		case flat::MinTextureFilter_NEAREST_MIPMAP_LINEAR:
			return tex::FILTER_NEAREST_MIP_MAP_LINEAR;
		case flat::MinTextureFilter_NEAREST_MIPMAP_NEAREST:
			return tex::FILTER_NEAREST_MIPMAP_NEAREST;
		default:
			R2_CHECK(false, "Unsupported Min Filter!");
			break;
		}
		return 0;
	}

	s32 GetMagFilter(const flat::MaterialTextureParam* texParam)
	{
		R2_CHECK(texParam != nullptr, "We should have a proper texParam!");
		if (texParam->magFilter() == flat::MagTextureFilter_LINEAR)
		{
			return tex::FILTER_LINEAR;
		}
		else if (texParam->magFilter() == flat::MagTextureFilter_NEAREST)
		{
			return tex::FILTER_NEAREST;
		}

		R2_CHECK(false, "Unsupported Mag filter!");
		return 0;
	}

	bool UploadMaterialTextureParams(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture)
	{
		if (!materialParams)
		{
			R2_CHECK(false, "materialParams is nullptr");
			return false;
		}

		if (!textures && !cubemapTexture)
		{
			R2_CHECK(false, "There's nothing to upload here");
			return false;
		}

		const auto numTextureParams = materialParams->textureParams()->size();
		bool isCubemap = cubemapTexture != nullptr;

		r2::SArray<tex::Texture>* materialTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, tex::Texture, numTextureParams);


		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const flat::MaterialTextureParam* textureParam = materialParams->textureParams()->Get(i);

			u64 textureHandle = textureParam->value();

			if (textureHandle == EMPTY_TEXTURE)
			{
				continue;
			}

			flat::MaterialPropertyType propertyType = textureParam->propertyType();

			if (!isCubemap)
			{
				//find the texture that matches textureHandle
				const tex::Texture* theTexture = FindTextureForTextureName(textures, textureHandle, propertyType);

				R2_CHECK(theTexture != nullptr, "We should always have the texture here if we don't have an empty textureHandle!");

				r2::sarr::Push(*materialTextures, *theTexture);

				r2::draw::texsys::UploadToGPU(theTexture->textureAssetHandle, theTexture->type, textureParam->anisotropicFiltering(), GetWrapMode(textureParam), GetMinFilter(textureParam), GetMagFilter(textureParam));
			}
			else
			{
				if (IsTextureParamCubemapTexture(cubemapTexture, textureHandle, propertyType))
				{
					r2::draw::texsys::UploadToGPU(*cubemapTexture, textureParam->anisotropicFiltering(), GetWrapMode(textureParam), GetMinFilter(textureParam), GetMagFilter(textureParam));
				}
			}
		}

		//Now update the GPURenderMaterials
		UpdateGPURenderMaterialForMaterialParams(renderMaterialCache, materialParams, materialTextures, cubemapTexture);

		FREE(materialTextures, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	//bool UploadMaterialTextureParamsArray(RenderMaterialCache& renderMaterialCache, const r2::SArray<const flat::MaterialParams*>* materialParamsArray, const r2::SArray<tex::Texture>* textures, const r2::SArray<const tex::CubemapTexture*>* cubemapTextures)
	//{
	//	return false;
	//}

	bool UnloadMaterialParams(RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		r2::SArray<r2::asset::AssetHandle>* defaultAssetHandles = nullptr;

		r2::SArray<r2::asset::AssetHandle>* assetHandles = r2::shashmap::Get(*renderMaterialCache.mUploadedTextureForMaterialMap, materialName, defaultAssetHandles);

		if (assetHandles == defaultAssetHandles)
		{
			//we don't have anything to unload
			return true;
		}

		const u32 numTexturesToUnload = r2::sarr::Size(*assetHandles);

		for (u32 i = 0; i < numTexturesToUnload; ++i)
		{
			texsys::UnloadFromGPU(r2::sarr::At(*assetHandles, i));
		}

		RemoveGPURenderMaterial(renderMaterialCache, materialName);

		FREE(assetHandles, *renderMaterialCache.mAssetHandleArena);

		r2::shashmap::Remove(*renderMaterialCache.mUploadedTextureForMaterialMap, materialName);

		return true;
	}

	bool UnloadMaterialParamsArray(RenderMaterialCache& renderMaterialCache, const r2::SArray<u64>& materialNames)
	{
		const u32 numMaterialsToUnload = r2::sarr::Size(materialNames);

		for (u32 i = 0; i < numMaterialsToUnload; ++i)
		{
			UnloadMaterialParams(renderMaterialCache, r2::sarr::At(materialNames, i));
		}

		return true;
	}

	bool IsMaterialLoadedOnGPU(const RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		return r2::shashmap::Has(*renderMaterialCache.mGPURenderMaterialIndices, materialName);
	}

	GPURenderMaterialHandle GetGPURenderMaterialHandle(const RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		s32 defaultIndex = -1;

		s32 index = r2::shashmap::Get(*renderMaterialCache.mGPURenderMaterialIndices, materialName, defaultIndex);

		if (index == defaultIndex)
		{
			return InvalideGPURenderMaterialHandle;
		}

		GPURenderMaterialHandle newHandle;
		newHandle.index = index;
		newHandle.materialName = materialName;
		newHandle.renderMaterialCacheName = renderMaterialCache.mName;

		return newHandle;
	}

	const RenderMaterialParams* GetGPURenderMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		s32 defaultIndex = -1;

		s32 index = r2::shashmap::Get(*renderMaterialCache.mGPURenderMaterialIndices, materialName, defaultIndex);

		if (index == defaultIndex)
		{
			return nullptr;
		}

		return r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index);
	}

	const RenderMaterialParams* GetGPURenderMaterial(RenderMaterialCache& renderMaterialCache, const GPURenderMaterialHandle& handle)
	{
		if (IsGPURenderMaterialHandleInvalid(handle))
		{
			return nullptr;
		}

		return r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, handle.index);
	}

	bool GetGPURenderMaterials(RenderMaterialCache& renderMaterialCache, const r2::SArray<GPURenderMaterialHandle>* handles, r2::SArray<RenderMaterialParams>* gpuRenderMaterials)
	{
		if (handles == nullptr)
		{
			R2_CHECK(false, "handles are nullptr");
			return false;
		}

		if (gpuRenderMaterials == nullptr)
		{
			R2_CHECK(false, "gpuRenderMaterials are nullptr");
			return false;
		}

		if (r2::sarr::RoomLeft(*gpuRenderMaterials) < r2::sarr::Size(*handles))
		{
			R2_CHECK(false, "We don't have enough space to fit the GPURenderMaterials");
			return false;
		}

		const u32 numHandles = r2::sarr::Size(*handles);

		for (u32 i = 0; i < numHandles; ++i)
		{
			const GPURenderMaterialHandle& gpuRenderMaterialHandle = r2::sarr::At(*handles, i);

			R2_CHECK(gpuRenderMaterialHandle.renderMaterialCacheName == renderMaterialCache.mName, "These always need to match");

			const RenderMaterialParams* renderMaterialPtr = r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, gpuRenderMaterialHandle.index);

			R2_CHECK(renderMaterialPtr != nullptr, "Should never be nullptr");

			r2::sarr::Push(*gpuRenderMaterials, *renderMaterialPtr);
		}

		return true;
	}

	bool IsGPURenderMaterialHandleInvalid(const GPURenderMaterialHandle& handle)
	{
		return handle.index == InvalideGPURenderMaterialHandle.index || handle.materialName == 0 || handle.renderMaterialCacheName == 0;
	}

	bool AreGPURenderMaterialHandlesEqual(const GPURenderMaterialHandle& handle1, const GPURenderMaterialHandle& handle2)
	{
		return handle1.index == handle2.index && handle1.materialName == handle2.materialName && handle1.renderMaterialCacheName == handle2.renderMaterialCacheName;
	}

	s32 GetPackingType(const flat::MaterialParams* materialParams, tex::TextureType textureType)
	{
		R2_CHECK(materialParams != nullptr, "This shouldn't be null!");

		for (flatbuffers::uoffset_t i = 0; i < materialParams->textureParams()->size(); ++i)
		{
			const flat::MaterialTextureParam* texParam = materialParams->textureParams()->Get(i);

			if (textureType == tex::TextureType::Roughness &&
				texParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_ROUGHNESS)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Metallic && texParam->propertyType() == flat::MaterialPropertyType_METALLIC)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Diffuse && texParam->propertyType() == flat::MaterialPropertyType_ALBEDO)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Occlusion && texParam->propertyType() == flat::MaterialPropertyType_AMBIENT_OCCLUSION)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::ClearCoat && texParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::ClearCoatRoughness && texParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::ClearCoatNormal && texParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT_NORMAL)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Anisotropy && texParam->propertyType() == flat::MaterialPropertyType_ANISOTROPY)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Height && texParam->propertyType() == flat::MaterialPropertyType_HEIGHT)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Emissive && texParam->propertyType() == flat::MaterialPropertyType_EMISSION)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Normal && texParam->propertyType() == flat::MaterialPropertyType_NORMAL)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}

			if (textureType == tex::TextureType::Detail && texParam->propertyType() == flat::MaterialPropertyType_DETAIL)
			{
				return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			}
		}

		return 0;
	}

	void UpdateGPURenderMaterialForMaterialParams(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture)
	{
		R2_CHECK(materialParams != nullptr, "This should not be nullptr");

		s32 defaultIndex;

		s32 index = r2::shashmap::Get(*renderMaterialCache.mGPURenderMaterialIndices, materialParams->name(), defaultIndex);

		RenderMaterialParams* gpuRenderMaterial = nullptr;

		if (index == defaultIndex)
		{
			//make a new one
			gpuRenderMaterial = AddNewGPURenderMaterial(renderMaterialCache, materialParams);
		}
		else
		{
			gpuRenderMaterial = r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index);
		}

		R2_CHECK(gpuRenderMaterial != nullptr, "Should never happen");

		for (flatbuffers::uoffset_t i = 0; i < materialParams->colorParams()->size(); ++i)
		{
			const flat::MaterialColorParam* colorParam = materialParams->colorParams()->Get(i);
			if (colorParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_ALBEDO)
			{
				gpuRenderMaterial->albedo.color = glm::vec4(colorParam->value()->r(), colorParam->value()->g(), colorParam->value()->b(), colorParam->value()->a());
			}
		}

		for (flatbuffers::uoffset_t i = 0; i < materialParams->floatParams()->size(); ++i)
		{
			const flat::MaterialFloatParam* floatParam = materialParams->floatParams()->Get(i);

			if (floatParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_ROUGHNESS)
			{
				gpuRenderMaterial->roughness.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_METALLIC)
			{
				gpuRenderMaterial->metallic.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_REFLECTANCE)
			{
				gpuRenderMaterial->reflectance = floatParam->value();
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_AMBIENT_OCCLUSION)
			{
				gpuRenderMaterial->ao.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT)
			{
				gpuRenderMaterial->clearCoat.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS)
			{
				gpuRenderMaterial->clearCoatRoughness.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_ANISOTROPY)
			{
				gpuRenderMaterial->anisotropy.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::MaterialPropertyType_HEIGHT_SCALE)
			{
				gpuRenderMaterial->heightScale = floatParam->value();
			}
		}

		for (flatbuffers::uoffset_t i = 0; i < materialParams->boolParams()->size(); ++i)
		{
			const flat::MaterialBoolParam* boolParam = materialParams->boolParams()->Get(i);

			if (boolParam->propertyType() == flat::MaterialPropertyType_DOUBLE_SIDED)
			{
				gpuRenderMaterial->doubleSided = boolParam->value();
			}
		}

		if (!cubemapTexture)
		{
			for (flatbuffers::uoffset_t i = 0; i < materialParams->textureParams()->size(); ++i)
			{
				const auto* textureParam = materialParams->textureParams()->Get(i);
				u64 textureHandle = textureParam->value();

				if (textureHandle == EMPTY_TEXTURE)
				{
					continue;
				}

				const tex::Texture* theTexture = FindTextureForTextureName(textures, textureHandle, textureParam->propertyType());

				tex::TextureAddress address = texsys::GetTextureAddress(*theTexture);
			 	s32 channel = GetPackingType(materialParams, theTexture->type);

				switch (theTexture->type)
				{
				case tex::Diffuse:
					gpuRenderMaterial->albedo.texture = address;
					gpuRenderMaterial->albedo.texture.channel = channel;
					break;
				case tex::Metallic:
					gpuRenderMaterial->metallic.texture = address;
					gpuRenderMaterial->metallic.texture.channel = channel;
					break;
				case tex::Normal:
					gpuRenderMaterial->normalMap.texture = address;
					gpuRenderMaterial->normalMap.texture.channel = channel;
					break;
				case tex::Emissive:
					gpuRenderMaterial->emission.texture = address;
					gpuRenderMaterial->emission.texture.channel = channel;
					break;
				case tex::Roughness:
					gpuRenderMaterial->roughness.texture = address;
					gpuRenderMaterial->roughness.texture.channel = channel;
					break;
				case tex::Anisotropy:
					gpuRenderMaterial->anisotropy.texture = address;
					gpuRenderMaterial->anisotropy.texture.channel = channel;
					break;
				case tex::Height:
					gpuRenderMaterial->height.texture = address;
					gpuRenderMaterial->height.texture.channel = channel;
					break;
				case tex::ClearCoat:
					gpuRenderMaterial->clearCoat.texture = address;
					gpuRenderMaterial->clearCoat.texture.channel = channel;
					break;
				case tex::ClearCoatNormal:
					gpuRenderMaterial->clearCoatNormal.texture = address;
					gpuRenderMaterial->clearCoatNormal.texture.channel = channel;
					break;
				case tex::ClearCoatRoughness:
					gpuRenderMaterial->clearCoatRoughness.texture = address;
					gpuRenderMaterial->clearCoatRoughness.texture.channel = channel;
					break;
				case tex::Detail:
					gpuRenderMaterial->detail.texture = address;
					gpuRenderMaterial->detail.texture.channel = channel;
					break;
				case tex::Occlusion:
					gpuRenderMaterial->ao.texture = address;
					gpuRenderMaterial->ao.texture.channel = channel;
					break;
				default:
					R2_CHECK(false, "Unsupported texture type");
					break;
				} 
			}

			r2::SArray<r2::asset::AssetHandle>* textureAssetHandles = nullptr;

			if (!r2::shashmap::Has(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name()))
			{
				textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, r2::sarr::Size(*textures));

				r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name(), textureAssetHandles);
			}
			else
			{
				r2::SArray<r2::asset::AssetHandle>* defaultTextureAssetHandles = nullptr;
				
				textureAssetHandles = r2::shashmap::Get(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name(), defaultTextureAssetHandles);
			
				if (r2::sarr::Capacity(*textureAssetHandles) < r2::sarr::Size(*textures))
				{
					FREE(textureAssetHandles, *renderMaterialCache.mAssetHandleArena);

					r2::shashmap::Remove(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name());

					textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, r2::sarr::Size(*textures));

					r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name(), textureAssetHandles);
				}
			}

			r2::sarr::Clear(*textureAssetHandles);

			for (u32 i = 0; i < r2::sarr::Size(*textures); ++i)
			{
				r2::sarr::Push(*textureAssetHandles, r2::sarr::At(*textures, i).textureAssetHandle);
			}
		}
		else
		{
			gpuRenderMaterial->albedo.texture = texsys::GetTextureAddress(*cubemapTexture);

			const u32 numAssetHandles = cubemapTexture->numMipLevels * tex::CubemapSide::NUM_SIDES;
			
			r2::SArray<r2::asset::AssetHandle>* textureAssetHandles = nullptr;

			if (!r2::shashmap::Has(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name()))
			{
				textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, numAssetHandles);

				r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name(), textureAssetHandles);
			}
			else
			{
				r2::SArray<r2::asset::AssetHandle>* defaultTextureAssetHandles = nullptr;

				textureAssetHandles = r2::shashmap::Get(*renderMaterialCache.mUploadedTextureForMaterialMap, materialParams->name(), defaultTextureAssetHandles);

				R2_CHECK(r2::sarr::Capacity(*textureAssetHandles) >= numAssetHandles, "Since this is a cubemap this must be true");
			}

			r2::sarr::Clear(*textureAssetHandles);

			for (u32 m = 0; m < cubemapTexture->numMipLevels; ++m)
			{
				for (u32 s = 0; s < tex::NUM_SIDES; ++s)
				{
					r2::sarr::Push(*textureAssetHandles, cubemapTexture->mips[m].sides[s].textureAssetHandle);
				}
			}
		}
	}

	RenderMaterialParams* AddNewGPURenderMaterial(RenderMaterialCache& renderMaterialCache, const flat::MaterialParams* materialParams)
	{
		R2_CHECK(materialParams != nullptr, "Should never be the case");

		u64 materialName = materialParams->name();

		R2_CHECK(!r2::shashmap::Has(*renderMaterialCache.mGPURenderMaterialIndices, materialName), "Shouldn't have this already");

		RenderMaterialParams* newGPURenderMaterial = ALLOC(RenderMaterialParams, *renderMaterialCache.mGPURenderMaterialArena);

		R2_CHECK(newGPURenderMaterial != nullptr, "Should never happen");

		//now find where we should add it
		s32 index = r2::squeue::First(*renderMaterialCache.mFreeIndices);

		r2::shashmap::Set(*renderMaterialCache.mGPURenderMaterialIndices, materialName, index);

		r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index) = newGPURenderMaterial;

		r2::squeue::PopFront(*renderMaterialCache.mFreeIndices);

		return newGPURenderMaterial;
	}

	void RemoveGPURenderMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		s32 defaultIndex = -1;

		s32 index = r2::shashmap::Get(*renderMaterialCache.mGPURenderMaterialIndices, materialName, defaultIndex);

		if (index == defaultIndex)
		{
			return;//?
		}

		RenderMaterialParams* gpuRenderMaterial = r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index);

		FREE(gpuRenderMaterial, *renderMaterialCache.mGPURenderMaterialArena);

		r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index) = nullptr;
		
		r2::squeue::PushBack(*renderMaterialCache.mFreeIndices, index);

		r2::shashmap::Remove(*renderMaterialCache.mGPURenderMaterialIndices, materialName);
	}

	tex::TextureType GetTextureTypeForPropertyType(flat::MaterialPropertyType propertyType)
	{
		switch (propertyType)
		{
		case flat::MaterialPropertyType_ALBEDO:
			return tex::TextureType::Diffuse;
		case flat::MaterialPropertyType_AMBIENT_OCCLUSION:
			return tex::TextureType::Occlusion;
		case  flat::MaterialPropertyType_ANISOTROPY:
			return tex::TextureType::Anisotropy;
		case flat::MaterialPropertyType_CLEAR_COAT:
			return tex::TextureType::ClearCoat;
		case flat::MaterialPropertyType_CLEAR_COAT_NORMAL:
			return tex::TextureType::ClearCoatNormal;
		case flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS:
			return tex::TextureType::ClearCoatRoughness;
		case flat::MaterialPropertyType_DETAIL:
			return tex::TextureType::Detail;
		case flat::MaterialPropertyType_EMISSION:
			return tex::TextureType::Emissive;
		case flat::MaterialPropertyType::MaterialPropertyType_HEIGHT:
			return tex::TextureType::Height;
		case flat::MaterialPropertyType_METALLIC:
			return tex::TextureType::Metallic;
		case flat::MaterialPropertyType_NORMAL:
			return tex::TextureType::Normal;
		case flat::MaterialPropertyType_ROUGHNESS:
			return tex::TextureType::Roughness;

		default:
			R2_CHECK(false, "Unsupported MaterialPropertyType");	
		}
		
		return tex::TextureType::Diffuse;

	}
}