#include "r2pch.h"
#include "r2/Render/Model/Material.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/File/FileSystem.h"

#include "r2/Render/Model/Material_generated.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/MaterialParams_generated.h"
#include "r2/Render/Model/MaterialParamsPack_generated.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Render/Model/Textures/TexturePack.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Math/MathUtils.h"

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#endif


namespace r2::draw::mat
{
#ifdef R2_ASSET_PIPELINE
	r2::asset::pln::AssetThreadSafeQueue<std::string> s_texturesChangedQueue;
#endif // R2_ASSET_PIPELINE


	struct TextureLookUpEntry
	{
		MaterialHandle materialHandle;
		tex::Texture texture;
	};

	struct MaterialSystems
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		r2::mem::LinearArena* mSystemsArena;
		r2::SArray<r2::draw::MaterialSystem*>* mMaterialSystems = nullptr;

		r2::SHashMap<MaterialHandle>* mMaterialNamesToMaterialHandles = nullptr;
		r2::SHashMap<TextureLookUpEntry>* mTextureNamesToMaterialHandles = nullptr;
	};
}


namespace
{
	r2::draw::mat::MaterialSystems* s_optrMaterialSystems = nullptr;
	
	const u64 ALIGNMENT = 16;
	const u32 MAX_TEXTURES_PER_MATERIAL = 9;
	const u64 MAX_TEXTURE_PACKS = 1024;
	const u64 TEXTURE_PACK_CAPCITY = 1024;
	const f64 HASH_MAP_BUFFER_MULTIPLIER = 1.5f;
	const u32 NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME = 2;
	const u64 MIN_TEXTURE_CAPACITY = 8;

}



namespace r2::draw::mat
{
	const MaterialHandle InvalidMaterialHandle = {};

	r2::asset::FileList LoadTexturePacks(const MaterialSystem& system, const flat::TexturePacksManifest* texturePacksManifest, u32&, u32&);
	
	void LoadAllMaterialParamsFromMaterialParamsPack(MaterialSystem& system, const flat::MaterialParamsPack* materialParamsPack, u32 numNormalTexturePacks, u32 numCubemapTexturePacks, r2::asset::FileList list);
	void UploadMaterialTexturesToGPUInternal(MaterialSystem& system, u64 index);

	void UpdateRenderMaterialData(MaterialSystem& system, u64 materialIndex);

	void UpdateRenderMaterialDataTexture(MaterialSystem& system, u64 materialIndex, const tex::Texture& texture);
	void UpdateRenderMaterialDataTexture(MaterialSystem& system, u64 materialIndex, const tex::CubemapTexture& cubemap);
	void ClearRenderMaterialData(MaterialSystem& system);

	MaterialHandle MakeMaterialHandleFromIndex(const MaterialSystem& system, u64 index);
	u64 GetIndexFromMaterialHandle(MaterialHandle handle);

	r2::asset::AssetFile* FindAssetFile(const r2::asset::FileList& list, u64 assetID)
	{
		const u64 size = r2::sarr::Size(*list);

		for (u64 i = 0; i < size; ++i)
		{
			if (r2::sarr::At(*list, i)->GetAssetHandle(0) == assetID)
			{
				return r2::sarr::At(*list, i);
			}
		}

		return nullptr;
	}

	void AddTextureNameToMap(const MaterialSystem& system, MaterialHandle materialHandle, const r2::asset::AssetFile* file, tex::TextureType type)
	{
		if (file)
		{
			char assetName[r2::fs::FILE_PATH_LENGTH];

			file->GetAssetName(0, assetName, r2::fs::FILE_PATH_LENGTH);

			char filename[r2::fs::FILE_PATH_LENGTH];

			r2::fs::utils::CopyFileNameWithExtension(assetName, filename);

			auto filenameID = STRING_ID(filename);

			if (r2::shashmap::Has(*s_optrMaterialSystems->mTextureNamesToMaterialHandles, filenameID))
			{
				R2_CHECK(false, "We Shouldn't already have the asset: %s in our map!", assetName);
				return;
			}

			TextureLookUpEntry entry;
			entry.materialHandle = materialHandle;
			entry.texture.type = type;
			//@NOTE: this only works because we magically know the format of the asset handle. If it changes, this will have to change as well
			entry.texture.textureAssetHandle = { file->GetAssetHandle(0), static_cast<s64>(system.mAssetCache->GetSlot()) };

			r2::shashmap::Set(*s_optrMaterialSystems->mTextureNamesToMaterialHandles, filenameID, entry);
		}
	}



	MaterialHandle MakeMaterialHandleFromIndex(const MaterialSystem& system, u64 index)
	{
		const u64 numMaterials = r2::sarr::Size(*system.mInternalData);
		if (index >= numMaterials)
		{
			R2_CHECK(false, "the index is greater than the number of materials we have!");
			return InvalidMaterialHandle;
		}

		MaterialHandle handle;
		handle.slot = system.mSlot;
		handle.handle = static_cast<u32>(index + 1);
		return handle;
	}

	u64 GetIndexFromMaterialHandle(MaterialHandle handle)
	{
		if (handle.handle == InvalidMaterialHandle.handle)
		{
			R2_CHECK(false, "You passed in an invalid material handle!");
			return 0;
		}

		return static_cast<u64>(handle.handle - 1);
	}

	bool IsInvalidHandle(const MaterialHandle& materialHandle)
	{
		return materialHandle.handle == mat::InvalidMaterialHandle.handle || materialHandle.slot == mat::InvalidMaterialHandle.slot;
	}

	bool IsValid(const MaterialHandle& materialHandle)
	{
		return !IsInvalidHandle(materialHandle);
	}

	bool AreMaterialHandlesEqual(const MaterialHandle& materialHandle1, const MaterialHandle& materialHandle2)
	{
		return materialHandle1.handle == materialHandle2.handle && materialHandle1.slot == materialHandle2.slot;
	}

