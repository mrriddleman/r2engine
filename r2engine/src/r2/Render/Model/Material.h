#ifndef __MATERIAL_H__
#define __MATERIAL_H__


#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
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

#endif // !__MATERIAL_H__
