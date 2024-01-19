#ifndef __SHADER_PARAMS_H__
#define __SHADER_PARAMS_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Shader/ShaderParams_generated.h"
#include "r2/Render/Model/Shader/ShaderTypes.h"
#include "r2/Core/Assets/AssetTypes.h"
#include <glm/glm.hpp>

namespace r2::draw
{
	struct ShaderFloatParam
	{
		flat::ShaderPropertyType propertyType;
		float value;
	};

	struct ShaderColorParam
	{
		flat::ShaderPropertyType propertyType;
		glm::vec4 value;
	};

	struct ShaderBoolParam
	{
		flat::ShaderPropertyType propertyType;
		bool value;
	};

	struct ShaderULongParam
	{
		flat::ShaderPropertyType propertyType;
		u64 value;
	};

	struct ShaderStringParam
	{
		flat::ShaderPropertyType propertyType;
		std::string value;
	};

	struct ShaderTextureParam
	{
		flat::ShaderPropertyType propertyType;
		r2::asset::AssetName value;
		flat::ShaderPropertyPackingType packingType;
		r2::asset::AssetName texturePack;
		flat::MinTextureFilter minFilter;
		flat::MagTextureFilter magFilter;
		float anisotropicFiltering;
		flat::TextureWrapMode wrapS;
		flat::TextureWrapMode wrapT;
		flat::TextureWrapMode wrapR;
		u32 textureCoordIndex;
	};

	struct ShaderStageParam
	{
		flat::ShaderPropertyType propertyType;
		r2::draw::ShaderName shaderName;
		r2::draw::ShaderName shaderStageName;
		std::string value;
	};

	struct ShaderParams
	{
		std::vector<ShaderULongParam> ulongParams;
		std::vector<ShaderBoolParam> boolParams;
		std::vector<ShaderFloatParam> floatParams;
		std::vector<ShaderColorParam> colorParams;
		std::vector<ShaderTextureParam> textureParams;
		std::vector<ShaderStringParam> stringParams;
		std::vector<ShaderStageParam> shaderStageParams;
	};

	void MakeShaderParams(const flat::ShaderParams* flatShaderParams, ShaderParams& shaderParams);
}

#endif
#endif