	void LoadAllMaterialTexturesFromDisk(MaterialSystem& system)
	{
		if (!system.mMaterialTextures ||
			!system.mTexturePacks)
		{
			R2_CHECK(false, "Material system has not been initialized!");
			return;
		}

		const u64 numMaterials = r2::sarr::Size(*system.mInternalData);
		for(u64 i = 0; i < numMaterials; ++i)
		{
			const MaterialTextureEntry& textureEntry = r2::sarr::At(*system.mMaterialTextureEntries, i);

			if (textureEntry.mIndex != -1) //we already have them loaded - or have loaded before
				continue;

			//const Material& material = r2::sarr::At(*system.mMaterials, i);

			const InternalMaterialData& internalMaterialData = r2::sarr::At(*system.mInternalData, i);

			//We may want to move this to the load from disk as that's what this is basically for...
			r2::draw::tex::TexturePack* defaultTexturePack = nullptr;
			r2::draw::tex::TexturePack* result = r2::shashmap::Get(*system.mTexturePacks, internalMaterialData.texturePackName, defaultTexturePack);

			if (internalMaterialData.texturePackName == STRING_ID(""))
			{
				//we don't have one!
				continue;
			}

			if (result == defaultTexturePack )
			{
				R2_CHECK(false, "We couldn't get the texture pack!");
				return;
			}

			MaterialTextureEntry newEntry;
			newEntry.mType = result->metaData.assetType;

			if (result->metaData.assetType == r2::asset::TEXTURE)
			{
				r2::SArray<r2::draw::tex::Texture>* materialTextures = MAKE_SARRAY(*system.mLinearArena, r2::draw::tex::Texture, result->totalNumTextures);

				R2_CHECK(materialTextures != nullptr, "We couldn't allocate the array for the material textures");

				const auto numAlbedos = r2::sarr::Size(*result->albedos);
				for (u64 t = 0; t < numAlbedos; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Diffuse;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->albedos, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numSpeculars = r2::sarr::Size(*result->speculars);
				for (flatbuffers::uoffset_t t = 0; t < numSpeculars; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Specular;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->speculars, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numEmissives = r2::sarr::Size(*result->emissives);
				for (u64 t = 0; t < numEmissives; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Emissive;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->emissives, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numNormals = r2::sarr::Size(*result->normals);
				for (flatbuffers::uoffset_t t = 0; t < numNormals; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Normal;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->normals, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numMetalics = r2::sarr::Size(*result->metallics);
				for (u64 t = 0; t < numMetalics; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Metallic;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->metallics, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numHeights = r2::sarr::Size(*result->heights);
				for (u64 t = 0; t < numHeights; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Height;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->heights, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numMicros = r2::sarr::Size(*result->micros);
				for (u64 t = 0; t < numMicros; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Roughness;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->micros, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numOcclusions = r2::sarr::Size(*result->occlusions);
				for (flatbuffers::uoffset_t t = 0; t < numOcclusions; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Occlusion;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->occlusions, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				const auto numAnisotropys = r2::sarr::Size(*result->anisotropys);
				for (flatbuffers::uoffset_t t = 0; t < numAnisotropys; ++t)
				{
					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Anisotropy;
					texture.textureAssetHandle =
						system.mAssetCache->LoadAsset(r2::sarr::At(*result->anisotropys, t));
					r2::sarr::Push(*materialTextures, texture);
				}

				newEntry.mIndex = r2::sarr::Size(*system.mMaterialTextures);
				
				r2::sarr::Push(*system.mMaterialTextures, materialTextures);
			}
			else
			{
				r2::draw::tex::CubemapTexture cubemap;

				const auto numMipLevels = result->metaData.numLevels;
				cubemap.numMipLevels = numMipLevels;

				for (auto mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
				{
					for (auto side = 0; side < r2::draw::tex::NUM_SIDES; side++)
					{
						r2::draw::tex::Texture texture;
						texture.type = tex::TextureType::Diffuse;

						auto cubemapSide = result->metaData.mipLevelMetaData[mipLevel].sides[side].side;

						texture.textureAssetHandle = system.mAssetCache->LoadAsset(result->metaData.mipLevelMetaData[mipLevel].sides[cubemapSide].asset);
						cubemap.mips[mipLevel].mipLevel = result->metaData.mipLevelMetaData[mipLevel].mipLevel;
						cubemap.mips[mipLevel].sides[cubemapSide] = texture;
					}
				}

				newEntry.mIndex = r2::sarr::Size(*system.mMaterialCubemapTextures);

				r2::sarr::Push(*system.mMaterialCubemapTextures, cubemap);
			}

			system.mMaterialTextureEntries->mData[i] = newEntry;
		}
	}

	void UploadAllMaterialTexturesToGPU(MaterialSystem& system)
	{
		const u64 capacity = r2::sarr::Capacity(*system.mMaterialTextureEntries);

		for (u64 i = 0; i < capacity; ++i)
		{
			UploadMaterialTexturesToGPUInternal(system, i);
		}
	}

	void UploadMaterialTexturesToGPUFromMaterialName(MaterialSystem& system, u64 materialName)
	{
		UploadMaterialTexturesToGPU(system, GetMaterialHandleFromMaterialName(system, materialName));
	}

	void UploadMaterialTexturesToGPU(MaterialSystem& system, MaterialHandle matID)
	{
		u64 index = GetIndexFromMaterialHandle(matID);
		UploadMaterialTexturesToGPUInternal(system, index);
	}

	void UploadMaterialTexturesToGPUInternal(MaterialSystem& system, u64 materialIndex)
	{
		const MaterialTextureEntry& entry = r2::sarr::At(*system.mMaterialTextureEntries, materialIndex);
		
		if (entry.mIndex == -1)
		{
			return;
		}

		if (entry.mType == r2::asset::TEXTURE)
		{
			r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*system.mMaterialTextures, entry.mIndex);

			if (!textures)
			{
				return;
			}

			const u64 numTextures = r2::sarr::Size(*textures);

			for (u64 i = 0; i < numTextures; ++i)
			{
				const auto& texture = r2::sarr::At(*textures, i);
				r2::draw::texsys::UploadToGPU(texture.textureAssetHandle, texture.type);

				//UpdateRenderMaterialDataTexture(system, materialIndex, texture);
			}
		}
		else
		{
			const r2::draw::tex::CubemapTexture& cubemap = r2::sarr::At(*system.mMaterialCubemapTextures, entry.mIndex);
			
			r2::draw::texsys::UploadToGPU(cubemap);

			//UpdateRenderMaterialDataTexture(system, materialIndex, cubemap);
		}

		UpdateRenderMaterialData(system, materialIndex);
	}

	void UnloadAllMaterialTexturesFromGPU(MaterialSystem& system)
	{
		const u64 capacity = r2::sarr::Capacity(*system.mMaterialTextures);

		for (u64 i = 0; i < capacity; ++i)
		{
			r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*system.mMaterialTextures, i);

			if (!textures)
			{
				continue;
			}

			const u64 numTextures = r2::sarr::Size(*textures);

			for (u64 t = 0; t < numTextures; ++t)
			{
				r2::draw::texsys::UnloadFromGPU(r2::sarr::At(*textures, t).textureAssetHandle);
			}
		}

		if (system.mMaterialCubemapTextures)
		{
			const u64 numCubemapTextures = r2::sarr::Size(*system.mMaterialCubemapTextures);
			for (u64 t = 0; t < numCubemapTextures; ++t)
			{
				const auto numMipLevels = r2::sarr::At(*system.mMaterialCubemapTextures, t).numMipLevels;

				for (u32 i = 0; i < numMipLevels; ++i)
				{
					for (size_t s = 0; s < r2::draw::tex::NUM_SIDES; ++s)
					{
						r2::draw::texsys::UnloadFromGPU(r2::sarr::At(*system.mMaterialCubemapTextures, t).mips[i].sides[s].textureAssetHandle);
					}
				}
			}
		}

		ClearRenderMaterialData(system);

	}

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(const MaterialSystem& system, MaterialHandle matID)
	{
		if (matID.handle == InvalidMaterialHandle.handle && system.mSlot == matID.slot)
		{
			R2_CHECK(false, "Passed in an invalid material ID");
			return nullptr;
		}

		u64 materialIndex = GetIndexFromMaterialHandle(matID);

		const MaterialTextureEntry& entry = r2::sarr::At(*system.mMaterialTextureEntries, materialIndex);

		if (entry.mType == r2::asset::TEXTURE)
		{
			return r2::sarr::At(*system.mMaterialTextures, entry.mIndex);
		}
		
		return nullptr;
		
	}

	const r2::SArray<r2::draw::tex::CubemapTexture>* GetCubemapTextures(const MaterialSystem& system)
	{
		return system.mMaterialCubemapTextures;
	}

	const r2::draw::tex::CubemapTexture* GetCubemapTexture(const MaterialSystem& system, MaterialHandle matID)
	{
		if (IsInvalidHandle(matID))
		{
			return nullptr;
		}

		const auto index = mat::GetIndexFromMaterialHandle(matID);
		
		const InternalMaterialData& internalMaterialData = r2::sarr::At(*system.mInternalData, index);

		
		const auto& assetHandle = internalMaterialData.textureAssets.diffuseTexture.textureAssetHandle;

		const u64 numCubemapTextures = r2::sarr::Size(*system.mMaterialCubemapTextures);
		for (u64 i = 0; i < numCubemapTextures; ++i)
		{
			const r2::draw::tex::CubemapTexture& cubemap = r2::sarr::At(*system.mMaterialCubemapTextures, i);
			const auto& nextAssetHandle = GetCubemapAssetHandle(cubemap);
			
			if (nextAssetHandle.handle == assetHandle.handle &&
				nextAssetHandle.assetCache == assetHandle.assetCache)
			{
				return &cubemap;
			}
		}

		return nullptr;
	}

	//MaterialHandle AddMaterial(MaterialSystem& system, const Material& mat)
	//{
	//	//see if we already have this material - if we do then return that
	//	const u64 numMaterials = r2::sarr::Size(*system.mMaterials);

	//	for (u64 i = 0; i < numMaterials; ++i)
	//	{
	//		const Material& nextMaterial = r2::sarr::At(*system.mMaterials, i);
	//		if (mat.materialID == nextMaterial.materialID)
	//		{
	//			return MakeMaterialHandleFromIndex(system, i);
	//		}
	//	}

	//	r2::sarr::Push(*system.mMaterials, mat);
	//	return MakeMaterialHandleFromIndex(system, numMaterials);
	//}

	//const Material* GetMaterial(const MaterialSystem& system, MaterialHandle matID)
	//{
	//	u64 materialIndex = GetIndexFromMaterialHandle(matID);

	//	if (materialIndex >= r2::sarr::Size(*system.mMaterials))
	//	{
	//		return nullptr;
	//	}

	//	return &r2::sarr::At(*system.mMaterials, materialIndex);
	//}

	ShaderHandle GetShaderHandle(const MaterialSystem& system, MaterialHandle matID)
	{
		u64 materialIndex = GetIndexFromMaterialHandle(matID);

		if (materialIndex >= r2::sarr::Size(*system.mInternalData))
		{
			return InvalidShader;
		}

		return r2::sarr::At(*system.mInternalData, materialIndex).shaderHandle;
	}

	const RenderMaterialParams& GetRenderMaterial(const MaterialSystem& system, MaterialHandle matID)
	{
		u64 materialIndex = GetIndexFromMaterialHandle(matID);

		if (materialIndex >= r2::sarr::Size(*system.mInternalData))
		{
			return RenderMaterialParams{};
		}

		return r2::sarr::At(*system.mInternalData, materialIndex).renderMaterial;
	}

	MaterialHandle GetMaterialHandleFromMaterialName(const MaterialSystem& system, u64 materialName)
	{
		const u64 numMaterials = r2::sarr::Size(*system.mInternalData);

		for (u64 i = 0; i < numMaterials; ++i)
		{
			const InternalMaterialData& nextMaterial = r2::sarr::At(*system.mInternalData, i);
			if (materialName == nextMaterial.materialName)
			{
				return MakeMaterialHandleFromIndex(system, i);
			}
		}

		return InvalidMaterialHandle;
	}

	u64 LoadMaterialAndTextureManifests(const char* materialManifestPath, const char* textureManifestPath, void** materialPackData, void** texturePacksData)
	{
		if (!materialManifestPath || strcmp(materialManifestPath, "") == 0)
		{
			R2_CHECK(false, "materialManifestPath is null or empty");
			return 0;
		}

		if (!textureManifestPath || strcmp(textureManifestPath, "") == 0)
		{
			R2_CHECK(false, "textureManifestPath is null or empty");
			return 0;
		}

		if (materialPackData == nullptr || texturePacksData == nullptr)
		{

			return 0;
		}


		*materialPackData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, materialManifestPath);
		if (!(*materialPackData))
		{
			R2_CHECK(false, "Failed to read the material pack file: %s", materialManifestPath);
			return false;
		}

		const flat::MaterialPack* materialPack = flat::GetMaterialPack(*materialPackData);

		R2_CHECK(materialPack != nullptr, "Failed to get the material pack from the data!");

		*texturePacksData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, textureManifestPath);
		if (!(*texturePacksData))
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", textureManifestPath);
			return false;
		}

		const flat::TexturePacksManifest* texturePacks = flat::GetTexturePacksManifest(*texturePacksData);

		R2_CHECK(texturePacks != nullptr, "Failed to get the material pack from the data!");

		return r2::draw::mat::MemorySize(64, materialPack->materials()->size(),
			texturePacks->totalTextureSize(),
			texturePacks->totalNumberOfTextures(),
			texturePacks->texturePacks()->size(),
			texturePacks->maxTexturesInAPack());
	}

	u64 GetMaterialBoundarySize(const char* materialManifestPath, const char* textureManifestPath)
	{
		if (!materialManifestPath || strcmp(materialManifestPath, "") == 0)
		{
			R2_CHECK(false, "materialManifestPath is null or empty");
			return 0;
		}

		if (!textureManifestPath || strcmp(textureManifestPath, "") == 0)
		{
			R2_CHECK(false, "textureManifestPath is null or empty");
			return 0;
		}

		u64 materialParamsPackSize = 0;
		void* materialParamsPackData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, materialManifestPath, materialParamsPackSize);

		if (!materialParamsPackData)
		{
			R2_CHECK(false, "Failed to read the material params pack file: %s", materialManifestPath);
			return false;
		}

		u64 texturePacksSize = 0;
		void* texturePacksData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, textureManifestPath, texturePacksSize);
		if (!texturePacksData)
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", textureManifestPath);
			return false;
		}

		const flat::MaterialParamsPack* materialPack = flat::GetMaterialParamsPack(materialParamsPackData);
		R2_CHECK(materialPack != nullptr, "Why would this be null at this point? Problem in flatbuffers?");

		const flat::TexturePacksManifest* texturePack = flat::GetTexturePacksManifest(texturePacksData);
		R2_CHECK(texturePack != nullptr, "Why would this be null at this point? Problem in flatbuffers?");

		const auto memorySize = r2::draw::mat::MemorySize(16, materialPack->pack()->size(),
			texturePack->totalTextureSize(),
			texturePack->totalNumberOfTextures(),
			texturePack->texturePacks()->size(),
			texturePack->maxTexturesInAPack(), materialParamsPackSize, texturePacksSize);

		FREE(texturePacksData, *MEM_ENG_SCRATCH_PTR);
		FREE(materialParamsPackData, *MEM_ENG_SCRATCH_PTR);

		return memorySize;
	}
	

	u64 MemorySize(u64 alignment, u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack, u32 materialParamsFileSize, u32 texturePacksManifestFileSize)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 hashCapacity = numMaterials;
		u64 fullAtCapacity = r2::SHashMap<r2::draw::tex::TexturePack*>::FullAtCapacity(numMaterials);
		while (hashCapacity <= fullAtCapacity )
		{
			hashCapacity = static_cast<u64>(round(static_cast<f64>(hashCapacity) * HASH_MAP_BUFFER_MULTIPLIER));
		}

		hashCapacity = std::min(hashCapacity, MIN_TEXTURE_CAPACITY);

		u64 memorySize = 0;
		u64 memSize1 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), alignment, headerSize, boundsChecking);
		u64 memSize2 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::MaterialSystem), alignment, headerSize, boundsChecking);
		//u64 memSize3 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::Material>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
		u64 memSize4 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::SArray<r2::draw::tex::Texture>*>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
		u64 memSize5 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::tex::Texture>::MemorySize(maxTexturesInAPack), alignment, headerSize, boundsChecking) * numMaterials;
		u64 memSize6 = r2::mem::utils::GetMaxMemoryForAllocation(r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, totalNumberOfTextures, textureCacheInBytes, alignment), alignment, headerSize, boundsChecking);
		u64 memSize7 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::draw::tex::TexturePack*>::MemorySize(hashCapacity), alignment, headerSize, boundsChecking);

		u64 t = r2::draw::tex::TexturePackMemorySize(maxTexturesInAPack, alignment, headerSize, boundsChecking);
		u64 memSize8 = r2::mem::utils::GetMaxMemoryForAllocation(t, alignment, headerSize, boundsChecking) * numPacks;
		u64 memSize9 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::SArray<r2::draw::tex::CubemapTexture>>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
		u64 memSize10 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialTextureEntry>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
		u64 memSize11 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::tex::CubemapTexture>::MemorySize(maxTexturesInAPack), alignment, headerSize, boundsChecking);
		u64 memSize12 = r2::mem::utils::GetMaxMemoryForAllocation(materialParamsFileSize, alignment, headerSize, boundsChecking);
		u64 memSize13 = r2::mem::utils::GetMaxMemoryForAllocation(texturePacksManifestFileSize, alignment, headerSize, boundsChecking);
		u64 memSize14 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<InternalMaterialData>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);

		memorySize = memSize1 + memSize2  + memSize4 + memSize5 + memSize6 + memSize7 + memSize8 + memSize9 + memSize10 + memSize11 + memSize12 + memSize13 + memSize14;

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, alignment);
	}

	void LoadAllMaterialParamsFromMaterialParamsPack(MaterialSystem& system, const flat::MaterialParamsPack* materialParamsPack, u32 numNormalTexturePacks, u32 numCubemapTexturePacks, r2::asset::FileList list)
	{
		R2_CHECK(materialParamsPack != nullptr, "Material pack is nullptr");

		const auto numMaterials = materialParamsPack->pack()->size();

		system.mMaterialTextureEntries = MAKE_SARRAY(*system.mLinearArena, MaterialTextureEntry, numMaterials);
		R2_CHECK(system.mMaterialTextureEntries != nullptr, "We couldn't allocate the material texture entries!");

		system.mMaterialTextureEntries->mSize = numMaterials;

		if (numNormalTexturePacks > 0)
		{
			system.mMaterialTextures = MAKE_SARRAY(*system.mLinearArena, r2::SArray<r2::draw::tex::Texture>*, numMaterials);
			R2_CHECK(system.mMaterialTextures != nullptr, "We couldn't allocate the material textures!");

			for (u64 i = 0; i < numMaterials; ++i)
			{
				system.mMaterialTextures->mData[i] = nullptr;
			}
		}

		if (numCubemapTexturePacks > 0)
		{
			system.mMaterialCubemapTextures = MAKE_SARRAY(*system.mLinearArena, r2::draw::tex::CubemapTexture, numCubemapTexturePacks);
			R2_CHECK(system.mMaterialCubemapTextures != nullptr, "We couldn't allocate the material cubemaps!");
		}

		for (u64 i = 0; i < numMaterials; ++i)
		{
			system.mMaterialTextureEntries->mData[i] = MaterialTextureEntry{};
		}

		const auto EMPTY = STRING_ID("");

		for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
		{
			const flat::MaterialParams* material = materialParamsPack->pack()->Get(i);
			MaterialTextureAssets materialTextureAssets;
			RenderMaterialParams renderMaterialParams;

			ShaderHandle shaderHandle = InvalidShader;
			auto materialID =  material->name();
			u64 texturePackName = 0;

			for (flatbuffers::uoffset_t i = 0; i < material->ulongParams()->size(); ++i)
			{
				const flat::MaterialULongParam* ulongParam = material->ulongParams()->Get(i);

				if (ulongParam->propertyType() == flat::MaterialPropertyType_SHADER)
				{
					shaderHandle = r2::draw::shadersystem::FindShaderHandle(ulongParam->value());
				}
			}

			r2::sarr::Push(*system.mInternalData, InternalMaterialData{});
			InternalMaterialData& internalMaterialData = r2::sarr::Last(*system.mInternalData);

			internalMaterialData.materialName = material->name();
			
			internalMaterialData.shaderHandle = shaderHandle;
			
			r2::asset::AssetFile* diffuseFile = nullptr;
			r2::asset::AssetFile* detailFile = nullptr;
			r2::asset::AssetFile* normalMapFile = nullptr;
			r2::asset::AssetFile* emissionFile = nullptr;
			r2::asset::AssetFile* metallicFile = nullptr;
			r2::asset::AssetFile* roughnessFile = nullptr;
			r2::asset::AssetFile* aoFile = nullptr;
			r2::asset::AssetFile* heightFile = nullptr;
			r2::asset::AssetFile* anisotropyFile = nullptr;

			for (flatbuffers::uoffset_t i = 0; i < material->textureParams()->size(); ++i)
			{
				const flat::MaterialTextureParam* textureParam = material->textureParams()->Get(i);

				//@TODO(Serge): this should probably be per texture not per material but...
				internalMaterialData.texturePackName = textureParam->texturePackName();

				if (textureParam->propertyType() == flat::MaterialPropertyType_ALBEDO &&
					textureParam->value() != EMPTY)
				{
					diffuseFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(diffuseFile != nullptr, "This should never be null!");

					materialTextureAssets.diffuseTexture.type = tex::Diffuse;
					materialTextureAssets.diffuseTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_NORMAL &&
					textureParam->value() != EMPTY)
				{
					normalMapFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(normalMapFile != nullptr, "This should never be null!");

					materialTextureAssets.normalMapTexture.type = tex::Normal;
					materialTextureAssets.normalMapTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_EMISSION &&
					textureParam->value() != EMPTY)
				{
					emissionFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(emissionFile != nullptr, "This should never be null!");

					materialTextureAssets.emissionTexture.type = tex::Emissive;
					materialTextureAssets.emissionTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_METALLIC &&
					textureParam->value() != EMPTY)
				{
					metallicFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(metallicFile != nullptr, "This should never be null!");

					materialTextureAssets.metallicTexture.type = tex::Metallic;
					materialTextureAssets.metallicTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_ROUGHNESS &&
					textureParam->value() != EMPTY)
				{
					roughnessFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(roughnessFile != nullptr, "This should never be null!");

					materialTextureAssets.roughnessTexture.type = tex::Roughness;
					materialTextureAssets.roughnessTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_AMBIENT_OCCLUSION &&
					textureParam->value() != EMPTY)
				{
					aoFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(aoFile != nullptr, "This should never be null!");

					materialTextureAssets.aoTexture.type = tex::Occlusion;
					materialTextureAssets.aoTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_HEIGHT &&
					textureParam->value() != EMPTY)
				{
					heightFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(heightFile != nullptr, "This should never be null!");

					materialTextureAssets.heightTexture.type = tex::Height;
					materialTextureAssets.heightTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}

				if (textureParam->propertyType() == flat::MaterialPropertyType_ANISOTROPY &&
					textureParam->value() != EMPTY)
				{
					anisotropyFile = FindAssetFile(list, textureParam->value());
					R2_CHECK(anisotropyFile != nullptr, "This should never be null!");

					materialTextureAssets.anisotropyTexture.type = tex::Anisotropy;
					materialTextureAssets.anisotropyTexture.textureAssetHandle = { textureParam->value(), static_cast<s64>(system.mAssetCache->GetSlot()) };
				}
			}

			internalMaterialData.textureAssets = materialTextureAssets;

			const auto& handle = MakeMaterialHandleFromIndex(system, i);

			r2::draw::MaterialHandle defaultHandle;

			MaterialHandle temp = r2::shashmap::Get(*s_optrMaterialSystems->mMaterialNamesToMaterialHandles, materialID, defaultHandle);

			R2_CHECK(temp.handle == defaultHandle.handle && temp.slot == defaultHandle.slot, "We shouldn't already have an entry for this material!");

			r2::shashmap::Set(*s_optrMaterialSystems->mMaterialNamesToMaterialHandles, materialID, handle);

			AddTextureNameToMap(system, handle, diffuseFile, tex::Diffuse);
			AddTextureNameToMap(system, handle, normalMapFile, tex::Normal);
			AddTextureNameToMap(system, handle, emissionFile, tex::Emissive);
			AddTextureNameToMap(system, handle, metallicFile, tex::Metallic);
			AddTextureNameToMap(system, handle, roughnessFile, tex::Roughness); 
			AddTextureNameToMap(system, handle, aoFile, tex::Occlusion);
			AddTextureNameToMap(system, handle, heightFile, tex::Height);
			AddTextureNameToMap(system, handle, anisotropyFile, tex::Anisotropy);
		}
	}

	r2::asset::FileList LoadTexturePacks(const MaterialSystem& system, const flat::TexturePacksManifest* texturePacksManifest, u32& numNormalTexturePacks, u32& numCubemapTexturePacks)
	{
		const flatbuffers::uoffset_t numTexturePacks = texturePacksManifest->texturePacks()->size();

		u32 maxNumTextures = 0;
		numCubemapTexturePacks = 0;
		numNormalTexturePacks = 0;

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const flat::TexturePack* nextPack = texturePacksManifest->texturePacks()->Get(i);

			maxNumTextures = std::max({
				maxNumTextures,
				nextPack->albedo()->size(),
				nextPack->emissive()->size(),
				nextPack->height()->size(),
				nextPack->metallic()->size(),
				nextPack->micro()->size(),
				nextPack->normal()->size(),
				nextPack->occlusion()->size(),
				nextPack->specular()->size(),
				nextPack->anisotropy()->size()});

			if (nextPack->metaData()->type() == flat::TextureType_CUBEMAP)
			{
				++numCubemapTexturePacks;
			}
			else if (nextPack->metaData()->type() == flat::TextureType_TEXTURE)
			{
				++numNormalTexturePacks;
			}
		}

		R2_CHECK(maxNumTextures <= TEXTURE_PACK_CAPCITY, "We're not allocating enough space for the texture pack capacity!");

		r2::asset::FileList list = r2::asset::lib::MakeFileList(texturePacksManifest->totalNumberOfTextures());

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const flat::TexturePack* nextPack = texturePacksManifest->texturePacks()->Get(i);

			r2::draw::tex::TexturePack* texturePack = MAKE_TEXTURE_PACK(*system.mLinearArena, maxNumTextures);

			R2_CHECK(texturePack != nullptr, "Failed to allocate the texture pack!");

			texturePack->totalNumTextures = nextPack->totalNumberOfTextures();

			r2::asset::AssetType textureType = r2::asset::TEXTURE;

			if (nextPack->metaData()->type() == flat::TextureType::TextureType_CUBEMAP)
			{
				textureType = r2::asset::CUBEMAP_TEXTURE;
			}

			texturePack->metaData.assetType = textureType;

			if (nextPack->metaData()->mipLevels())
			{
				const auto numMipLevels = nextPack->metaData()->mipLevels()->size();

				texturePack->metaData.numLevels = static_cast<u32>( numMipLevels );

				for (flatbuffers::uoffset_t m = 0; m < numMipLevels; ++m)
				{
					const auto mipLevel = nextPack->metaData()->mipLevels()->Get(m);

					texturePack->metaData.mipLevelMetaData[m].mipLevel = mipLevel->level();

					const auto numSides = mipLevel->sides()->size();

					for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
					{
						const auto side = mipLevel->sides()->Get(i);


						tex::CubemapSide cubemapSide = (tex::CubemapSide)side->side();

						texturePack->metaData.mipLevelMetaData[m].sides[cubemapSide].side = cubemapSide;

						char assetName[r2::fs::FILE_PATH_LENGTH];
						r2::fs::utils::CopyFileNameWithParentDirectories(side->textureName()->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
						r2::asset::Asset textureAsset(assetName, textureType);
						texturePack->metaData.mipLevelMetaData[m].sides[cubemapSide].asset = textureAsset;
					}
				}
			}

			const auto numAlbedos = nextPack->albedo()->size();
			for (flatbuffers::uoffset_t t = 0; t < numAlbedos; ++t)
			{
				auto nextTexturePath = nextPack->albedo()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->albedos, textureAsset);
			}

			const auto numEmissives = nextPack->emissive()->size();
			for (flatbuffers::uoffset_t t = 0; t < numEmissives; ++t)
			{
				auto nextTexturePath = nextPack->emissive()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);


				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->emissives, textureAsset);
			}

			const auto numHeights = nextPack->height()->size();
			for (flatbuffers::uoffset_t t = 0; t < numHeights; ++t)
			{
				auto nextTexturePath = nextPack->height()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->heights, textureAsset);
			}

			const auto numMetalics = nextPack->metallic()->size();
			for (flatbuffers::uoffset_t t = 0; t < numMetalics; ++t)
			{
				auto nextTexturePath = nextPack->metallic()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->metallics, textureAsset);
			}

			const auto numMicros = nextPack->micro()->size();
			for (flatbuffers::uoffset_t t = 0; t < numMicros; ++t)
			{
				auto nextTexturePath = nextPack->micro()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->micros, textureAsset);
			}

			const auto numNormals = nextPack->normal()->size();
			for (flatbuffers::uoffset_t t = 0; t < numNormals; ++t)
			{
				auto nextTexturePath = nextPack->normal()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->normals, textureAsset);
			}

			const auto numOcclusions = nextPack->occlusion()->size();
			for (flatbuffers::uoffset_t t = 0; t < numOcclusions; ++t)
			{
				auto nextTexturePath = nextPack->occlusion()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->occlusions, textureAsset);
			}

			const auto numSpeculars = nextPack->specular()->size();
			for (flatbuffers::uoffset_t t = 0; t < numSpeculars; ++t)
			{
				auto nextTexturePath = nextPack->specular()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->speculars, textureAsset);
			}

			const auto numAnisotropies = nextPack->anisotropy()->size();
			for (flatbuffers::uoffset_t t = 0; t < numAnisotropies; ++t)
			{
				auto nextTexturePath = nextPack->anisotropy()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName, textureType);
				r2::sarr::Push(*texturePack->anisotropys, textureAsset);

			}

			texturePack->packName = nextPack->packName();
			

			r2::shashmap::Set(*system.mTexturePacks, nextPack->packName(), texturePack);
		}

		return list;
	}

	void UpdateRenderMaterialData(MaterialSystem& system, u64 materialIndex)
	{
		if (!system.mInternalData)
		{
			R2_CHECK(false, "The system hasn't allocated the internal data!");
			return;
		}

		const MaterialTextureEntry& entry = r2::sarr::At(*system.mMaterialTextureEntries, materialIndex);

		if (entry.mIndex == -1)
		{
			return;
		}

		if (system.mMaterialParamsPack != nullptr)
		{
			InternalMaterialData& internalMaterialData = r2::sarr::At(*system.mInternalData, materialIndex);

			const flat::MaterialParams* materialParams = system.mMaterialParamsPack->pack()->Get(materialIndex);

			R2_CHECK(materialParams != nullptr, "This shouldn't be null!");

			for (flatbuffers::uoffset_t i = 0; i < materialParams->colorParams()->size(); ++i)
			{
				const flat::MaterialColorParam* colorParam = materialParams->colorParams()->Get(i);

				if (colorParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_ALBEDO)
				{
					internalMaterialData.renderMaterial.albedo.color = glm::vec4(colorParam->value()->r(), colorParam->value()->g(), colorParam->value()->b(), colorParam->value()->a());
				}
			}

			for (flatbuffers::uoffset_t i = 0; i < materialParams->floatParams()->size(); ++i)
			{
				const flat::MaterialFloatParam* floatParam = materialParams->floatParams()->Get(i);

				if (floatParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_ROUGHNESS)
				{
					internalMaterialData.renderMaterial.roughness.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_METALLIC)
				{
					internalMaterialData.renderMaterial.metallic.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_REFLECTANCE)
				{
					internalMaterialData.renderMaterial.reflectance = floatParam->value();
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_AMBIENT_OCCLUSION)
				{
					internalMaterialData.renderMaterial.ao.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT)
				{
					internalMaterialData.renderMaterial.clearCoat.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS)
				{
					internalMaterialData.renderMaterial.clearCoatRoughness.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_ANISOTROPY)
				{
					internalMaterialData.renderMaterial.anisotropy.color = glm::vec4( floatParam->value() );
				}

				if (floatParam->propertyType() == flat::MaterialPropertyType_HEIGHT_SCALE)
				{
					internalMaterialData.renderMaterial.heightScale = floatParam->value();
				}
			}

			//@TODO(Serge): once we use RenderMaterialParams - add other properties here

		}
		else
		{
			//TODO(Serge): assert here
		}

		if (entry.mType == r2::asset::TEXTURE)
		{
			r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*system.mMaterialTextures, entry.mIndex);

			if (!textures)
			{
				return;
			}

			const u64 numTextures = r2::sarr::Size(*textures);

			for (u64 i = 0; i < numTextures; ++i)
			{
				const auto& texture = r2::sarr::At(*textures, i);
				UpdateRenderMaterialDataTexture(system, materialIndex, texture);
			}
		}
		else
		{
			const r2::draw::tex::CubemapTexture& cubemap = r2::sarr::At(*system.mMaterialCubemapTextures, entry.mIndex);
			UpdateRenderMaterialDataTexture(system, materialIndex, cubemap);
		}
	}

	s32 GetPackingType(MaterialSystem& system, u64 materialIndex, tex::TextureType textureType)
	{
		const flat::MaterialParams* materialParams = system.mMaterialParamsPack->pack()->Get(materialIndex);

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

			//if (textureType = tex::TextureType::ClearCoat && texParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT)
			//{
			//	return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			//}

			//if (textureType == tex::TextureType::ClearCoatRoughness && texParam->propertyType() == flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS)
			//{
			//	return texParam->packingType() > flat::MaterialPropertyPackingType_A ? -1 : texParam->packingType();
			//}

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
		}

		return 0;
	}

	void UpdateRenderMaterialDataTexture(MaterialSystem& system, u64 materialIndex, const tex::Texture& texture)
	{
		if (!system.mInternalData)
		{
			R2_CHECK(false, "The system hasn't allocated the internal data!");
			return;
		}

		InternalMaterialData& internalMaterialData = r2::sarr::At(*system.mInternalData, materialIndex);

		if (texture.type == tex::TextureType::Diffuse)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.diffuseTexture);
			internalMaterialData.renderMaterial.albedo.texture = address;
			internalMaterialData.renderMaterial.albedo.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Normal)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.normalMapTexture);
			internalMaterialData.renderMaterial.normalMap.texture = address;
			internalMaterialData.renderMaterial.normalMap.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Emissive)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.emissionTexture);
			internalMaterialData.renderMaterial.emission.texture = address;
			internalMaterialData.renderMaterial.emission.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Metallic)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.metallicTexture);
			internalMaterialData.renderMaterial.metallic.texture = address;
			internalMaterialData.renderMaterial.metallic.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Anisotropy)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.anisotropyTexture);
			internalMaterialData.renderMaterial.anisotropy.texture = address;
			internalMaterialData.renderMaterial.anisotropy.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Occlusion)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.aoTexture);
			internalMaterialData.renderMaterial.ao.texture = address;
			internalMaterialData.renderMaterial.ao.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Height)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.heightTexture);
			internalMaterialData.renderMaterial.height.texture = address;
			internalMaterialData.renderMaterial.height.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
		else if (texture.type == tex::TextureType::Roughness)
		{
			auto address = r2::draw::texsys::GetTextureAddress(internalMaterialData.textureAssets.roughnessTexture);
			internalMaterialData.renderMaterial.roughness.texture = address;
			internalMaterialData.renderMaterial.roughness.texture.channel = GetPackingType(system, materialIndex, texture.type);
		}
	}

	void UpdateRenderMaterialDataTexture(MaterialSystem& system, u64 materialIndex, const tex::CubemapTexture& cubemap)
	{
		if (!system.mInternalData)
		{
			R2_CHECK(false, "The system hasn't allocated the internal data!");
			return;
		}

		InternalMaterialData* internalMaterialData = nullptr;
		internalMaterialData = &r2::sarr::At(*system.mInternalData, materialIndex);

		if (!internalMaterialData)
		{
			R2_CHECK(false, "We don't have internalMaterialData for material index: %llu!", materialIndex);
			return;
		}

		internalMaterialData->renderMaterial.albedo.texture = r2::draw::texsys::GetTextureAddress(cubemap);
	}

	void ClearRenderMaterialData(MaterialSystem& system)
	{
		const auto numMaterials = r2::sarr::Size(*system.mInternalData);

		for (u32 i = 0; i < numMaterials; ++i)
		{
			InternalMaterialData& internalMaterialData = r2::sarr::At(*system.mInternalData, i);
			internalMaterialData.renderMaterial = {};
		}
	}
}


