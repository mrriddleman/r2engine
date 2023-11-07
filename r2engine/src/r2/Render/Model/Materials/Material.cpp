#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Materials/Material.h"

namespace r2::mat
{
	void MakeMaterialFromFlatMaterial(u64 materialPackName, const flat::Material* flatMaterial, Material& outMaterial)
	{
		outMaterial.materialName.name = flatMaterial->assetName();
		outMaterial.materialName.packName = materialPackName;
		outMaterial.stringName = flatMaterial->stringName()->str();
		outMaterial.transparencyType = flatMaterial->transparencyType();
		r2::draw::MakeMaterialShaderEffectPasses(flatMaterial->shaderEffectPasses(), outMaterial.shaderEffectPasses);
		r2::draw::MakeShaderParams(flatMaterial->shaderParams(), outMaterial.shaderParams);
	}
}

#endif