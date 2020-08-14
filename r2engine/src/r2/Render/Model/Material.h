#ifndef __MATERIAL_H__
#define __MATERIAL_H__


#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"


namespace flat
{
	struct MaterialPack;
	struct TexturePacksManifest;
}

namespace r2::draw::tex
{
	struct TexturePack;
}

namespace r2::asset
{
	class AssetCache;
}

namespace r2::draw
{
	struct Material
	{
		u64 materialID = 0;
		ShaderHandle shaderId = InvalidShader;
		u64 texturePackHandle = 0;
		glm::vec4 color = glm::vec4(1.0f);
	};

	struct MaterialSystem
	{
		r2::mem::utils::MemBoundary mMaterialMemBoundary = {};
		r2::mem::LinearArena* mLinearArena = nullptr;
		r2::SHashMap<r2::draw::tex::TexturePack*>* mTexturePacks = nullptr; //maybe not even needed? - I think if we can get rid of the asset path we can get rid of this?
		r2::SArray<r2::draw::Material>* mMaterials = nullptr;
		r2::SArray<r2::SArray<r2::draw::tex::Texture>*>* mMaterialTextures = nullptr;
		r2::asset::AssetCache* mAssetCache = nullptr;
		r2::mem::utils::MemBoundary mCacheBoundary = {};
		s32 mSlot = -1;
	};

	struct MaterialHandle
	{
		u32 handle = 0;
		s32 slot = -1;
	};

	

	struct ModelMaterial
	{
		union
		{
			tex::TextureAddress textures[8];

			struct
			{
				tex::TextureAddress diffuse1;
				tex::TextureAddress specular1;
				tex::TextureAddress emissive1;
				tex::TextureAddress normal1;
				tex::TextureAddress metallic1;
				tex::TextureAddress height1;
				tex::TextureAddress microfacet1;
				tex::TextureAddress occlusion1;
			};
		};
	};
}

namespace r2::draw::matsys
{
	//for the matsys
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numMaterialSystems, const char* systemName);
	void Update();
	void ShutdownMaterialSystems();
	u64 MemorySize(u64 numSystems, u64 alignment);
	r2::draw::MaterialSystem* GetMaterialSystem(s32 slot);

#ifdef R2_ASSET_PIPELINE
	void TextureChanged(std::string texturePath);
#endif // R2_ASSET_PIPELINE

	//creating/freeing a new material system
	MaterialSystem* CreateMaterialSystem(const r2::mem::utils::MemBoundary& boundary, const flat::MaterialPack* materialPack, const flat::TexturePacksManifest* texturePackManifest);
	void FreeMaterialSystem(MaterialSystem* system);

	template<class ARENA>
	MaterialSystem* CreateMaterialSystem(ARENA& arena, const char* materialPackPath, const char* texturePackManifestPath);

	template<class ARENA>
	void FreeMaterialSystem(ARENA& arena, MaterialSystem* system);
}

namespace r2::draw::mat
{
	static const MaterialHandle InvalidMaterial;

	//@TODO(Serge): add a progress function here
	void LoadAllMaterialTexturesFromDisk(MaterialSystem& system);
	//@TODO(Serge): add function to upload only 1 material?

	void UploadAllMaterialTexturesToGPU(const MaterialSystem& system);
	void UploadMaterialTexturesToGPUFromMaterialName(const MaterialSystem& system, u64 materialName);
	void UploadMaterialTexturesToGPU(const MaterialSystem& system, MaterialHandle matID);

	void UnloadAllMaterialTexturesFromGPU(const MaterialSystem& system);

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(const MaterialSystem& system, MaterialHandle matID);
	MaterialHandle AddMaterial(MaterialSystem& system, const Material& mat);
	const Material* GetMaterial(const MaterialSystem& system, MaterialHandle matID);
	MaterialHandle GetMaterialHandleFromMaterialName(const MaterialSystem& system, u64 materialName);
	u64 MemorySize(u64 alignment, u64 capacity, u64 textureCacheInBytes, u64 numTextures, u64 numPacks, u64 maxTexturesInAPack);
}

namespace r2::draw::matsys
{
	template<class ARENA>
	MaterialSystem* CreateMaterialSystem(ARENA& arena, const char* materialPackPath, const char* texturePackManifestPath)
	{
		void* materialPackData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, materialPackPath);
		if (!materialPackData)
		{
			R2_CHECK(false, "Failed to read the material pack file: %s", materialPackPath);
			return false;
		}