namespace r2::draw::matsys
{

	s32 AddMaterialSystem(r2::draw::MaterialSystem* system)
	{
		u64 capacity = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);

		for (u64 i = 0; i < capacity; ++i)
		{
			if (s_optrMaterialSystems->mMaterialSystems->mData[i] == nullptr)
			{
				s_optrMaterialSystems->mMaterialSystems->mData[i] = system;
				system->mSlot = static_cast<s32>(i);
				return static_cast<s32>(i);
			}
		}

		R2_CHECK(false, "We couldn't find a spot for the system!");
		return -1;
	}

	void RemoveMaterialSystem(r2::draw::MaterialSystem* system)
	{
		u64 capacity = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);

		for (u64 i = 0; i < capacity; ++i)
		{
			if (s_optrMaterialSystems->mMaterialSystems->mData[i] == system)
			{
				s_optrMaterialSystems->mMaterialSystems->mData[i] = nullptr;
				return;
			}
		}
	}

	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numMaterialSystems, u64 maxMaterialsPerSystem, const char* systemName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(numMaterialSystems, maxMaterialsPerSystem, ALIGNMENT);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enought space to allocate the renderer!");
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, systemName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for the engine model area");
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* materialSystemsArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(materialSystemsArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");


		if (!materialSystemsArena)
		{
			R2_CHECK(materialSystemsArena != nullptr, "materialSystemsArena is null");
			return false;
		}

		s_optrMaterialSystems = ALLOC(r2::draw::mat::MaterialSystems, *materialSystemsArena);

		R2_CHECK(s_optrMaterialSystems != nullptr, "We couldn't allocate s_optrMaterialSystems!");

		

		s_optrMaterialSystems->mMemoryAreaHandle = memoryAreaHandle;
		s_optrMaterialSystems->mSubAreaHandle = subAreaHandle;
		s_optrMaterialSystems->mSystemsArena = materialSystemsArena;
		s_optrMaterialSystems->mMaterialSystems = MAKE_SARRAY(*materialSystemsArena, r2::draw::MaterialSystem*, numMaterialSystems);

		for (u64 i = 0; i < numMaterialSystems; ++i)
		{
			s_optrMaterialSystems->mMaterialSystems->mData[i] = nullptr;
		}

		s_optrMaterialSystems->mMaterialNamesToMaterialHandles = MAKE_SHASHMAP(*materialSystemsArena, r2::draw::MaterialHandle, numMaterialSystems * maxMaterialsPerSystem * r2::SHashMap<r2::draw::MaterialHandle>::LoadFactorMultiplier());
		s_optrMaterialSystems->mTextureNamesToMaterialHandles = MAKE_SHASHMAP(*materialSystemsArena, r2::draw::mat::TextureLookUpEntry, numMaterialSystems * maxMaterialsPerSystem * MAX_TEXTURES_PER_MATERIAL * r2::SHashMap<r2::draw::MaterialHandle>::LoadFactorMultiplier());



		return s_optrMaterialSystems->mMaterialSystems != nullptr;
	}

	MaterialSystem* CreateMaterialSystem(const r2::mem::utils::MemBoundary& boundary, const char* materialParamsPackPath, const char* texturePackManifestPath)
	{
		u64 unallocatedSpace = boundary.size;

		R2_CHECK(unallocatedSpace > 0 && boundary.location != nullptr, "We should have a valid boundary");

		//Emplace the linear arena in the subarea
		r2::mem::LinearArena* materialLinearArena = EMPLACE_LINEAR_ARENA_IN_BOUNDARY(boundary);

		if (!materialLinearArena)
		{
			R2_CHECK(materialLinearArena != nullptr, "linearArena is null");
			return false;
		}

		//Load the packs
		if (!materialParamsPackPath || strcmp(materialParamsPackPath, "") == 0)
		{
			R2_CHECK(false, "materialParamsPackPath is null or empty");
			return 0;
		}

		if (!texturePackManifestPath || strcmp(texturePackManifestPath, "") == 0)
		{
			R2_CHECK(false, "textureManifestPath is null or empty");
			return 0;
		}

		u64 materialParamsPackSize = 0;
		void* materialParamsPackData = r2::fs::ReadFile(*materialLinearArena, materialParamsPackPath, materialParamsPackSize);

		if (!materialParamsPackData)
		{
			R2_CHECK(false, "Failed to read the material params pack file: %s", materialParamsPackPath);
			return false;
		}

		u64 texturePacksSize = 0;
		void* texturePacksData = r2::fs::ReadFile(*materialLinearArena, texturePackManifestPath, texturePacksSize);
		if (!texturePacksData)
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", texturePackManifestPath);
			return false;
		}

		const flat::MaterialParamsPack* materialPack = flat::GetMaterialParamsPack(materialParamsPackData);
		R2_CHECK(materialPack != nullptr, "Why would this be null at this point? Problem in flatbuffers?");

		const flat::TexturePacksManifest* texturePack = flat::GetTexturePacksManifest(texturePacksData);
		R2_CHECK(texturePack != nullptr, "Why would this be null at this point? Problem in flatbuffers?");

		const auto memorySize = r2::draw::mat::MemorySize(16, materialPack->pack()->size(),
			texturePack->totalTextureSize(),
			texturePack->totalNumberOfTextures(),
			texturePack->texturePacks()->size(),
			texturePack->maxTexturesInAPack(), materialParamsPackSize, texturePacksSize);

		if (memorySize > unallocatedSpace)
		{
			R2_CHECK(false, "We don't have enough space to allocate a new sub area for this system");
			return false;
		}

		MaterialSystem* system = ALLOC(r2::draw::MaterialSystem, *materialLinearArena);

		R2_CHECK(system != nullptr, "We couldn't allocate the material system!");

		system->mLinearArena = materialLinearArena;
		
	//	system->mMaterials = MAKE_SARRAY(*materialLinearArena, r2::draw::Material, materialPack->pack()->size());
	//	R2_CHECK(system->mMaterials != nullptr, "we couldn't allocate the array for materials?");
		
		system->mTexturePacks = MAKE_SHASHMAP(*materialLinearArena, r2::draw::tex::TexturePack*, static_cast<u64>(static_cast<f64>(texturePack->texturePacks()->size() * HASH_MAP_BUFFER_MULTIPLIER)));
		R2_CHECK(system->mTexturePacks != nullptr, "we couldn't allocate the array for mTexturePacks?");

		system->mInternalData = MAKE_SARRAY(*materialLinearArena, InternalMaterialData, materialPack->pack()->size());
		R2_CHECK(system->mInternalData != nullptr, "We couldn't allocate the array for mInternalData");

		system->mMaterialMemBoundary = boundary;

		system->mMaterialParamsPackData = materialParamsPackData;
		system->mTexturePackManifestData = texturePacksData;

		system->mMaterialParamsPack = materialPack;
		system->mTexturePackManifest = texturePack;

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 assetCacheBoundarySize =
			r2::mem::utils::GetMaxMemoryForAllocation(
				r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking,
					texturePack->totalNumberOfTextures(), texturePack->totalTextureSize(), boundary.alignment),
				boundary.alignment, headerSize, boundsChecking);
		system->mCacheBoundary = MAKE_BOUNDARY(*materialLinearArena, assetCacheBoundarySize, boundary.alignment);

		u32 numNormalTexturePacks;
		u32 numCubemapTexturePacks;
		r2::asset::FileList fileList = r2::draw::mat::LoadTexturePacks(*system, texturePack, numNormalTexturePacks, numCubemapTexturePacks);

		system->mAssetCache = r2::asset::lib::CreateAssetCache(system->mCacheBoundary, fileList);

		AddMaterialSystem(system);

		mat::LoadAllMaterialParamsFromMaterialParamsPack(*system, materialPack, numNormalTexturePacks, numCubemapTexturePacks, fileList);


		return system;
	}

	void FreeMaterialSystem(MaterialSystem* system)
	{
		RemoveMaterialSystem(system);

		r2::draw::mat::UnloadAllMaterialTexturesFromGPU(*system);

		r2::mem::LinearArena* materialArena = system->mLinearArena;

		const s64 numMaterialTextures = static_cast<s64>(r2::sarr::Capacity(*system->mMaterialTextures));
		for (s64 i = numMaterialTextures - 1; i >= 0; --i)
		{
			r2::SArray<r2::draw::tex::Texture>* materialTextures = r2::sarr::At(*system->mMaterialTextures, i);
			if (materialTextures)
			{
				FREE(materialTextures, *materialArena);
			}
		}

		FREE(system->mMaterialTextures, *materialArena);
		FREE(system->mMaterialTextureEntries, *materialArena);
		if (system->mMaterialCubemapTextures)
		{
			FREE(system->mMaterialCubemapTextures, *materialArena);
		}
		
		r2::asset::lib::DestroyCache(system->mAssetCache);

		FREE(system->mCacheBoundary.location, *materialArena);

		//delete all the texture packs
		const s64 numTexturePacks = static_cast<s64>(r2::sarr::Capacity(*system->mTexturePacks->mData));
		for (s64 i = numTexturePacks - 1; i >= 0; --i)
		{
			r2::SHashMap<r2::draw::tex::TexturePack*>::HashMapEntry& entry = r2::sarr::At(*system->mTexturePacks->mData, i);
			if (entry.value != nullptr)
			{
				FREE_TEXTURE_PACK(*materialArena, entry.value);
			}
		}

		if (system->mTexturePackManifestData)
		{
			FREE(system->mTexturePackManifestData, *materialArena);
		}

		if (system->mMaterialParamsPackData)
		{
			FREE(system->mMaterialParamsPackData, *materialArena);
		}

		FREE(system->mTexturePacks, *materialArena);

		FREE(system->mInternalData, *materialArena);

	//	FREE(system->mMaterials, *materialArena);

		FREE(system, *materialArena);

		FREE_EMPLACED_ARENA(materialArena);
	}

	r2::draw::MaterialSystem* GetMaterialSystem(s32 slot)
	{
		if (!s_optrMaterialSystems || !s_optrMaterialSystems->mMaterialSystems)
		{
			return nullptr;
		}

		u64 numSystems = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);
		if (slot >= 0 && slot < numSystems)
		{
			return r2::sarr::At(*s_optrMaterialSystems->mMaterialSystems, slot);
		}

		return nullptr;
	}

	MaterialSystem* FindMaterialSystem(u64 materialName)
	{
		u64 capacity = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);

		for (u64 i = 0; i < capacity; ++i)
		{
			MaterialSystem* system = s_optrMaterialSystems->mMaterialSystems->mData[i];
			if (system != nullptr)
			{
				auto handle = mat::GetMaterialHandleFromMaterialName(*system, materialName);
				if (!mat::IsInvalidHandle(handle))
				{
					return system;
				}
			}
		}

		return nullptr;
	}

	MaterialHandle FindMaterialHandle(u64 materialName)
	{
		if (!s_optrMaterialSystems || !s_optrMaterialSystems->mMaterialNamesToMaterialHandles)
		{

			R2_CHECK(false, "We haven't initialized the Materials Systems!");
			return mat::InvalidMaterialHandle;
		}

		MaterialHandle defaultHandle = {};
		MaterialHandle result = r2::shashmap::Get(*s_optrMaterialSystems->mMaterialNamesToMaterialHandles, materialName, defaultHandle);

		if (result.handle == defaultHandle.handle || result.slot == defaultHandle.slot)
		{
		//	R2_CHECK(false, "We don't have the material handle for that material name!");
		}

		return result;
	}

	MaterialHandle FindMaterialFromTextureName(const char* textureName, r2::draw::tex::Texture& outTexture)
	{
		if (!textureName && strcmp(textureName, "") == 0)
		{
			R2_CHECK(false, "You passed in an invalid texture name!");
			return mat::InvalidMaterialHandle;
		}

		if (!s_optrMaterialSystems || !s_optrMaterialSystems->mTextureNamesToMaterialHandles)
		{
			R2_CHECK(false, "We haven't initialized the materials systems yet!");
			return mat::InvalidMaterialHandle;
		}

		r2::draw::mat::TextureLookUpEntry defaultEntry;

		r2::draw::mat::TextureLookUpEntry result = r2::shashmap::Get(*s_optrMaterialSystems->mTextureNamesToMaterialHandles, STRING_ID(textureName), defaultEntry);

		if (result.materialHandle.handle == defaultEntry.materialHandle.handle || result.materialHandle.slot == defaultEntry.materialHandle.slot)
		{
			return mat::InvalidMaterialHandle;
		}

		outTexture = result.texture;

		return result.materialHandle;
	}

	void Update()
	{
#ifdef R2_ASSET_PIPELINE
		
		std::string path;
		while (s_optrMaterialSystems && mat::s_texturesChangedQueue.TryPop(path))
		{
			//find which texture pack the path belongs to

			char fileName[r2::fs::FILE_PATH_LENGTH];
			char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(path.c_str(), sanitizedPath);
			r2::fs::utils::CopyFileNameWithParentDirectories(sanitizedPath, fileName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

			std::transform(std::begin(fileName), std::end(fileName), std::begin(fileName), (int(*)(int))std::tolower);


			std::filesystem::path filePath = sanitizedPath;

			
			
			u64 packName = STRING_ID(filePath.parent_path().parent_path().stem().string().c_str());


			r2::asset::AssetType assetType;
			MaterialSystem* foundSystem = nullptr;
			const u64 numMaterialSystems = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);
			for (u64 i = 0; i < numMaterialSystems; ++i)
			{
				MaterialSystem* system = r2::sarr::At(*s_optrMaterialSystems->mMaterialSystems, i);

				if (system)
				{
					r2::draw::tex::TexturePack* theDefault = nullptr;
					r2::draw::tex::TexturePack* result = r2::shashmap::Get(*system->mTexturePacks, packName, theDefault);
					if (result)
					{
						r2::asset::Asset asset(fileName, result->metaData.assetType);
						if (system->mAssetCache->HasAsset(asset))
						{
							assetType = result->metaData.assetType;
							foundSystem = system;
							break;
						}
					}
				}
			}

			//reload the path
			if (foundSystem != nullptr)
			{
				r2::asset::Asset asset(fileName, assetType);
				r2::asset::AssetHandle assetHandle = foundSystem->mAssetCache->ReloadAsset(asset);
				r2::draw::texsys::ReloadTexture(assetHandle);

				//@NOTE: since we have the same asset handle we don't need to do anything to the mMaterialTextures array
				//		 if that changes we'll have to add something here

				//Now we need to update the material's rendermaterial now that this has changed
				//first find it...
				char textureName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithExtension(sanitizedPath, textureName);
				tex::Texture foundTexture;

				MaterialHandle materialHandle = FindMaterialFromTextureName(textureName, foundTexture);

				if (!mat::IsInvalidHandle(materialHandle) && materialHandle.slot == foundSystem->mSlot)
				{
					auto materialIndex = mat::GetIndexFromMaterialHandle(materialHandle);

					const MaterialTextureEntry& entry = r2::sarr::At(*foundSystem->mMaterialTextureEntries, materialIndex);

					if (entry.mIndex == -1)
					{
						return;
					}

					if (entry.mType == r2::asset::TEXTURE)
					{
						mat::UpdateRenderMaterialDataTexture(*foundSystem, materialIndex, foundTexture);
					}
					else
					{
						const r2::draw::tex::CubemapTexture& cubemap = r2::sarr::At(*foundSystem->mMaterialCubemapTextures, entry.mIndex);

						mat::UpdateRenderMaterialDataTexture(*foundSystem, materialIndex, cubemap);
					}
				}
				
			}
		}
#endif // R2_ASSET_PIPELINE
	}

	u64 MemorySize(u64 numSystems, u64 maxMaterialsPerSystem, u64 alignment)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		return 
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::mat::MaterialSystems), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), alignment, headerSize, boundsChecking) + 
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialSystem*>::MemorySize(numSystems), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::draw::MaterialHandle>::MemorySize(numSystems * maxMaterialsPerSystem * r2::SHashMap<r2::draw::MaterialHandle>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::draw::mat::TextureLookUpEntry>::MemorySize(numSystems * maxMaterialsPerSystem * MAX_TEXTURES_PER_MATERIAL * r2::SHashMap<r2::draw::MaterialHandle>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking);
	}

	void ShutdownMaterialSystems()
	{
		r2::mem::LinearArena* arena = s_optrMaterialSystems->mSystemsArena;

		u64 numSystems = r2::sarr::Capacity(*s_optrMaterialSystems->mMaterialSystems);
		for (u64 i = 0; i < numSystems; ++i)
		{
			if (r2::sarr::At(*s_optrMaterialSystems->mMaterialSystems, i) != nullptr)
			{
				R2_CHECK(false, "Material system at slot: %llu has not been freed!", i);
			}
		}
		FREE(s_optrMaterialSystems->mMaterialNamesToMaterialHandles, *arena);
		FREE(s_optrMaterialSystems->mTextureNamesToMaterialHandles, *arena);
		FREE(s_optrMaterialSystems->mMaterialSystems, *arena);

		FREE(s_optrMaterialSystems, *arena);

		FREE_EMPLACED_ARENA(arena);
	}



#ifdef R2_ASSET_PIPELINE
	void TextureChanged(std::string texturePath)
	{
		mat::s_texturesChangedQueue.Push(texturePath);
	}
#endif // R2_ASSET_PIPELINE
}
