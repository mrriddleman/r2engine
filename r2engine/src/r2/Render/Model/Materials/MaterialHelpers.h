#ifndef __MATERIAL_HELPERS_H__
#define __MATERIAL_HELPERS_H__


#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Render/Model/Shader/Shader.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Utils/Utils.h"

namespace flat
{
	struct MaterialPack;
	struct Material;
	struct MaterialName;
}

namespace r2::mat
{

	const flat::Material* GetMaterialForMaterialName(const flat::MaterialPack* materialPack, u64 materialName);
	const flat::Material* GetMaterialForMaterialName(MaterialName materialName);

	u64 GetShaderNameForMaterialName(const flat::MaterialPack* materialPack, u64 materialName, r2::draw::eMeshPass meshPass, r2::draw::eShaderEffectType shaderEffectType);
	r2::draw::ShaderHandle GetShaderHandleForMaterialName(MaterialName materialName, r2::draw::eMeshPass meshPass, r2::draw::eShaderEffectType shaderEffectType);
	r2::draw::ShaderEffectPasses GetShaderEffectPassesForMaterialName(MaterialName materialName);


	u64 GetAlbedoTextureNameForMaterialName(const flat::MaterialPack* materialPack, u64 materialName);

	MaterialName MakeMaterialNameFromFlatMaterial(const flat::MaterialName* flatMaterialName);

	void GetAllTexturePacksForMaterial(const flat::Material* material, r2::SArray<u64>* texturePacks);

	

#ifdef R2_ASSET_PIPELINE
	std::vector<const flat::Material*> GetAllMaterialsInMaterialPackThatContainTexture(const flat::MaterialPack* materialPack, u64 texturePackName, u64 textureName);
	std::vector<const flat::Material*> GetAllMaterialsInMaterialPackThatContainTexturePack(const flat::MaterialPack* materialPack, u64 texturePackName);
#endif
}

#endif