		const flat::MaterialPack* materialPack = flat::GetMaterialPack(materialPackData);

		R2_CHECK(materialPack != nullptr, "Failed to get the material pack from the data!");

		void* texturePacksData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, texturePackManifestPath);
		if (!texturePacksData)
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", texturePackManifestPath);
			return false;
		}

		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(texturePacksData);

		R2_CHECK(texturePacksManifest != nullptr, "Failed to get the material pack from the data!");

		u64 materialMemorySystemSize = MaterialSystemMemorySize(materialPack->materials()->size(),
			texturePacksManifest->totalTextureSize(),
			texturePacksManifest->totalNumberOfTextures(),
			texturePacksManifest->texturePacks()->size(),
			texturePacksManifest->maxTexturesInAPack());

		r2::mem::utils::MemBoundary boundary = MAKE_BOUNDARY(arena, materialMemorySystemSize, 16);


		//Here we should be loading the material pack
		u64 unallocatedSpace = boundary.size;
		u64 memoryNeeded = r2::draw::mat::MemorySize(boundary.alignment, materialPack->materials()->size(),
			texturePacksManifest->totalTextureSize(),
			texturePacksManifest->totalNumberOfTextures(),
			texturePacksManifest->texturePacks()->size(),
			texturePacksManifest->maxTexturesInAPack());

		if (memoryNeeded > unallocatedSpace)
		{
			R2_CHECK(false, "We don't have enough space to allocate a new sub area for this system");
			return nullptr;
		}

		//Emplace the linear arena in the subarea
		r2::mem::LinearArena* materialLinearArena = EMPLACE_LINEAR_ARENA_IN_BOUNDARY(boundary);

		if (!materialLinearArena)
		{
			R2_CHECK(materialLinearArena != nullptr, "linearArena is null");
			return nullptr;
		}

		//allocate the MemorySystem
		MaterialSystem* system = ALLOC(r2::draw::MaterialSystem, *materialLinearArena);

		R2_CHECK(system != nullptr, "We couldn't allocate the material system!");

		system->mLinearArena = materialLinearArena;
		system->mMaterials = MAKE_SARRAY(*materialLinearArena, r2::draw::Material, materialPack->materials()->size());
		R2_CHECK(system->mMaterials != nullptr, "we couldn't allocate the array for materials?");
		system->mTexturePacks = MAKE_SHASHMAP(*materialLinearArena, r2::draw::tex::TexturePack*, static_cast<u64>(static_cast<f64>(texturePacksManifest->texturePacks()->size() * HASH_MAP_BUFFER_MULTIPLIER)));
		R2_CHECK(system->mTexturePacks != nullptr, "we couldn't allocate the array for mTexturePacks?");
		system->mMaterialMemBoundary = boundary;

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 assetCacheBoundarySize =
			r2::mem::utils::GetMaxMemoryForAllocation(
				r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking,
					texturePacksManifest->totalNumberOfTextures(), texturePacksManifest->totalTextureSize(), boundary.alignment),
				boundary.alignment, headerSize, boundsChecking);
		system->mCacheBoundary = MAKE_BOUNDARY(*materialLinearArena, assetCacheBoundarySize, boundary.alignment);

		r2::asset::FileList fileList = r2::draw::mat::LoadTexturePacks(*system, texturePacksManifest);

		system->mAssetCache = r2::asset::lib::CreateAssetCache(system->mCacheBoundary, fileList);

		r2::draw::mat::LoadAllMaterialsFromMaterialPack(*system, materialPack);

		s32 slot = AddMaterialSystem(system);
		system->mSlot = slot;

		FREE(texturePacksData, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPackData, *MEM_ENG_SCRATCH_PTR);

		return system;
	}

	template<class ARENA>
	void FreeMaterialSystem(ARENA& arena, MaterialSystem* system)
	{
		void* boundaryLocation = system->mMaterialMemBoundary.location;

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

		FREE(system->mTexturePacks, *materialArena);

		FREE(system->mMaterials, *materialArena);

		FREE(system, *materialArena);

		FREE_EMPLACED_ARENA(materialArena);

		FREE(boundaryLocation, arena);
	}
}


#endif // !__MATERIAL_H__
