#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Shader/ShaderEffect.h"

namespace r2::draw
{
	void MakeMaterialShaderEffectPasses(const flat::ShaderEffectPasses* shaderEffectPasses, MaterialShaderEffectPasses& materialShaderEffectPasses)
	{
		for (flatbuffers::uoffset_t i = 0; i < shaderEffectPasses->shaderEffectPasses()->size(); ++i)
		{
			const flat::ShaderEffect* flatShaderEffect = shaderEffectPasses->shaderEffectPasses()->Get(i);

			materialShaderEffectPasses.meshPasses[i].assetName = flatShaderEffect->assetName();
			materialShaderEffectPasses.meshPasses[i].assetNameString = flatShaderEffect->assetNameString()->str();
			materialShaderEffectPasses.meshPasses[i].staticShader = flatShaderEffect->staticShader();
			materialShaderEffectPasses.meshPasses[i].dynamicShader = flatShaderEffect->dynamicShader();
			materialShaderEffectPasses.meshPasses[i].staticVertexLayout = flatShaderEffect->staticVertexLayout();
			materialShaderEffectPasses.meshPasses[i].dynamicVertexLayout = flatShaderEffect->dynamicVertexLayout();
		}

		MakeShaderParams(shaderEffectPasses->defaultShaderParams(), materialShaderEffectPasses.defaultShaderParams);
	}
}


#endif