#include "r2pch.h"

#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
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

	void UpdateGPURenderMaterialForMaterial(RenderMaterialCache& renderMaterialCache, const flat::Material* material, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture);
	RenderMaterialParams* AddNewGPURenderMaterial(RenderMaterialCache& renderMaterialCache, const flat::Material* material);
	void RemoveGPURenderMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName);
	//tex::TextureType GetTextureTypeForPropertyType(flat::ShaderPropertyType propertyType);

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

		newRenderMaterialCache->mName = r2::utils::HashBytes32(areaName, strlen(areaName));

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
			u64 materialName = iter->key;

			r2::SArray<r2::asset::AssetHandle>* defaultAssetHandles = nullptr;

			r2::SArray<r2::asset::AssetHandle>* assetHandles = r2::shashmap::Get(*cache->mUploadedTextureForMaterialMap, materialName, defaultAssetHandles);

			if (assetHandles == defaultAssetHandles)
			{
				//we don't have anything to unload
				continue;
			}

			const u32 numTexturesToUnload = r2::sarr::Size(*assetHandles);



			for (u32 i = 0; i < numTexturesToUnload; ++i)
			{
				auto handle = r2::sarr::At(*assetHandles, i);
				texsys::UnloadFromGPU(handle);
			}

			s32 defaultIndex = -1;

			s32 index = r2::shashmap::Get(*cache->mGPURenderMaterialIndices, materialName, defaultIndex);

			if (index == defaultIndex)
			{
				return;//?
			}

			RenderMaterialParams* gpuRenderMaterial = r2::sarr::At(*cache->mGPURenderMaterialArray, index);

			FREE(gpuRenderMaterial, *cache->mGPURenderMaterialArena);

			r2::sarr::At(*cache->mGPURenderMaterialArray, index) = nullptr;

			r2::squeue::PushBack(*cache->mFreeIndices, index);

			//r2::shashmap::Remove(*renderMaterialCache.mGPURenderMaterialIndices, materialName);

			//RemoveGPURenderMaterial(renderMaterialCache, materialName);

			FREE(assetHandles, *cache->mAssetHandleArena);

			r2::shashmap::Remove(*cache->mUploadedTextureForMaterialMap, materialName);

		//	UnloadMaterialParams(*cache, iter->key);
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

	const tex::Texture* FindTextureForTextureName(const r2::SArray<tex::Texture>* textures, u64 textureName)
	{
		const u32 numTextures = r2::sarr::Size(*textures);

		for (u32 i = 0; i < numTextures; ++i)
		{
			const tex::Texture& nextTexture = r2::sarr::At(*textures, i);

			if (nextTexture.textureAssetHandle.handle == textureName )
			{
				return &nextTexture;
			}
		}

		return nullptr;
	}

	bool IsTextureParamCubemapTexture(const tex::CubemapTexture* cubemapTexture, u64 textureName, flat::ShaderPropertyType propertyType)
	{
		return tex::GetCubemapAssetHandle(*cubemapTexture).handle == textureName && propertyType == flat::ShaderPropertyType_ALBEDO; //GetTextureTypeForPropertyType(propertyType) == tex::Diffuse;
	}

	bool IsTextureParamCubemapTexture(const r2::SArray<tex::CubemapTexture>* cubemapTextures, u64 textureName, flat::ShaderPropertyType propertyType, const tex::CubemapTexture** foundCubemap)
	{
		if (!cubemapTextures)
		{
			return false;
		}

		const auto numCubemapTextures = r2::sarr::Size(*cubemapTextures);

		for (u32 i = 0; i < numCubemapTextures; ++i)
		{
			const tex::CubemapTexture& cubemap = r2::sarr::At(*cubemapTextures, i);

			if (IsTextureParamCubemapTexture(&cubemap, textureName, propertyType))
			{
				*foundCubemap = &cubemap;
				return true;
			}
		}

		return false;
	}

	s32 GetWrapMode(const flat::ShaderTextureParam* texParam)
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

	s32 GetMinFilter(const flat::ShaderTextureParam* texParam)
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

	s32 GetMagFilter(const flat::ShaderTextureParam* texParam)
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

	bool UploadMaterialTextureParams(RenderMaterialCache& renderMaterialCache, const flat::Material* material, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture, bool shouldReload)
	{
		if (!material)
		{
			R2_CHECK(false, "materialParams is nullptr");
			return false;
		}

		const flatbuffers::Vector<flatbuffers::Offset<flat::ShaderTextureParam>>* textureParams = nullptr;
		flatbuffers::uoffset_t numTextureParams = 0;
		r2::SArray < tex::Texture >* materialTextures = nullptr;

		bool isCubemap = false;
		if (textures || cubemapTexture)
		{
			textureParams = material->shaderParams()->textureParams();
			numTextureParams = textureParams->size();

			isCubemap = cubemapTexture != nullptr;

			materialTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, tex::Texture, numTextureParams);
		}

		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const flat::ShaderTextureParam* textureParam = textureParams->Get(i);

			u64 textureHandle = textureParam->value()->assetName();

			if (textureHandle == EMPTY_TEXTURE)
			{
				continue;
			}

			if (!isCubemap)
			{
				//find the texture that matches textureHandle
				const tex::Texture* texture = FindTextureForTextureName(textures, textureHandle);

				if (texture)
				{
					r2::sarr::Push(*materialTextures, *texture);

					if (shouldReload && texsys::IsUploaded(texture->textureAssetHandle))
					{
						texsys::ReloadTexture(texture->textureAssetHandle);
					}
					else
					{//, texture->type
						texsys::UploadToGPU({ texture->textureAssetHandle }, textureParam->anisotropicFiltering(), GetWrapMode(textureParam), GetMinFilter(textureParam), GetMagFilter(textureParam));
					}
				}
			}
			else
			{
				flat::ShaderPropertyType propertyType = textureParam->propertyType();
				if (IsTextureParamCubemapTexture(cubemapTexture, textureHandle, propertyType))
				{
					const auto cubemapTextureAssetHandle = tex::GetCubemapAssetHandle(*cubemapTexture);
					if (shouldReload && texsys::IsUploaded(cubemapTextureAssetHandle))
					{
						texsys::ReloadTexture(cubemapTextureAssetHandle);
					}
					else
					{
						texsys::UploadToGPU(*cubemapTexture, textureParam->anisotropicFiltering(), GetWrapMode(textureParam), GetMinFilter(textureParam), GetMagFilter(textureParam));
					}
				}
			}
		}

		//Now update the GPURenderMaterials
		UpdateGPURenderMaterialForMaterial(renderMaterialCache, material, materialTextures, cubemapTexture);

		FREE(materialTextures, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	bool UploadMaterialTextureParamsArray(RenderMaterialCache& renderMaterialCache, const flat::MaterialPack* materialPack, const r2::SArray<tex::Texture>* textures, const r2::SArray<tex::CubemapTexture>* cubemapTextures)
	{
		R2_CHECK(materialPack != nullptr, "Should never happen");

		const auto* materials = materialPack->pack();

		for (flatbuffers::uoffset_t i = 0; i < materials->size(); ++i)
		{
			const flat::Material* material = materials->Get(i);

			const auto* textureParams = material->shaderParams()->textureParams();


			const auto numTextureParams = textureParams->size();//materialParams->textureParams()->size();
			
			bool isCubemap = false;
			const tex::CubemapTexture* foundCubemap = nullptr;
			//const flat::MaterialTextureParam* cubemapTextureParam = nullptr;
			const flat::ShaderTextureParam* cubemapTextureParam = nullptr;

			for (flatbuffers::uoffset_t j = 0; j < numTextureParams; ++j)
			{
				const flat::ShaderTextureParam* textureParam = textureParams->Get(j);

				u64 textureHandle = textureParam->value()->assetName();
				flat::ShaderPropertyType propertyType = textureParam->propertyType();

				if (textureHandle == EMPTY_TEXTURE || propertyType != flat::ShaderPropertyType_ALBEDO)
				{
					continue;
				}
				
				if (IsTextureParamCubemapTexture(cubemapTextures, textureHandle, propertyType, &foundCubemap))
				{
					cubemapTextureParam = textureParam;
					isCubemap = true;
					break;
				}
			}

			r2::SArray<tex::Texture>* materialTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, tex::Texture, numTextureParams);

			if (!isCubemap)
			{
				for (flatbuffers::uoffset_t j = 0; j < numTextureParams; ++j)
				{
					const flat::ShaderTextureParam* textureParam = textureParams->Get(j);

					u64 textureHandle = textureParam->value()->assetName();
					flat::ShaderPropertyType propertyType = textureParam->propertyType();

					if (textureHandle == EMPTY_TEXTURE)
					{
						continue;
					}

					const tex::Texture* texture = FindTextureForTextureName(textures, textureHandle);

					if (texture)
					{
						r2::sarr::Push(*materialTextures, *texture);
//, texture->type
						r2::draw::texsys::UploadToGPU({ texture->textureAssetHandle }, textureParam->anisotropicFiltering(), GetWrapMode(textureParam), GetMinFilter(textureParam), GetMagFilter(textureParam));
					}
				}
			}
			else
			{
				r2::draw::texsys::UploadToGPU(*foundCubemap, cubemapTextureParam->anisotropicFiltering(), GetWrapMode(cubemapTextureParam), GetMinFilter(cubemapTextureParam), GetMagFilter(cubemapTextureParam));
			}

			UpdateGPURenderMaterialForMaterial(renderMaterialCache, material, materialTextures, foundCubemap);

			FREE(materialTextures, *MEM_ENG_SCRATCH_PTR);
		}

		return true;
	}

	bool UnloadMaterial(RenderMaterialCache& renderMaterialCache, u64 materialName)
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

	bool UnloadMaterialArray(RenderMaterialCache& renderMaterialCache, const r2::SArray<u64>& materialNames)
	{
		const u32 numMaterialsToUnload = r2::sarr::Size(materialNames);

		for (u32 i = 0; i < numMaterialsToUnload; ++i)
		{
			UnloadMaterial(renderMaterialCache, r2::sarr::At(materialNames, i));
		}

		return true;
	}

	bool IsMaterialLoadedOnGPU(const RenderMaterialCache& renderMaterialCache, u64 materialName)
	{
		return r2::shashmap::Has(*renderMaterialCache.mGPURenderMaterialIndices, materialName);
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

	bool GetGPURenderMaterials(RenderMaterialCache& renderMaterialCache, const r2::SArray<u64>* handles, r2::SArray<RenderMaterialParams>* gpuRenderMaterials)
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
			u64 materialName = r2::sarr::At(*handles, i);

			const RenderMaterialParams* renderMaterialPtr = GetGPURenderMaterial(renderMaterialCache, materialName);

			R2_CHECK(renderMaterialPtr != nullptr, "Should never be nullptr");

			r2::sarr::Push(*gpuRenderMaterials, *renderMaterialPtr);
		}

		return true;
	}

	/*s32 GetPackingType(const flat::Material* material, tex::TextureType textureType)
	{
		R2_CHECK(material != nullptr, "This shouldn't be null!");

		const auto* textureParams = material->shaderParams()->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			const flat::ShaderTextureParam* texParam = textureParams->Get(i);

			auto paramTextureType = GetTextureTypeForPropertyType(texParam->propertyType());

			if (textureType != tex::TextureType::NUM_TEXTURE_TYPES && textureType == paramTextureType)
			{
				return texParam->packingType() > flat::ShaderPropertyPackingType_A ? -1 : texParam->packingType();
			}
		}

		return 0;
	}*/

	void ClearRenderMaterialParams(RenderMaterialParams* renderMaterialParams)
	{
		*renderMaterialParams = {};
	}

	void UpdateGPURenderMaterialForMaterial(RenderMaterialCache& renderMaterialCache, const flat::Material* material, const r2::SArray<tex::Texture>* textures, const tex::CubemapTexture* cubemapTexture)
	{
		R2_CHECK(material != nullptr, "This should not be nullptr");

		s32 defaultIndex;

		s32 index = r2::shashmap::Get(*renderMaterialCache.mGPURenderMaterialIndices, material->assetName()->assetName(), defaultIndex);

		RenderMaterialParams* gpuRenderMaterial = nullptr;

		if (index == defaultIndex)
		{
			//make a new one
			gpuRenderMaterial = AddNewGPURenderMaterial(renderMaterialCache, material);
		}
		else
		{
			gpuRenderMaterial = r2::sarr::At(*renderMaterialCache.mGPURenderMaterialArray, index);
			ClearRenderMaterialParams(gpuRenderMaterial);
		}

		R2_CHECK(gpuRenderMaterial != nullptr, "Should never happen");

		const auto* shaderParams = material->shaderParams();

		for (flatbuffers::uoffset_t i = 0; i < shaderParams->colorParams()->size(); ++i)
		{
			const flat::ShaderColorParam* colorParam = shaderParams->colorParams()->Get(i);
			if (colorParam->propertyType() == flat::ShaderPropertyType::ShaderPropertyType_ALBEDO)
			{
				gpuRenderMaterial->albedo.color = glm::vec4(colorParam->value()->r(), colorParam->value()->g(), colorParam->value()->b(), colorParam->value()->a());
			}
			if (colorParam->propertyType() == flat::ShaderPropertyType::ShaderPropertyType_EMISSION)
			{
				gpuRenderMaterial->emission.color = glm::vec4(colorParam->value()->r(), colorParam->value()->g(), colorParam->value()->b(), colorParam->value()->a());
			}
			if (colorParam->propertyType() == flat::ShaderPropertyType::ShaderPropertyType_DETAIL)
			{
				gpuRenderMaterial->detail.color = glm::vec4(colorParam->value()->r(), colorParam->value()->g(), colorParam->value()->b(), colorParam->value()->a());
			}
		}

		for (flatbuffers::uoffset_t i = 0; i < shaderParams->floatParams()->size(); ++i)
		{
			const flat::ShaderFloatParam* floatParam = shaderParams->floatParams()->Get(i);

			if (floatParam->propertyType() == flat::ShaderPropertyType::ShaderPropertyType_ROUGHNESS)
			{
				gpuRenderMaterial->roughness.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_METALLIC)
			{
				gpuRenderMaterial->metallic.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_REFLECTANCE)
			{
				gpuRenderMaterial->reflectance = floatParam->value();
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_AMBIENT_OCCLUSION)
			{
				gpuRenderMaterial->ao.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_CLEAR_COAT)
			{
				gpuRenderMaterial->clearCoat.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS)
			{
				gpuRenderMaterial->clearCoatRoughness.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_ANISOTROPY)
			{
				gpuRenderMaterial->anisotropy.color = glm::vec4(floatParam->value());
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_HEIGHT_SCALE)
			{
				gpuRenderMaterial->heightScale = floatParam->value();
			}

			if (floatParam->propertyType() == flat::ShaderPropertyType_EMISSION_STRENGTH)
			{
				gpuRenderMaterial->emissionStrength = floatParam->value();
			}
		}

		for (flatbuffers::uoffset_t i = 0; i < shaderParams->boolParams()->size(); ++i)
		{
			const flat::ShaderBoolParam* boolParam = shaderParams->boolParams()->Get(i);

			if (boolParam->propertyType() == flat::ShaderPropertyType_DOUBLE_SIDED)
			{
				gpuRenderMaterial->doubleSided = boolParam->value();
			}
		}

		if (!cubemapTexture)
		{
			for (flatbuffers::uoffset_t i = 0; i < shaderParams->textureParams()->size(); ++i)
			{
				const auto* textureParam = shaderParams->textureParams()->Get(i);
				u64 textureHandle = textureParam->value()->assetName();

				if (textureHandle == EMPTY_TEXTURE)
				{
					continue;
				}

				const tex::Texture* theTexture = FindTextureForTextureName(textures, textureHandle);

				if (!theTexture)
				{
					//put in the missing texture here
					theTexture = &renderMaterialCache.mMissingTexture;
				}


				auto propertyType = textureParam->propertyType();

				tex::TextureAddress address = texsys::GetTextureAddress(*theTexture);

			 	s32 channel = textureParam->packingType() > flat::ShaderPropertyPackingType_A ? -1 : textureParam->packingType();

				u32 textureCoordIndex = textureParam->textureCoordIndex();
				//@TODO(Serge): implement textureCoordinateIndex into the channel

				switch (propertyType)
				{
				case flat::ShaderPropertyType_ALBEDO:
					gpuRenderMaterial->albedo.texture = address;
					gpuRenderMaterial->albedo.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_METALLIC:
					gpuRenderMaterial->metallic.texture = address;
					gpuRenderMaterial->metallic.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_NORMAL:
					gpuRenderMaterial->normalMap.texture = address;
					gpuRenderMaterial->normalMap.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_EMISSION:
					gpuRenderMaterial->emission.texture = address;
					gpuRenderMaterial->emission.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_ROUGHNESS:
					gpuRenderMaterial->roughness.texture = address;
					gpuRenderMaterial->roughness.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_ANISOTROPY:
					gpuRenderMaterial->anisotropy.texture = address;
					gpuRenderMaterial->anisotropy.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_HEIGHT:
					gpuRenderMaterial->height.texture = address;
					gpuRenderMaterial->height.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_CLEAR_COAT:
					gpuRenderMaterial->clearCoat.texture = address;
					gpuRenderMaterial->clearCoat.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_CLEAR_COAT_NORMAL:
					gpuRenderMaterial->clearCoatNormal.texture = address;
					gpuRenderMaterial->clearCoatNormal.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS:
					gpuRenderMaterial->clearCoatRoughness.texture = address;
					gpuRenderMaterial->clearCoatRoughness.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_DETAIL:
					gpuRenderMaterial->detail.texture = address;
					gpuRenderMaterial->detail.texture.channel = channel;
					break;
				case flat::ShaderPropertyType_AMBIENT_OCCLUSION:
					gpuRenderMaterial->ao.texture = address;
					gpuRenderMaterial->ao.texture.channel = channel;
					break;
				default:
					R2_CHECK(false, "Unsupported texture type");
					break;
				} 
			}

			r2::SArray<r2::asset::AssetHandle>* textureAssetHandles = nullptr;

			if (!r2::shashmap::Has(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName()))
			{
				textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, r2::sarr::Size(*textures));

				r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName(), textureAssetHandles);
			}
			else
			{
				if (textures)
				{
					r2::SArray<r2::asset::AssetHandle>* defaultTextureAssetHandles = nullptr;

					textureAssetHandles = r2::shashmap::Get(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName(), defaultTextureAssetHandles);

					if (r2::sarr::Capacity(*textureAssetHandles) < r2::sarr::Size(*textures))
					{
						FREE(textureAssetHandles, *renderMaterialCache.mAssetHandleArena);

						r2::shashmap::Remove(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName());

						textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, r2::sarr::Size(*textures));

						r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName(), textureAssetHandles);
					}
				}
				
			}

			if (textures && textureAssetHandles)
			{
				r2::sarr::Clear(*textureAssetHandles);

				for (u32 i = 0; i < r2::sarr::Size(*textures); ++i)
				{
					r2::sarr::Push(*textureAssetHandles, r2::sarr::At(*textures, i).textureAssetHandle);
				}
			}
		}
		else
		{

			gpuRenderMaterial->cubemap.texture = texsys::GetTextureAddress(*cubemapTexture);

			const u32 numAssetHandles = cubemapTexture->numMipLevels * tex::CubemapSide::NUM_SIDES;

			r2::SArray<r2::asset::AssetHandle>* textureAssetHandles = nullptr;

			if (!r2::shashmap::Has(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName()))
			{
				textureAssetHandles = MAKE_SARRAY(*renderMaterialCache.mAssetHandleArena, r2::asset::AssetHandle, numAssetHandles);

				r2::shashmap::Set(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName(), textureAssetHandles);
			}
			else
			{
				r2::SArray<r2::asset::AssetHandle>* defaultTextureAssetHandles = nullptr;

				textureAssetHandles = r2::shashmap::Get(*renderMaterialCache.mUploadedTextureForMaterialMap, material->assetName()->assetName(), defaultTextureAssetHandles);

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

	RenderMaterialParams* AddNewGPURenderMaterial(RenderMaterialCache& renderMaterialCache, const flat::Material* material)
	{
		R2_CHECK(material != nullptr, "Should never be the case");

		u64 materialName = material->assetName()->assetName();

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
}