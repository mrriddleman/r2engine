#include "r2pch.h"
#include "r2/Render/Model/Material.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Material_generated.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Render/Model/Textures/TexturePack.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Render/Renderer/ShaderSystem.h"

namespace r2::draw
{
	struct MaterialSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mLinearArena = nullptr;
		r2::SHashMap<r2::draw::tex::TexturePack*>* mTexturePacks = nullptr; //maybe not even needed? - I think if we can get rid of the asset path we can get rid of this?
		r2::SArray<r2::draw::Material>* mMaterials = nullptr;
		r2::SArray<r2::SArray<r2::draw::tex::Texture>*>* mMaterialTextures = nullptr;
		r2::asset::AssetCache* mAssetCache = nullptr;
		r2::mem::utils::MemBoundary mCacheBoundary;
	};
}

namespace
{
	r2::draw::MaterialSystem* s_optrMaterialSystem = nullptr;
	const u64 ALIGNMENT = 16;
	const u32 MAX_TEXTURES_PER_MATERIAL = 32;
	const u64 MAX_TEXTURE_PACKS = 1024;
	const u64 TEXTURE_PACK_CAPCITY = 1024;
	const f64 HASH_MAP_BUFFER_MULTIPLIER = 1.5f;
	const u32 NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME = 2;
	const u64 MIN_TEXTURE_CAPACITY = 8;
}

namespace r2::draw::mat
{
	const MaterialHandle InvalidMaterialHandle = 0;

	r2::asset::FileList LoadTexturePacks(const flat::TexturePacksManifest* texturePacksManifest);
	void LoadAllMaterialsFromMaterialPack(const flat::MaterialPack* materialPack);
	void UploadMaterialTexturesToGPUInternal(u64 index);

