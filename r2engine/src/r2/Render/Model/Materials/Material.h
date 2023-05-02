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
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace flat
{
	struct MaterialPack;
	struct TexturePacksManifest;

	struct MaterialParamsPack;
	
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


	struct RenderMaterialParam
	{
		tex::TextureAddress texture;
		glm::vec4 color = glm::vec4(0); //for normal floats, we'll fill all the values with that value so when we access the channel, we get it the right value
	};

	struct RenderMaterialParams
	{
		RenderMaterialParam albedo;
		RenderMaterialParam normalMap;
		RenderMaterialParam emission;
		RenderMaterialParam metallic;
		RenderMaterialParam roughness;
		RenderMaterialParam ao;
		RenderMaterialParam height;
		RenderMaterialParam anisotropy;
		RenderMaterialParam detail;

		RenderMaterialParam clearCoat;
		RenderMaterialParam clearCoatRoughness;
		RenderMaterialParam clearCoatNormal;

		b32 doubleSided = false;
		f32 heightScale = 0.0f;
		f32 reflectance = 0.0f;
		s32 padding = 0;
	};

	//struct MaterialTextureEntry
	//{
	//	r2::asset::AssetType mType = r2::asset::TEXTURE; //TEXTURE OR CUBEMAP
	//	s64 mIndex = -1; //index into either mMaterialTextures or mCubemapTextures based on mType
	//};

	/*
		Diffuse = 0,
		Emissive,
		Normal,
		Metallic,
		Height,
		Roughness,
		Occlusion,
		Anisotropy,
		Detail,
		ClearCoat,
		ClearCoatRoughness,
		ClearCoatNormal,
		Cubemap,
	*/
	
	struct TextureAssets
	{
		tex::Texture diffuseTexture;
		tex::Texture emissionTexture;
		tex::Texture normalMapTexture;
		tex::Texture metallicTexture;
		tex::Texture heightTexture;
		tex::Texture roughnessTexture;
		tex::Texture aoTexture;
		tex::Texture anisotropyTexture;
		tex::Texture detailTexture;
		tex::Texture clearCoatTexture;
		tex::Texture clearCoatRoughnessTexture;
		tex::Texture clearCoatNormalTexture;
	};

	union MatTexUnion
	{
		TextureAssets materialTexture;
		tex::Texture textureTypes[tex::TextureType::Cubemap] = { tex::Texture{}, tex::Texture{}, tex::Texture{}, tex::Texture{}, tex::Texture{}, tex::Texture{},
																tex::Texture {}, tex::Texture{}, tex::Texture{}, tex::Texture{}, tex::Texture{}, tex::Texture{} };
	};

	struct MaterialTextureAssets
	{
		MatTexUnion normalTextures;
		tex::CubemapTexture cubemap; //may need more at some point
	};

	struct InternalMaterialData
	{
		u64 materialName = 0;
		ShaderHandle shaderHandle = InvalidShader;
		MaterialTextureAssets textureAssets;
		r2::draw::RenderMaterialParams renderMaterial = {};
		r2::asset::AssetType mType = r2::asset::TEXTURE;//TEXTURE OR CUBEMAP

	};

	struct MaterialSystem
	{
		r2::mem::utils::MemBoundary mMaterialMemBoundary = {};
		u64 mMaterialSystemName = 0;

#ifdef R2_ASSET_PIPELINE
		r2::mem::MallocArena* mLinearArena = nullptr;
#else
		r2::mem::LinearArena* mLinearArena = nullptr;
#endif

		r2::SHashMap<r2::draw::tex::TexturePack*>* mTexturePacks = nullptr;
		r2::SArray<InternalMaterialData>* mInternalData = nullptr; 

		char mMaterialPacksManifestFilePath[fs::FILE_PATH_LENGTH];
		char mTexturePacksManifestFilePath[fs::FILE_PATH_LENGTH];

		r2::asset::AssetCacheRecord mMaterialParamsPackData;
		r2::asset::AssetCacheRecord mTexturePackManifestData;

		const flat::MaterialParamsPack* mMaterialParamsPack = nullptr;
		const flat::TexturePacksManifest* mTexturePackManifest = nullptr;

		RenderMaterialParams mDefaultRenderMaterialParams;

		r2::asset::AssetCache* mAssetCache = nullptr;
		r2::mem::utils::MemBoundary mCacheBoundary = {};
		s32 mSlot = -1;
	};

	struct MaterialHandle
	{
		//forcing 8-byte alignment for free lists
		u64 handle = 0;
		s64 slot = -1;
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


	MaterialSystem* GetMaterialSystemBySystemName(u64 materialSystemName);

	MaterialSystem* FindMaterialSystem(u64 materialName);
	MaterialHandle FindMaterialHandle(u64 materialName);
	
	MaterialHandle FindMaterialFromTextureName(const char* textureName, r2::draw::tex::Texture& outTexture);

#ifdef R2_ASSET_PIPELINE
	//Find material functions

	MaterialSystem* FindMaterialSystemForMaterialPath(const std::string& materialPath);

	MaterialSystem* FindMaterialSystemForTexturePath(const std::string& texturePath);

	MaterialSystem* FindMaterialSystemForTextureManifestFilePath(const std::string& textureManifestFilePath);

	//Path changes functions
	void TextureChanged(const std::string& texturePath);

	void TexturePackAdded(const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsAdded);

	void TextureAdded(const std::string& texturePacksManifestFilePath, const std::string& texturePath);

	void TextureRemoved(const std::string& texturePacksManifestFilePath, const std::string& textureRemoved);
	void TexturePackRemoved(const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsLeft);


	void MaterialChanged(const std::string& materialPathChanged);

	void MaterialAdded(const std::string& materialPacksManifestFile, const std::string& materialPathAdded);

	void MaterialRemoved(const std::string& materialPacksManifestFile, const std::string& materialPathRemoved);

#endif // R2_ASSET_PIPELINE

	//creating/freeing a new material system
	MaterialSystem* CreateMaterialSystem(const r2::mem::utils::MemBoundary& boundary, const char* materialParamsPackPath, const char* texturePackManifestPath);
	MaterialSystem* CreateMaterialSystem(const r2::mem::utils::MemBoundary& boundary, const char* materialParamsPackPath, const flat::MaterialParamsPack* materialPack, u64 materialParamsPackSize, const char* texturePackManifestPath);
	void FreeMaterialSystem(MaterialSystem* system);
}

namespace r2::draw::mat
{
	static const MaterialHandle InvalidMaterial;

	bool IsInvalidHandle(const MaterialHandle& materialHandle);
	bool IsValid(const MaterialHandle& materialHandle);
	bool AreMaterialHandlesEqual(const MaterialHandle& materialHandle1, const MaterialHandle& materialHandle2);

	//@TODO(Serge): add a progress function here
	void LoadAllMaterialTexturesFromDisk(MaterialSystem& system);
	void LoadAllMaterialTexturesForMaterialFromDisk(MaterialSystem& system, const MaterialHandle& materialHandle);
	//@TODO(Serge): add function to upload only 1 material?

	void UnloadAllMaterialTexturesFromDisk(MaterialSystem& system);


	void UploadAllMaterialTexturesToGPU(MaterialSystem& system);
	void UploadMaterialTexturesToGPUFromMaterialName(MaterialSystem& system, u64 materialName);
	void UploadMaterialTexturesToGPU(MaterialSystem& system, MaterialHandle matID);
	void UnloadAllMaterialTexturesFromGPU(MaterialSystem& system);


	//const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(const MaterialSystem& system, MaterialHandle matID);
	//const r2::SArray<r2::draw::tex::CubemapTexture>* GetCubemapTextures(const MaterialSystem& system);

	const r2::draw::tex::CubemapTexture* GetCubemapTexture(const MaterialSystem& system, MaterialHandle matID);


//	MaterialHandle AddMaterial(MaterialSystem& system, const Material& mat);

//	const Material* GetMaterial(const MaterialSystem& system, MaterialHandle matID);
	ShaderHandle GetShaderHandle(const MaterialSystem& system, MaterialHandle matID);
	const RenderMaterialParams& GetRenderMaterial(const MaterialSystem& system, MaterialHandle matID);

	ShaderHandle GetShaderHandle(MaterialHandle matID);
	const RenderMaterialParams& GetRenderMaterial(MaterialHandle matID);

	MaterialHandle GetMaterialHandleFromMaterialName(const MaterialSystem& system, u64 materialName);
	MaterialHandle GetMaterialHandleFromMaterialPath(const MaterialSystem& system, const char* materialPath);

	const MaterialTextureAssets& GetMaterialTextureAssetsForMaterial(const MaterialSystem& system, MaterialHandle materialHandle);

	u64 MemorySize(u64 alignment, u64 numMaterials, u64 textureCacheInBytes, u64 numTextures, u64 numPacks, u64 maxTexturesInAPack, u32 materialParamsFileSize = 0, u32 texturePacksManifestFileSize = 0);

	u64 GetMaterialBoundarySize(const char* materialManifestPath, const char* textureManifestPath);
}

#endif // !__MATERIAL_H__
