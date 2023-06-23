#ifndef __MATERIAL_PARAMS_HELPERS_H__
#define __MATERIAL_PARAMS_HELPERS_H__


#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Render/Renderer/Shader.h"

namespace flat
{
	struct MaterialParams;
	struct MaterialParamsPack;
	struct MaterialName;
}

namespace r2::mat
{
	const flat::MaterialParams* GetMaterialParamsForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	const flat::MaterialParams* GetMaterialParamsForMaterialName(MaterialName materialName);

	u64 GetShaderNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	u64 GetAlbedoTextureNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	r2::draw::ShaderHandle GetShaderHandleForMaterialName(MaterialName materialName);

	MaterialName MakeMaterialNameFromFlatMaterial(const flat::MaterialName* flatMaterialName);

#ifdef R2_ASSET_PIPELINE
	std::vector<const flat::MaterialParams*> GetAllMaterialParamsInMaterialPackThatContainTexture(const flat::MaterialParamsPack* materialPack, u64 texturePackName, u64 textureName);
	std::vector<const flat::MaterialParams*> GetAllMaterialParamsInMaterialPackThatContainTexturePack(const flat::MaterialParamsPack* materialPack, u64 texturePackName);
#endif

#ifdef R2_EDITOR
	//std::vector<std::string> GetAllTexturePacksForMaterialNames(const r2::SArray<MaterialName>& materialNames);
#endif
}

#endif
