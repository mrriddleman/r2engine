#ifndef __SHADER_EFFECT_H__
#define __SHADER_EFFECT_H__

#include "r2/Render/Model/Shader/ShaderTypes.h"
#include "r2/Render/Model/Shader/ShaderEffectPasses_generated.h"
#include "r2/Render/Model/Shader/ShaderParams.h"

namespace r2::draw
{
	enum eShaderEffectType : u8
	{
		SET_STATIC = 0,
		SET_DYNAMIC,
		NUM_SHADER_EFFECT_TYPES
	};

	struct ShaderEffect
	{
		ShaderHandle staticShaderHandle = InvalidShader;
		ShaderHandle dynamicShaderHandle = InvalidShader;
	};

	struct ShaderEffectPasses
	{
		ShaderEffect meshPasses[flat::eMeshPass_NUM_SHADER_EFFECT_PASSES] = { {}, {}, {}, {}, {}, {} };
	};

	//@TODO(Serge): kinda dumb but...
#ifdef R2_ASSET_PIPELINE

	struct MaterialShaderEffect
	{
		ShaderName assetName = 0;
		std::string assetNameString;
		ShaderName staticShader = 0;
		ShaderName dynamicShader = 0;
		flat::eVertexLayoutType staticVertexLayout;
		flat::eVertexLayoutType dynamicVertexLayout;
	};

	struct MaterialShaderEffectPasses
	{
		MaterialShaderEffect meshPasses[flat::eMeshPass_NUM_SHADER_EFFECT_PASSES];
		r2::draw::ShaderParams defaultShaderParams;
	};

	void MakeMaterialShaderEffectPasses(const flat::ShaderEffectPasses* shaderEffectPasses, MaterialShaderEffectPasses& materialShaderEffectPasses);
#endif
}

#endif