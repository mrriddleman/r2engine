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
	struct RenderMaterial
	{
		tex::TextureAddress diffuseTexture = {};
		tex::TextureAddress specularTexture = {};
		tex::TextureAddress normalMapTexture = {};
		tex::TextureAddress emissionTexture = {};

		glm::vec4 baseColor = glm::vec4(1.0f);
		float specular = 0.f;
		float roughness= 0.f;
		float metallic = 0.f;
	};

	struct Material
	{
		u64 materialID = 0;
		ShaderHandle shaderId = InvalidShader;
		u64 texturePackHandle = 0;
		
		tex::Texture diffuseTexture;
		tex::Texture specularTexture;
		tex::Texture normalMapTexture;
		tex::Texture emissionTexture;

		glm::vec4 baseColor = glm::vec4(1.0f);
		float specular;
		float roughness;
		float metallic;
	};

	struct MaterialTextureEntry
	{
		r2::asset::AssetType mType = r2::asset::TEXTURE; //TEXTURE OR CUBEMAP
		s64 mIndex = -1; //index into either mMaterialTextures or mCubemapTextures based on mType
	};

	struct MaterialSystem
	{
		r2::mem::utils::MemBoundary mMaterialMemBoundary = {};
		r2::mem::LinearArena* mLinearArena = nullptr;
		r2::SHashMap<r2::draw::tex::TexturePack*>* mTexturePacks = nullptr;
		r2::SArray<r2::draw::Material>* mMaterials = nullptr;

		r2::SArray<MaterialTextureEntry>* mMaterialTextureEntries = nullptr; //size should the number of materials in the pack
		r2::SArray<r2::SArray<r2::draw::tex::Texture>*>* mMaterialTextures = nullptr;
		r2::SArray<r2::draw::tex::CubemapTexture>* mMaterialCubemapTextures = nullptr;

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
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numMaterialSystems, u64 maxNumMaterialsPerSystem, const char* systemName);
	void Update();
	void ShutdownMaterialSystems();
	u64 MemorySize(u64 numSystems, u64 maxNumMaterialsPerSystem, u64 alignment);
	r2::draw::MaterialSystem* GetMaterialSystem(s32 slot);

	MaterialSystem* FindMaterialSystem(u64 materialName);
	MaterialHandle FindMaterialHandle(u64 materialName);
	MaterialHandle FindMaterialFromTextureName(const char* textureName, r2::draw::tex::Texture& outTexture);

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

	bool IsInvalidHandle(const MaterialHandle& materialHandle);

	//@TODO(Serge): add a progress function here
	void LoadAllMaterialTexturesFromDisk(MaterialSystem& system);
	//@TODO(Serge): add function to upload only 1 material?

	void UploadAllMaterialTexturesToGPU(const MaterialSystem& system);
	void UploadMaterialTexturesToGPUFromMaterialName(const MaterialSystem& system, u64 materialName);
	void UploadMaterialTexturesToGPU(const MaterialSystem& system, MaterialHandle matID);

	void UnloadAllMaterialTexturesFromGPU(const MaterialSystem& system);

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(const MaterialSystem& system, MaterialHandle matID);
	const r2::SArray<r2::draw::tex::CubemapTexture>* GetCubemapTextures(const MaterialSystem& system);


	MaterialHandle AddMaterial(MaterialSystem& system, const Material& mat);
	const Material* GetMaterial(const MaterialSystem& system, MaterialHandle matID);
	MaterialHandle GetMaterialHandleFromMaterialName(const MaterialSystem& system, u64 materialName);
	u64 MemorySize(u64 alignment, u64 capacity, u64 textureCacheInBytes, u64 numTextures, u64 numPacks, u64 maxTexturesInAPack);

	u64 LoadMaterialAndTextureManifests(const char* materialManifestPath, const char* textureManifestPath, void** materialPack, void** texturePacks);

}

#endif // !__MATERIAL_H__
