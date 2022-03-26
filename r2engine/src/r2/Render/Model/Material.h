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
		glm::vec4 color; //for normal floats, we'll fill all the values with that value so when we access the channel, we get it the right value
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

		b32 doubleSided;
		f32 heightScale;
		f32 reflectance;
		s32 padding;
	};

	//struct RenderMaterial
	//{
	//	tex::TextureAddress diffuseTexture = {};
	//	tex::TextureAddress specularTexture = {};
	//	tex::TextureAddress normalMapTexture = {};
	//	tex::TextureAddress emissionTexture = {};
	//	tex::TextureAddress metallicTexture = {};
	//	tex::TextureAddress roughnessTexture = {};
	//	tex::TextureAddress aoTexture = {};
	//	tex::TextureAddress heightTexture = {};
	//	tex::TextureAddress anisotropyTexture = {};

	//	glm::vec3 baseColor = glm::vec3(1.0f);
	//	float specular = 0.f;
	//	float roughness= 0.f;
	//	float metallic = 0.f;
	//	float reflectance = 0.f;
	//	float ambientOcclusion = 0.f;
	//	float clearCoat = 0.0f;
	//	float clearCoatRoughness = 0.0f;
	//	float anisotropy = 0.0f;
	//	float heightScale = 0.0f;
	//};

	struct Material
	{
		u64 materialID = 0;
		ShaderHandle shaderId = InvalidShader;
		u64 texturePackHandle = 0;
		
		tex::Texture diffuseTexture;
		tex::Texture specularTexture;
		tex::Texture normalMapTexture;
		tex::Texture emissionTexture;
		tex::Texture metallicTexture;
		tex::Texture roughnessTexture;
		tex::Texture aoTexture;
		tex::Texture heightTexture;
		tex::Texture anisotropyTexture;

		glm::vec3 baseColor = glm::vec3(1.0f);
		float specular = 0.f;
		float roughness = 0.f;
		float metallic = 0.f;
		float reflectance = 0.f;
		float ambientOcclusion = 1.f;
		float clearCoat = 0.0f;
		float clearCoatRoughness = 0.0f;
		float anisotropy = 0.0f;
		float heightScale = 0.0f;
	};

	struct MaterialTextureEntry
	{
		r2::asset::AssetType mType = r2::asset::TEXTURE; //TEXTURE OR CUBEMAP
		s64 mIndex = -1; //index into either mMaterialTextures or mCubemapTextures based on mType
	};

	struct MaterialTextureAssets
	{
		tex::Texture diffuseTexture;
		tex::Texture specularTexture;
		tex::Texture normalMapTexture;
		tex::Texture emissionTexture;
		tex::Texture metallicTexture;
		tex::Texture roughnessTexture;
		tex::Texture aoTexture;
		tex::Texture heightTexture;
		tex::Texture anisotropyTexture;
	};

	struct InternalMaterialData
	{
		u64 materialName;
		u64 texturePackName;
		ShaderHandle shaderHandle = InvalidShader;
		MaterialTextureAssets textureAssets;
		r2::draw::RenderMaterialParams renderMaterial = {}; //@TODO(Serge): change to RenderMaterialParams
	};

	struct MaterialSystem
	{
		r2::mem::utils::MemBoundary mMaterialMemBoundary = {};
		r2::mem::LinearArena* mLinearArena = nullptr;
		r2::SHashMap<r2::draw::tex::TexturePack*>* mTexturePacks = nullptr;
	//	r2::SArray<r2::draw::Material>* mMaterials = nullptr; //@TODO(Serge): remove

		r2::SArray<InternalMaterialData>* mInternalData = nullptr; 

		r2::SArray<MaterialTextureEntry>* mMaterialTextureEntries = nullptr; //size should the number of materials in the pack
		r2::SArray<r2::SArray<r2::draw::tex::Texture>*>* mMaterialTextures = nullptr;
		r2::SArray<r2::draw::tex::CubemapTexture>* mMaterialCubemapTextures = nullptr;

		void* mMaterialParamsPackData = nullptr;
		void* mTexturePackManifestData = nullptr;

		const flat::MaterialParamsPack* mMaterialParamsPack = nullptr;
		const flat::TexturePacksManifest* mTexturePackManifest = nullptr;

		r2::asset::AssetCache* mAssetCache = nullptr;
		r2::mem::utils::MemBoundary mCacheBoundary = {};
		s32 mSlot = -1;
		u32 mNextAvailableHandle = 0;
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
	MaterialSystem* CreateMaterialSystem(const r2::mem::utils::MemBoundary& boundary, const char* materialParamsPackPath, const char* texturePackManifestPath);
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
	//@TODO(Serge): add function to upload only 1 material?

	void UploadAllMaterialTexturesToGPU(MaterialSystem& system);
	void UploadMaterialTexturesToGPUFromMaterialName(MaterialSystem& system, u64 materialName);
	void UploadMaterialTexturesToGPU(MaterialSystem& system, MaterialHandle matID);

	void UnloadAllMaterialTexturesFromGPU(MaterialSystem& system);

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(const MaterialSystem& system, MaterialHandle matID);
	const r2::SArray<r2::draw::tex::CubemapTexture>* GetCubemapTextures(const MaterialSystem& system);

	const r2::draw::tex::CubemapTexture* GetCubemapTexture(const MaterialSystem& system, MaterialHandle matID);


//	MaterialHandle AddMaterial(MaterialSystem& system, const Material& mat);

//	const Material* GetMaterial(const MaterialSystem& system, MaterialHandle matID);
	ShaderHandle GetShaderHandle(const MaterialSystem& system, MaterialHandle matID);
	const RenderMaterialParams& GetRenderMaterial(const MaterialSystem& system, MaterialHandle matID);

	MaterialHandle GetMaterialHandleFromMaterialName(const MaterialSystem& system, u64 materialName);
	u64 MemorySize(u64 alignment, u64 capacity, u64 textureCacheInBytes, u64 numTextures, u64 numPacks, u64 maxTexturesInAPack, u32 materialParamsFileSize = 0, u32 texturePacksManifestFileSize = 0);

	u64 LoadMaterialAndTextureManifests(const char* materialManifestPath, const char* textureManifestPath, void** materialPack, void** texturePacks);

	u64 GetMaterialBoundarySize(const char* materialManifestPath, const char* textureManifestPath);
}

#endif // !__MATERIAL_H__
