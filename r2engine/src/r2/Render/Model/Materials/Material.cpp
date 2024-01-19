#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Render/Model/Materials/Material.h"

namespace r2::mat
{
	void MakeMaterialFromFlatMaterial(u64 materialPackName, const flat::Material* flatMaterial, Material& outMaterial)
	{
		outMaterial.materialName.assetName.hashID = flatMaterial->assetName()->assetName();
		outMaterial.materialName.packName.hashID = materialPackName;
		
		outMaterial.stringName = flatMaterial->assetName()->stringName()->str();

		outMaterial.transparencyType = flatMaterial->transparencyType();
		r2::draw::MakeMaterialShaderEffectPasses(flatMaterial->shaderEffectPasses(), outMaterial.shaderEffectPasses);
		r2::draw::MakeShaderParams(flatMaterial->shaderParams(), outMaterial.shaderParams);
	}

	const flat::Material* MakeFlatMaterialFromMaterial(flatbuffers::FlatBufferBuilder& builder, const Material& material)
	{
		std::vector<flatbuffers::Offset<flat::ShaderEffect>> flatShaderEffects;

		for (u32 i = 0; i < flat::eMeshPass_NUM_SHADER_EFFECT_PASSES; ++i)
		{
			const auto& shaderEffect = material.shaderEffectPasses.meshPasses[i];

			flatShaderEffects.push_back(flat::CreateShaderEffect(
				builder,
				shaderEffect.assetName,
				builder.CreateString(shaderEffect.assetNameString),
				shaderEffect.staticShader,
				shaderEffect.dynamicShader,
				shaderEffect.staticVertexLayout,
				shaderEffect.dynamicVertexLayout));
		}

		auto shaderEffectPasses = flat::CreateShaderEffectPasses(builder, builder.CreateVector(flatShaderEffects), 0); //@NOTE(Serge): we should probably have some kind of defaults here

		std::vector<flatbuffers::Offset<flat::ShaderULongParam>> shaderULongParams;
		std::vector<flatbuffers::Offset<flat::ShaderBoolParam>> shaderBoolParams;
		std::vector<flatbuffers::Offset<flat::ShaderFloatParam>> shaderFloatParams;
		std::vector<flatbuffers::Offset<flat::ShaderColorParam>> shaderColorParams;
		std::vector<flatbuffers::Offset<flat::ShaderTextureParam>> shaderTextureParams;
		std::vector<flatbuffers::Offset<flat::ShaderStringParam>> shaderStringParams;
		std::vector<flatbuffers::Offset<flat::ShaderStageParam>> shaderStageParams;

		if (material.shaderParams.ulongParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.ulongParams.size(); ++i)
			{
				const auto& shaderULongParam = material.shaderParams.ulongParams[i];
				shaderULongParams.push_back(flat::CreateShaderULongParam(builder, shaderULongParam.propertyType, shaderULongParam.value));
			}
		}

		if (material.shaderParams.boolParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.boolParams.size(); ++i)
			{
				const auto& shaderBoolParam = material.shaderParams.boolParams[i];
				shaderBoolParams.push_back(flat::CreateShaderBoolParam(builder, shaderBoolParam.propertyType, shaderBoolParam.value));
			}
		}

		if (material.shaderParams.floatParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.floatParams.size(); ++i)
			{
				const auto& shaderFloatParam = material.shaderParams.floatParams[i];
				shaderFloatParams.push_back(flat::CreateShaderFloatParam(builder, shaderFloatParam.propertyType, shaderFloatParam.value));
			}
		}

		if (material.shaderParams.colorParams.size() > 0)
		{
			for (flatbuffers::uoffset_t i = 0; i < material.shaderParams.colorParams.size(); ++i)
			{
				const auto& shaderColorParam = material.shaderParams.colorParams[i];
				flat::Colour color(shaderColorParam.value.r, shaderColorParam.value.g, shaderColorParam.value.b, shaderColorParam.value.a);

				shaderColorParams.push_back(flat::CreateShaderColorParam(builder, shaderColorParam.propertyType, &color));
			}
		}

		if (material.shaderParams.textureParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.textureParams.size(); ++i)
			{
				const auto& shaderTextureParam = material.shaderParams.textureParams[i];

				//@TODO(Serge): UUID
				auto textureAssetName = flat::CreateAssetName(builder, 0, shaderTextureParam.value.hashID, builder.CreateString(shaderTextureParam.value.assetNameString));
				auto texturePackAssetName = flat::CreateAssetName(builder, 0, shaderTextureParam.texturePack.hashID, builder.CreateString(shaderTextureParam.texturePack.assetNameString));

				shaderTextureParams.push_back(
					flat::CreateShaderTextureParam(
						builder,
						shaderTextureParam.propertyType,
						textureAssetName,
						shaderTextureParam.packingType,
						texturePackAssetName,
						shaderTextureParam.minFilter,
						shaderTextureParam.magFilter,
						shaderTextureParam.anisotropicFiltering,
						shaderTextureParam.wrapS, shaderTextureParam.wrapT, shaderTextureParam.wrapR,
						shaderTextureParam.textureCoordIndex));
			}
		}

		if (material.shaderParams.stringParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.stringParams.size(); ++i)
			{
				const auto& shaderStringParam = material.shaderParams.stringParams[i];
				shaderStringParams.push_back(flat::CreateShaderStringParam(builder, shaderStringParam.propertyType, builder.CreateString(shaderStringParam.value)));
			}
		}

		if (material.shaderParams.shaderStageParams.size() > 0)
		{
			for (u32 i = 0; i < material.shaderParams.shaderStageParams.size(); ++i)
			{
				const auto& shaderStageParam = material.shaderParams.shaderStageParams[i];
				shaderStageParams.push_back(flat::CreateShaderStageParam(builder, shaderStageParam.propertyType, shaderStageParam.shaderName, shaderStageParam.shaderStageName, builder.CreateString(shaderStageParam.value)));
			}
		}

		auto shaderParams = flat::CreateShaderParams(
			builder,
			builder.CreateVector(shaderULongParams),
			builder.CreateVector(shaderBoolParams),
			builder.CreateVector(shaderFloatParams),
			builder.CreateVector(shaderColorParams),
			builder.CreateVector(shaderTextureParams),
			builder.CreateVector(shaderStringParams),
			builder.CreateVector(shaderStageParams));

		auto assetName = flat::CreateAssetName(builder, 0, material.materialName.assetName.hashID, builder.CreateString(material.stringName));

		auto flatMaterial = flat::CreateMaterial(
			builder,
			assetName,
			material.transparencyType,
			shaderEffectPasses,
			shaderParams);

		builder.Finish(flatMaterial);

		byte* buf = builder.GetBufferPointer();

		return flat::GetMaterial(buf);
	}
}

#endif