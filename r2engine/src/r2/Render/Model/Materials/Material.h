#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Render/Model/Materials/Material_generated.h"
#include "r2/Render/Model/Shader/Shader.h"
#include "r2/Render/Model/Shader/ShaderEffect.h"
#include "r2/Render/Model/Shader/ShaderParams.h"

namespace r2::mat
{
	struct Material
	{
		MaterialName materialName;
		std::string stringName;
		flat::eTransparencyType transparencyType;
		r2::draw::MaterialShaderEffectPasses shaderEffectPasses;
		r2::draw::ShaderParams shaderParams;
	};

	void MakeMaterialFromFlatMaterial(u64 materialPackName, const flat::Material* flatMaterial, Material& outMaterial);

}

#endif
#endif
