#ifndef __MATERIAL_PARAMS_HELPERS_H__
#define __MATERIAL_PARAMS_HELPERS_H__


#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Render/Renderer/Shader.h"

namespace flat
{
	struct MaterialParams;
	struct MaterialParamsPack;
}

namespace r2::mat
{
	const flat::MaterialParams* GetMaterialParamsForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	u64 GetShaderNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	u64 GetAlbedoTextureNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName);
	r2::draw::ShaderHandle GetShaderHandleForMaterialName(MaterialName materialName);




}

#endif
