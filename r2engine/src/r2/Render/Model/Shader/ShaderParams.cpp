#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Shader/ShaderParams.h"
#include <glm/glm.hpp>

namespace r2::draw
{
	void MakeShaderParams(const flat::ShaderParams* flatShaderParams, ShaderParams& shaderParams)
	{
		if (!flatShaderParams)
		{
			return;
		}

		const auto* flatULongParams = flatShaderParams->ulongParams();
		flatbuffers::uoffset_t numULongParams = flatULongParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numULongParams; ++i)
		{
			const flat::ShaderULongParam* flatULongParam = flatULongParams->Get(i);

			ShaderULongParam shaderULongParam;
			shaderULongParam.propertyType = flatULongParam->propertyType();
			shaderULongParam.value = flatULongParam->value();

			shaderParams.ulongParams.push_back(shaderULongParam);
		}

		const auto* flatBoolParams = flatShaderParams->boolParams();
		flatbuffers::uoffset_t numBoolParams = flatBoolParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numBoolParams; ++i)
		{
			const flat::ShaderBoolParam* flatBoolParam = flatBoolParams->Get(i);

			ShaderBoolParam shaderBoolParam;
			shaderBoolParam.propertyType = flatBoolParam->propertyType();
			shaderBoolParam.value = flatBoolParam->value();

			shaderParams.boolParams.push_back(shaderBoolParam);
		}

		const auto* flatFloatParams = flatShaderParams->floatParams();
		flatbuffers::uoffset_t numFloatParams = flatFloatParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numFloatParams; ++i)
		{
			const flat::ShaderFloatParam* flatFloatParam = flatFloatParams->Get(i);

			ShaderFloatParam shaderFloatParam;
			shaderFloatParam.propertyType = flatFloatParam->propertyType();
			shaderFloatParam.value = flatFloatParam->value();

			shaderParams.floatParams.push_back(shaderFloatParam);
		}

		const auto* flatColorParams = flatShaderParams->colorParams();
		flatbuffers::uoffset_t numColorParams = flatColorParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numColorParams; ++i)
		{
			const flat::ShaderColorParam* flatColorParam = flatColorParams->Get(i);

			ShaderColorParam shaderColorParam;
			shaderColorParam.propertyType = flatColorParam->propertyType();
			shaderColorParam.value = glm::vec4(flatColorParam->value()->r(), flatColorParam->value()->g(), flatColorParam->value()->b(), flatColorParam->value()->a());

			shaderParams.colorParams.push_back(shaderColorParam);
		}

		const auto* flatStringParams = flatShaderParams->stringParams();
		flatbuffers::uoffset_t numStringParams = flatStringParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numStringParams; ++i)
		{
			const flat::ShaderStringParam* flatStringParam = flatStringParams->Get(i);

			ShaderStringParam shaderStringParam;
			shaderStringParam.propertyType = flatStringParam->propertyType();
			shaderStringParam.value = flatStringParam->value()->str();

			shaderParams.stringParams.push_back(shaderStringParam);
		}

		const auto* flatShaderStageParams = flatShaderParams->shaderStageParams();
		flatbuffers::uoffset_t numShaderStageParams = flatShaderStageParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numShaderStageParams; ++i)
		{
			const flat::ShaderStageParam* flatShaderStageParam = flatShaderStageParams->Get(i);

			ShaderStageParam shaderStageParam;
			shaderStageParam.propertyType = flatShaderStageParam->propertyType();
			shaderStageParam.shaderName = flatShaderStageParam->shader();
			shaderStageParam.shaderStageName = flatShaderStageParam->shaderStageName();
			
			shaderStageParam.value = "";
			if (flatShaderStageParam->value())
			{
				shaderStageParam.value = flatShaderStageParam->value()->str();
			}

			shaderParams.shaderStageParams.push_back(shaderStageParam);
		}

		const auto* flatTextureParams = flatShaderParams->textureParams();
		flatbuffers::uoffset_t numTextureParams = flatTextureParams->size();
		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const flat::ShaderTextureParam* flatTextureParam = flatTextureParams->Get(i);

			ShaderTextureParam shaderTextureParam;
			shaderTextureParam.propertyType = flatTextureParam->propertyType();
			shaderTextureParam.value = flatTextureParam->value();
			shaderTextureParam.packingType = flatTextureParam->packingType();
			shaderTextureParam.texturePackName = flatTextureParam->texturePackName();
			shaderTextureParam.texturePackNameString = flatTextureParam->texturePackNameStr()->str();
			shaderTextureParam.minFilter = flatTextureParam->minFilter();
			shaderTextureParam.magFilter = flatTextureParam->magFilter();
			shaderTextureParam.anisotropicFiltering = flatTextureParam->anisotropicFiltering();
			shaderTextureParam.wrapS = flatTextureParam->wrapS();
			shaderTextureParam.wrapT = flatTextureParam->wrapT();
			shaderTextureParam.wrapR = flatTextureParam->wrapR();

			shaderParams.textureParams.push_back(shaderTextureParam);
		}
	}
}

#endif