	ShaderHandle MakeMaterialHandleFromIndex(u64 index)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "material system hasn't been initialized yet!");
			return InvalidMaterialHandle;
		}

		const u64 numMaterials = r2::sarr::Size(*s_optrMaterialSystem->mMaterials);
		if (index >= numMaterials)
		{
			R2_CHECK(false, "the index is greater than the number of materials we have!");
			return InvalidMaterialHandle;
		}

		return static_cast<MaterialHandle>(index + 1);
	}

	u64 GetIndexFromMaterialHandle(MaterialHandle handle)
	{
		if (handle == InvalidMaterialHandle)
		{
			R2_CHECK(false, "You passed in an invalid material handle!");
			return 0;
		}

		return static_cast<u64>(handle - 1);
	}

	//@TODO(Serge): change this so it's not a separate memory area - you probably just want to give this a memory boundary instead
	//@TODO(Serge): It's weird to have two different things to load (material pack and the texture pack) but yeah I dunno a better way right now
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, const char* materialPackPath, const char* texturePacksManifestPath)
	{
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Memory Area handle is invalid");
		R2_CHECK(s_optrMaterialSystem == nullptr, "Are you trying to initialize this system more than once?");

		if (memoryAreaHandle == r2::mem::MemoryArea::Invalid ||
			s_optrMaterialSystem != nullptr)
		{
			return false;
		}

		//Get the memory area
		r2::mem::MemoryArea* noptrMemArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		R2_CHECK(noptrMemArea != nullptr, "noptrMemArea is null?");
		if (!noptrMemArea)
		{
			return false;
		}

		//Here we should be loading the material pack
		void* materialPackData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, materialPackPath);
		if (!materialPackData)
		{
			R2_CHECK(false, "Failed to read the material pack file: %s", materialPackPath);
			return false;
		}
			
		const flat::MaterialPack* materialPack = flat::GetMaterialPack(materialPackData);

		R2_CHECK(materialPack != nullptr, "Failed to get the material pack from the data!");

		void* texturePacksData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, texturePacksManifestPath);
		if (!texturePacksData)
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", texturePacksManifestPath);
			return false;
		}

		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(texturePacksData);

		R2_CHECK(texturePacksManifest != nullptr, "Failed to get the material pack from the data!");

		u64 unallocatedSpace = noptrMemArea->UnAllocatedSpace();
		u64 memoryNeeded = MemorySize(materialPack->materials()->size(),
			texturePacksManifest->totalTextureSize(),
			texturePacksManifest->totalNumberOfTextures(),
			texturePacksManifest->texturePacks()->size(),
			texturePacksManifest->maxTexturesInAPack());

		if (memoryNeeded > unallocatedSpace)
		{
			R2_CHECK(false, "We don't have enough space to allocate a new sub area for this system");
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = noptrMemArea->AddSubArea(memoryNeeded, "Material System");

		R2_CHECK(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "We have an invalid sub area");

		if (subAreaHandle == r2::mem::MemoryArea::SubArea::Invalid)
		{
			return false;
		}

		r2::mem::MemoryArea::SubArea* noptrSubArea = noptrMemArea->GetSubArea(subAreaHandle);
		R2_CHECK(noptrSubArea != nullptr, "noptrSubArea is null");
		if (!noptrSubArea)
		{
			return false;
		}

		//Emplace the linear arena in the subarea
		r2::mem::LinearArena* materialLinearArena = EMPLACE_LINEAR_ARENA(*noptrSubArea);
		
		if (!materialLinearArena)
		{
			R2_CHECK(materialLinearArena != nullptr, "linearArena is null");
			return false;
		}

		//allocate the MemorySystem
		s_optrMaterialSystem = ALLOC(r2::draw::MaterialSystem, *materialLinearArena);

		R2_CHECK(s_optrMaterialSystem != nullptr, "We couldn't allocate the material system!");

		s_optrMaterialSystem->mMemoryAreaHandle = memoryAreaHandle;
		s_optrMaterialSystem->mSubAreaHandle = subAreaHandle;
		s_optrMaterialSystem->mLinearArena = materialLinearArena;
		s_optrMaterialSystem->mMaterials = MAKE_SARRAY(*materialLinearArena, r2::draw::Material, materialPack->materials()->size());
		R2_CHECK(s_optrMaterialSystem->mMaterials != nullptr, "we couldn't allocate the array for materials?");
		s_optrMaterialSystem->mTexturePacks = MAKE_SHASHMAP(*materialLinearArena, r2::draw::tex::TexturePack*, static_cast<u64>(static_cast<f64>(texturePacksManifest->texturePacks()->size() * HASH_MAP_BUFFER_MULTIPLIER)));
		R2_CHECK(s_optrMaterialSystem->mTexturePacks != nullptr, "we couldn't allocate the array for mTexturePacks?");

		
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 assetCacheBoundarySize =
			r2::mem::utils::GetMaxMemoryForAllocation(
				r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking,
					texturePacksManifest->totalNumberOfTextures(), texturePacksManifest->totalTextureSize(), ALIGNMENT),
				ALIGNMENT, headerSize, boundsChecking);
		s_optrMaterialSystem->mCacheBoundary = MAKE_BOUNDARY(*materialLinearArena, assetCacheBoundarySize, ALIGNMENT);

		r2::asset::FileList fileList = LoadTexturePacks(texturePacksManifest);

		s_optrMaterialSystem->mAssetCache = r2::asset::lib::CreateAssetCache(s_optrMaterialSystem->mCacheBoundary, fileList);

		LoadAllMaterialsFromMaterialPack(materialPack);

		FREE(texturePacksData, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPackData, *MEM_ENG_SCRATCH_PTR);

		return s_optrMaterialSystem->mMaterials != nullptr;
	}

	void LoadAllMaterialTexturesFromDisk()
	{
		if (!s_optrMaterialSystem ||
			!s_optrMaterialSystem->mMaterialTextures ||
			!s_optrMaterialSystem->mTexturePacks)
		{
			R2_CHECK(false, "Material system has not been initialized!");
			return;
		}

		const u64 numMaterials = r2::sarr::Size(*s_optrMaterialSystem->mMaterials);
		for(u64 i = 0; i < numMaterials; ++i)
		{
			r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*s_optrMaterialSystem->mMaterialTextures, i);

			if (textures) //we already have them loaded - or have loaded before
				continue;

			const Material& material = r2::sarr::At(*s_optrMaterialSystem->mMaterials, i);
			//We may want to move this to the load from disk as that's what this is basically for...
			r2::draw::tex::TexturePack* defaultTexturePack = nullptr;
			r2::draw::tex::TexturePack* result = r2::shashmap::Get(*s_optrMaterialSystem->mTexturePacks, material.texturePackHandle, defaultTexturePack);

			if (result == defaultTexturePack)
			{
				R2_CHECK(false, "We couldn't get the texture pack!");
				return;
			}

			r2::SArray<r2::draw::tex::Texture>* materialTextures = MAKE_SARRAY(*s_optrMaterialSystem->mLinearArena, r2::draw::tex::Texture, result->totalNumTextures);

			R2_CHECK(materialTextures != nullptr, "We couldn't allocate the array for the material textures");

			const auto numAlbedos = r2::sarr::Size(*result->albedos);
			for (u64 t = 0; t < numAlbedos; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Diffuse;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->albedos, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numEmissives = r2::sarr::Size(*result->emissives);
			for (u64 t = 0; t < numEmissives; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Emissive;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->emissives, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numHeights = r2::sarr::Size(*result->heights);
			for (u64 t = 0; t < numHeights; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Height;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->heights, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numMetalics = r2::sarr::Size(*result->metalics);
			for (u64 t = 0; t < numMetalics; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Metallic;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->metalics, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numMicros = r2::sarr::Size(*result->micros);
			for (u64 t = 0; t < numMicros; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::MicroFacet;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->micros, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numNormals = r2::sarr::Size(*result->normals);
			for (flatbuffers::uoffset_t t = 0; t < numNormals; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Normal;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->normals, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numOcclusions = r2::sarr::Size(*result->occlusions);
			for (flatbuffers::uoffset_t t = 0; t < numOcclusions; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Occlusion;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->occlusions, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			const auto numSpeculars = r2::sarr::Size(*result->speculars);
			for (flatbuffers::uoffset_t t = 0; t < numSpeculars; ++t)
			{
				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Specular;
				texture.textureAssetHandle =
					s_optrMaterialSystem->mAssetCache->LoadAsset(r2::sarr::At(*result->speculars, t));
				r2::sarr::Push(*materialTextures, texture);
			}

			s_optrMaterialSystem->mMaterialTextures->mData[i] = materialTextures;
		}
	}

	void UploadAllMaterialTexturesToGPU()
	{
		const u64 capacity = r2::sarr::Capacity(*s_optrMaterialSystem->mMaterialTextures);

		for (u64 i = 0; i < capacity; ++i)
		{
			UploadMaterialTexturesToGPUInternal(i);
		}
	}

	void UploadMaterialTexturesToGPUFromMaterialName(u64 materialName)
	{
		UploadMaterialTexturesToGPU(GetMaterialHandleFromMaterialName(materialName));
	}

	void UploadMaterialTexturesToGPU(MaterialHandle matID)
	{
		u64 index = GetIndexFromMaterialHandle(matID);
		UploadMaterialTexturesToGPUInternal(index);
	}

	void UploadMaterialTexturesToGPUInternal(u64 index)
	{
		r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*s_optrMaterialSystem->mMaterialTextures, index);

		if (!textures)
		{
			return;
		}

		const u64 numTextures = r2::sarr::Size(*textures);

		for (u64 i = 0; i < numTextures; ++i)
		{
			r2::draw::texsys::UploadToGPU(r2::sarr::At(*textures, i).textureAssetHandle);
		}
	}

	void UnloadAllMaterialTexturesFromGPU()
	{
		const u64 capacity = r2::sarr::Capacity(*s_optrMaterialSystem->mMaterialTextures);

		for (u64 i = 0; i < capacity; ++i)
		{
			r2::SArray<r2::draw::tex::Texture>* textures = r2::sarr::At(*s_optrMaterialSystem->mMaterialTextures, i);

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
	}

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(MaterialHandle matID)
	{
		if (matID == InvalidMaterialHandle)
		{
			R2_CHECK(false, "Passed in an invalid material ID");
			return nullptr;
		}

		const r2::SArray<r2::draw::tex::Texture>* materialTextures =
			r2::sarr::At(*s_optrMaterialSystem->mMaterialTextures, GetIndexFromMaterialHandle(matID));

		return materialTextures;
	}

	MaterialHandle AddMaterial(const Material& mat)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return InvalidMaterialHandle;
		}

		//see if we already have this material - if we do then return that
		const u64 numMaterials = r2::sarr::Size(*s_optrMaterialSystem->mMaterials);

		for (u64 i = 0; i < numMaterials; ++i)
		{
			const Material& nextMaterial = r2::sarr::At(*s_optrMaterialSystem->mMaterials, i);
			if (mat.materialID == nextMaterial.materialID)
			{
				return MakeMaterialHandleFromIndex(i);
			}
		}

		r2::sarr::Push(*s_optrMaterialSystem->mMaterials, mat);
		return MakeMaterialHandleFromIndex(numMaterials);
	}

	const Material* GetMaterial(MaterialHandle matID)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return nullptr;
		}

		if (matID == InvalidMaterialHandle)
		{
			R2_CHECK(false, "You passed in an invalid material handle");
			return nullptr;
		}

		u64 materialIndex = GetIndexFromMaterialHandle(matID);

		if (materialIndex >= r2::sarr::Size(*s_optrMaterialSystem->mMaterials))
		{
			return nullptr;
		}

		return &r2::sarr::At(*s_optrMaterialSystem->mMaterials, materialIndex);
	}

	MaterialHandle GetMaterialHandleFromMaterialName(u64 materialName)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return InvalidMaterialHandle;
		}

		//see if we already have this material - if we do then return that
		const u64 numMaterials = r2::sarr::Size(*s_optrMaterialSystem->mMaterials);

		for (u64 i = 0; i < numMaterials; ++i)
		{
			const Material& nextMaterial = r2::sarr::At(*s_optrMaterialSystem->mMaterials, i);
			if (materialName == nextMaterial.materialID)
			{
				return MakeMaterialHandleFromIndex(i);
			}
		}

		return InvalidMaterialHandle;
	}

	void Shutdown()
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return;
		}

		r2::mem::LinearArena* materialArena = s_optrMaterialSystem->mLinearArena;

		const s64 numMaterialTextures = static_cast<s64>(r2::sarr::Capacity(*s_optrMaterialSystem->mMaterialTextures));
		for (s64 i = numMaterialTextures-1; i >= 0; --i)
		{
			r2::SArray<r2::draw::tex::Texture>* materialTextures = r2::sarr::At(*s_optrMaterialSystem->mMaterialTextures, i);
			if (materialTextures)
			{
				FREE(materialTextures, *materialArena);
			}
		}

		FREE(s_optrMaterialSystem->mMaterialTextures, *materialArena);

		r2::asset::lib::DestroyCache(s_optrMaterialSystem->mAssetCache);
		
		FREE(s_optrMaterialSystem->mCacheBoundary.location, *materialArena);

		//delete all the texture packs
		const s64 numTexturePacks = static_cast<s64>(r2::sarr::Capacity(*s_optrMaterialSystem->mTexturePacks->mData));
		for (s64 i = numTexturePacks - 1; i >= 0; --i)
		{
			r2::SHashMap<r2::draw::tex::TexturePack*>::HashMapEntry& entry = r2::sarr::At(*s_optrMaterialSystem->mTexturePacks->mData, i);
			if (entry.value != nullptr)
			{
				FREE_TEXTURE_PACK(*materialArena, entry.value);
			}
		}

		FREE(s_optrMaterialSystem->mTexturePacks, *materialArena);

		FREE(s_optrMaterialSystem->mMaterials, *materialArena);
		
		FREE(s_optrMaterialSystem, *materialArena);
		s_optrMaterialSystem = nullptr;

		FREE_EMPLACED_ARENA(materialArena);
	}

	u64 MemorySize(u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack)
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
		u64 memSize1 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize2 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::MaterialSystem), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize3 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::Material>::MemorySize(numMaterials), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize4 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::SArray<r2::draw::tex::Texture>*>::MemorySize(numMaterials), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize5 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::tex::Texture>::MemorySize(maxTexturesInAPack), ALIGNMENT, headerSize, boundsChecking) * numMaterials;
		u64 memSize6 = r2::mem::utils::GetMaxMemoryForAllocation(r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, totalNumberOfTextures, textureCacheInBytes, ALIGNMENT), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize7 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::draw::tex::TexturePack*>::MemorySize(hashCapacity), ALIGNMENT, headerSize, boundsChecking);

		u64 t = r2::draw::tex::TexturePackMemorySize(maxTexturesInAPack, ALIGNMENT, headerSize, boundsChecking);
		u64 memSize8 = r2::mem::utils::GetMaxMemoryForAllocation(t, ALIGNMENT, headerSize, boundsChecking) * numPacks;

		memorySize = memSize1 + memSize2 + memSize3 + memSize4 + memSize5 + memSize6 + memSize7 + memSize8;

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, ALIGNMENT);
	}


	void LoadAllMaterialsFromMaterialPack(const flat::MaterialPack* materialPack)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system!");
			return;
		}

		R2_CHECK(materialPack != nullptr, "Material pack is nullptr");

		const auto numMaterials = materialPack->materials()->size();

		s_optrMaterialSystem->mMaterialTextures = MAKE_SARRAY(*s_optrMaterialSystem->mLinearArena, r2::SArray<r2::draw::tex::Texture>*, numMaterials);

		R2_CHECK(s_optrMaterialSystem->mMaterialTextures != nullptr, "We couldn't allocate the material textures!");


		for (u64 i = 0; i < numMaterials; ++i)
		{
			s_optrMaterialSystem->mMaterialTextures->mData[i] = nullptr;
		}

		for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
		{
			const flat::Material* material = materialPack->materials()->Get(i);

			r2::draw::Material newMaterial;
			
			newMaterial.materialID = material->name();
			newMaterial.color = glm::vec4(material->color()->r(), material->color()->g(), material->color()->b(), material->color()->a());
			
			//@NOTE: So if we need this to change for example we don't have the shaders setup yet -
			//instead just set the shader name and have the look up be later when we bind the shader
			//just like we do with the texture pack handle
			newMaterial.shaderId = r2::draw::shadersystem::FindShaderHandle(material->shader());

			newMaterial.texturePackHandle = material->texturePackName();

			MaterialHandle handle = AddMaterial(newMaterial);
			R2_CHECK(handle != InvalidMaterialHandle, "We couldn't add the new material?");
		}
	}

	r2::asset::FileList LoadTexturePacks(const flat::TexturePacksManifest* texturePacksManifest)
	{
		const flatbuffers::uoffset_t numTexturePacks = texturePacksManifest->texturePacks()->size();

		u32 maxNumTextures = 0;

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const flat::TexturePack* nextPack = texturePacksManifest->texturePacks()->Get(i);

			maxNumTextures = std::max({
				maxNumTextures,
				nextPack->albedo()->size(),
				nextPack->emissive()->size(),
				nextPack->height()->size(),
				nextPack->metalic()->size(),
				nextPack->micro()->size(),
				nextPack->normal()->size(),
				nextPack->occlusion()->size(),
				nextPack->specular()->size()});
		}

		R2_CHECK(maxNumTextures <= TEXTURE_PACK_CAPCITY, "We're not allocating enough space for the texture pack capacity!");

		r2::asset::FileList list = r2::asset::lib::MakeFileList(texturePacksManifest->totalNumberOfTextures());

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const flat::TexturePack* nextPack = texturePacksManifest->texturePacks()->Get(i);

			r2::draw::tex::TexturePack* texturePack = MAKE_TEXTURE_PACK(*s_optrMaterialSystem->mLinearArena, maxNumTextures);

			R2_CHECK(texturePack != nullptr, "Failed to allocate the texture pack!");

			texturePack->totalNumTextures = nextPack->totalNumberOfTextures();

			const auto numAlbedos = nextPack->albedo()->size();
			for (flatbuffers::uoffset_t t = 0; t < numAlbedos; ++t)
			{
				auto nextTexturePath = nextPack->albedo()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName);
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
				r2::asset::Asset textureAsset(assetName);
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
				r2::asset::Asset textureAsset(assetName);
				r2::sarr::Push(*texturePack->heights, textureAsset);
			}

			const auto numMetalics = nextPack->metalic()->size();
			for (flatbuffers::uoffset_t t = 0; t < numMetalics; ++t)
			{
				auto nextTexturePath = nextPack->metalic()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName);
				r2::sarr::Push(*texturePack->metalics, textureAsset);
			}

			const auto numMicros = nextPack->micro()->size();
			for (flatbuffers::uoffset_t t = 0; t < numMicros; ++t)
			{
				auto nextTexturePath = nextPack->micro()->Get(t);

				r2::asset::RawAssetFile* nextFile = r2::asset::lib::MakeRawAssetFile(nextTexturePath->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);

				r2::sarr::Push(*list, (r2::asset::AssetFile*)nextFile);

				char assetName[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::CopyFileNameWithParentDirectories(nextTexturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
				r2::asset::Asset textureAsset(assetName);
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
				r2::asset::Asset textureAsset(assetName);
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
				r2::asset::Asset textureAsset(assetName);
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
				r2::asset::Asset textureAsset(assetName);
				r2::sarr::Push(*texturePack->speculars, textureAsset);
			}

			texturePack->packName = nextPack->packName();

			r2::shashmap::Set(*s_optrMaterialSystem->mTexturePacks, nextPack->packName(), texturePack);
		}

		return list;
	}
}