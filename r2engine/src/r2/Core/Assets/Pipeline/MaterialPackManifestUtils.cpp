#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Materials/Material_generated.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"

#include "r2/Render/Model/Shader/ShaderEffect_generated.h"
#include "r2/Render/Model/Shader/ShaderEffectPasses_generated.h"
#include "r2/Render/Model/Shader/ShaderParams_generated.h"

#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Utils/Flags.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

namespace r2::asset::pln
{
	const std::string MATERIAL_PACK_MANIFEST_NAME_FBS = "MaterialPack.fbs";
	const std::string MATERIAL_FBS = "Material.fbs";

	const std::string JSON_EXT = ".json";
	
	const std::string MPAK_EXT = ".mpak";
	const std::string MTRL_EXT = ".mtrl";

	const std::string MATERIALS_JSON_DIR = "materials_raw";
	const std::string MATERIALS_BIN_DIR = "materials_bin";


	const bool GENERATE_MATERIALS = false; 

	//@HACKY but meh...
	char sanitizeRootMaterialDirPath[r2::fs::FILE_PATH_LENGTH];

	bool GenerateMaterialFromJSON(const std::string& outputDir, const std::string& path)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialSchemaPath, MATERIAL_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, materialSchemaPath, path);
	}


	bool GenerateMaterialPackManifestFromJson(const std::string& jsonMaterialPackManifestFilePath, const std::string& outputDir)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, texturePackManifestSchemaPath, jsonMaterialPackManifestFilePath);
	}

	bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)
	{
		return FindManifestFile(directory, stemName, MPAK_EXT, outPath, isBinary);

		//for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		//{
		//	//UGH MAC - ignore .DS_Store
		//	if (std::filesystem::file_size(file.path()) <= 0 ||
		//		(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
		//			std::filesystem::path(file.path()).extension().string() != MPAK_EXT &&
		//			file.path().stem().string() != stemName))
		//	{
		//		continue;
		//	}

		//	if (file.path().stem().string() == stemName && ((isBinary && std::filesystem::path(file.path()).extension().string() == MPAK_EXT) ||
		//		(!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT)))
		//	{
		//		outPath = file.path().string();
		//		return true;
		//	}
		//}

		//return false;
	}

	bool RegenerateMaterialPackManifest(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		return GenerateBinaryMaterialPackManifest(binaryDir, binFilePath, rawFilePath);//GenerateBinaryParamsPackManifest(binaryDir, binFilePath, rawFilePath);
	}

	bool SaveMaterialsToMaterialPackManifestFile(const std::vector<r2::mat::Material>& materials, const std::string& binFilePath, const std::string& rawFilePath)
	{
		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::Material >> flatMaterials;

		for (const auto& material :materials)
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
							shaderTextureParam.wrapS, shaderTextureParam.wrapT, shaderTextureParam.wrapR));
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

			flatMaterials.push_back(flat::CreateMaterial(
				builder,
				assetName,
				material.transparencyType,
				shaderEffectPasses,
				shaderParams));
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest

		const auto materialPackName = r2::asset::Asset::GetAssetNameForFilePath(binPath.string().c_str(), r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

		char materialPackNameStr[r2::fs::FILE_PATH_LENGTH];

		r2::asset::MakeAssetNameStringForFilePath(binPath.string().c_str(), materialPackNameStr, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

		auto assetName = flat::CreateAssetName(builder, 0, materialPackName, builder.CreateString(materialPackNameStr));

		//@TODO(Serge): version
		auto manifest = flat::CreateMaterialPack(builder, 1, assetName, builder.CreateVector(flatMaterials));

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(binPath.string(), (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialPackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path jsonPath = rawFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), materialPackManifestSchemaPath, binPath.string());

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binPath.parent_path().string(), materialPackManifestSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}
	
	//bool GenerateMaterialFromOldMaterialParamsPack(const flat::MaterialParams* const materialParamsData, const std::string& pathOfSource, const std::string& outputDir)
	//{
	//	R2_CHECK(materialParamsData != nullptr, "Passed in nullptr for the materialParamsData!");

	//	std::filesystem::path sourceFilePath = pathOfSource;

	//	//For the engine materials: First we need to check to see if the string "Dynamic" or "Static" is at the start of the material name
	//	//if it is then we strip it off
	//	std::string materialName = sourceFilePath.stem().string();

	//	static const std::string STATIC_STRING = "Static";
	//	static const std::string DYNAMIC_STRING = "Dynamic";

	//	//for engine materials only
	//	if (materialName.find(STATIC_STRING, 0) != std::string::npos)
	//	{
	//		materialName = materialName.substr(STATIC_STRING.size());
	//	}
	//	else if (materialName.find(DYNAMIC_STRING, 0) != std::string::npos)
	//	{
	//		materialName = materialName.substr(DYNAMIC_STRING.size());
	//	}

	//	std::filesystem::path materialDir = std::filesystem::path(outputDir) / materialName;

	//	std::filesystem::path materialJSONPath = materialDir / (materialName + JSON_EXT);

	//	bool materialDirExists = std::filesystem::exists(materialDir);

	//	if (materialDirExists && std::filesystem::exists(materialJSONPath))
	//	{
	//		//we already made the material - don't do it again
	//		return true;
	//	}

	//	if (!materialDirExists)
	//	{
	//		std::filesystem::create_directory(materialDir);
	//	}

	//	struct ShaderEffect
	//	{
	//		std::string shaderEffectName = "";
	//		u64 staticShader = 0;
	//		u64 dynamicShader = 0;
	//	};

	//	struct ShaderEffectPasses
	//	{
	//		ShaderEffect shaderEffectPasses[flat::eMeshPass::eMeshPass_NUM_SHADER_EFFECT_PASSES];
	//		bool isOpaque;
	//	};

	//	static ShaderEffect forwardOpaqueShaderEffect = {
	//		"forward_opaque_shader_effect",
	//		STRING_ID("Sandbox"),
	//		STRING_ID("AnimModel")
	//	};

	//	static ShaderEffect forwardTransparentShaderEffect = {
	//		"forward_transparent_shader_effect",
	//		STRING_ID("TransparentModel"),
	//		STRING_ID("TransparentAnimModel")
	//	};

	//	static ShaderEffect skyboxShaderEffect = {
	//		"default_skybox_shader_effect",
	//		STRING_ID("Skybox"),
	//		0
	//	};

	//	static ShaderEffect ellenEyeShaderEffect = {
	//		"ellen_eye_shader_effect",
	//		0,
	//		STRING_ID("EllenEyeShader")
	//	};

	//	static ShaderEffect debugShaderEffect = {
	//		"debug_shader_effect",
	//		STRING_ID("Debug"),
	//		0
	//	};

	//	static ShaderEffect debugTransparentShaderEffect = {
	//		"debug_transparent_shader_effect",
	//		STRING_ID("TransparentDebug"),
	//		0
	//	};

	//	static ShaderEffect debugModelShaderEffect = {
	//		"debug_model_shader_effect",
	//		STRING_ID("DebugModel"),
	//		0
	//	};

	//	static ShaderEffect debugModelTransparentShaderEffect = {
	//		"debug_model_transparent_shader_effect",
	//		STRING_ID("TransparentDebugModel"),
	//		0
	//	};

	//	static ShaderEffect depthShaderEffect = {
	//		"depth_shader_effect",
	//		STRING_ID("StaticDepth"),
	//		STRING_ID("DynamicDepth")
	//	};

	//	static ShaderEffect entityColorEffect = {
	//		"entity_color_effect",
	//		STRING_ID("StaticEntityColor"),
	//		STRING_ID("DynamicEntityColor")
	//	};

	//	static ShaderEffect defaultOutlineEffect = {
	//		"default_outline_effect",
	//		STRING_ID("StaticOutline"),
	//		STRING_ID("DynamicOutline")
	//	};

	//	static ShaderEffect directionShadowEffect = {
	//		"direction_shadow_effect",
	//		STRING_ID("StaticShadowDepth"),
	//		STRING_ID("DynamicShaderDepth")
	//	};

	//	static ShaderEffect screenCompositeEffect = {
	//		"screen_composite_effect",
	//		STRING_ID("Screen"),
	//		0
	//	};

	//	static ShaderEffect pointShadowEffect = {
	//		"point_shadow_effect",
	//		STRING_ID("PointLightStaticShadowDepth"),
	//		STRING_ID("PointLightDynamicShadowDepth")
	//	};

	//	static ShaderEffect spotLightShadowEffect = {
	//		"spotlight_shadow_effect",
	//		STRING_ID("SpotLightStaticShadowDepth"),
	//		STRING_ID("SpotLightDynamicShadowDepth")
	//	};


	//	static ShaderEffectPasses opaqueForwardPass = {
	//		{forwardOpaqueShaderEffect, {}, depthShaderEffect, directionShadowEffect, pointShadowEffect, spotLightShadowEffect},
	//		true
	//	};

	//	static ShaderEffectPasses transparentForwardPass = {
	//		{{}, forwardTransparentShaderEffect, {}, {}, {}, {}},
	//		false
	//	};

	//	static ShaderEffectPasses skyboxPass = {
	//		{skyboxShaderEffect, {}, {}, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses ellenEyePass = {
	//		{ellenEyeShaderEffect,{}, depthShaderEffect, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses debugOpaquePass = {
	//		{debugShaderEffect,{},{},{}, {}, {} },
	//		true
	//	};

	//	static ShaderEffectPasses debugTransparentPass = {
	//		{{}, debugTransparentShaderEffect, {}, {}, {}, {}},
	//		false
	//	};

	//	static ShaderEffectPasses debugModelOpaquePass = {
	//		{debugModelShaderEffect, {}, depthShaderEffect, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses debugModelTransparentPass = {
	//		{{},debugModelTransparentShaderEffect,{}, {}, {}, {}},
	//		false
	//	};

	//	static ShaderEffectPasses depthPass = {
	//		{{}, {}, depthShaderEffect, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses entityColorPass = {
	//		{entityColorEffect, {}, {}, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses defaultOutlinePass = {
	//		{defaultOutlineEffect, {}, {}, {}, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses shadowDepthPass = {
	//		{{}, {}, {}, directionShadowEffect, {}, {}},
	//		true
	//	};

	//	static ShaderEffectPasses screenCompositePass = {
	//		{screenCompositeEffect, {}, {}, {}, {}, {}},
	//		true
	//	};

	//	static std::unordered_map<u64, ShaderEffectPasses> s_shaderAssetNameToShaderEffectPassesMap =
	//	{
	//		{STRING_ID("AnimModel"), opaqueForwardPass},
	//		{STRING_ID("Sandbox"), opaqueForwardPass},
	//		{STRING_ID("TransparentModel"), transparentForwardPass},
	//		{STRING_ID("TransparentAnimModel"), transparentForwardPass},
	//		{STRING_ID("Skybox"), skyboxPass},
	//		{STRING_ID("EllenEyeShader"), ellenEyePass},

	//		//@TODO(Serge): the internal renderer materials
	//		{STRING_ID("Debug"), debugOpaquePass},
	//		{STRING_ID("DebugModel"), debugModelOpaquePass},
	//		{STRING_ID("TransparentDebug"), debugTransparentPass},
	//		{STRING_ID("TransparentDebugModel"), debugModelTransparentPass},
	//		{STRING_ID("DynamicDepth"), depthPass},
	//		{STRING_ID("StaticDepth"), depthPass},
	//		{STRING_ID("DynamicEntityColor"), entityColorPass},
	//		{STRING_ID("StaticEntityColor"), entityColorPass},
	//		{STRING_ID("DynamicOutline"), defaultOutlinePass},
	//		{STRING_ID("StaticOutline"), defaultOutlinePass},
	//		{STRING_ID("DynamicShadowDepth"), shadowDepthPass},
	//		{STRING_ID("StaticShadowDepth"), shadowDepthPass},
	//		{STRING_ID("Screen"), screenCompositePass}
	//	};

	//	

	//	flatbuffers::FlatBufferBuilder builder;

	//	std::vector<flatbuffers::Offset<flat::ShaderULongParam>> shaderULongParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderBoolParam>>  shaderBoolParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderFloatParam>> shaderFloatParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderColorParam>> shaderColorParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderTextureParam>> shaderTextureParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderStringParam>> shaderStringParams;
	//	std::vector<flatbuffers::Offset<flat::ShaderStageParam>> shaderStageParams;

	//	flatbuffers::uoffset_t numULongParams = 0;
	//	if(materialParamsData->ulongParams())
	//		numULongParams = materialParamsData->ulongParams()->size();

	//	flatbuffers::uoffset_t numBoolParams = 0;
	//	if(materialParamsData->boolParams())
	//		numBoolParams = materialParamsData->boolParams()->size();

	//	flatbuffers::uoffset_t numFloatParams = 0;
	//	if (materialParamsData->floatParams())
	//		numFloatParams = materialParamsData->floatParams()->size();

	//	flatbuffers::uoffset_t numColorParams = 0;
	//	if (materialParamsData->colorParams())
	//		numColorParams = materialParamsData->colorParams()->size();
	//	
	//	flatbuffers::uoffset_t numTextureParams = 0;
	//	if(materialParamsData->textureParams())
	//		numTextureParams = materialParamsData->textureParams()->size();

	//	flatbuffers::uoffset_t numStringParams = 0;
	//	if(materialParamsData->stringParams())
	//		numStringParams = materialParamsData->stringParams()->size();

	//	flatbuffers::uoffset_t numShaderStageParams = 0;
	//	if (materialParamsData->shaderParams())
	//		numShaderStageParams = materialParamsData->shaderParams()->size();

	//	u64 shaderName = 0;

	//	for (flatbuffers::uoffset_t i = 0; i < numULongParams; ++i)
	//	{
	//		const flat::MaterialULongParam* materialULongParam = materialParamsData->ulongParams()->Get(i);

	//		if (materialULongParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_SHADER)
	//		{
	//			//don't bother with shaders though we'll need to use this later to create the shader effect data
	//			shaderName = materialULongParam->value();
	//			continue;
	//		}

	//		shaderULongParams.push_back(flat::CreateShaderULongParam(builder, static_cast<flat::ShaderPropertyType>(materialULongParam->propertyType()), materialULongParam->value()));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numBoolParams; ++i)
	//	{
	//		const flat::MaterialBoolParam* materialBoolParam = materialParamsData->boolParams()->Get(i);
	//		shaderBoolParams.push_back(flat::CreateShaderBoolParam(builder, static_cast<flat::ShaderPropertyType>(materialBoolParam->propertyType()), materialBoolParam->value()));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numFloatParams; ++i)
	//	{
	//		const flat::MaterialFloatParam* materialFloatParam = materialParamsData->floatParams()->Get(i);
	//		shaderFloatParams.push_back(flat::CreateShaderFloatParam(builder, static_cast<flat::ShaderPropertyType>(materialFloatParam->propertyType()), materialFloatParam->value()));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numColorParams; ++i)
	//	{
	//		const flat::MaterialColorParam* materialColorParam = materialParamsData->colorParams()->Get(i);
	//		shaderColorParams.push_back(flat::CreateShaderColorParam(builder, static_cast<flat::ShaderPropertyType>(materialColorParam->propertyType()), materialColorParam->value()));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
	//	{
	//		const flat::MaterialTextureParam* materialTextureParam = materialParamsData->textureParams()->Get(i);

	//		shaderTextureParams.push_back(flat::CreateShaderTextureParam(
	//			builder,
	//			static_cast<flat::ShaderPropertyType>(materialTextureParam->propertyType()),
	//			materialTextureParam->value(),
	//			static_cast<flat::ShaderPropertyPackingType>(materialTextureParam->packingType()),
	//			materialTextureParam->texturePackName(),
	//			builder.CreateString(materialTextureParam->texturePackNameStr()),
	//			materialTextureParam->minFilter(),
	//			materialTextureParam->magFilter(),
	//			materialTextureParam->anisotropicFiltering(),
	//			materialTextureParam->wrapS(),
	//			materialTextureParam->wrapT(),
	//			materialTextureParam->wrapR()));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numStringParams; ++i)
	//	{
	//		const flat::MaterialStringParam* materialStringParam = materialParamsData->stringParams()->Get(i);
	//		shaderStringParams.push_back(flat::CreateShaderStringParam(builder, static_cast<flat::ShaderPropertyType>(materialStringParam->propertyType()), builder.CreateString(materialStringParam->value())));
	//	}

	//	for (flatbuffers::uoffset_t i = 0; i < numShaderStageParams; ++i)
	//	{
	//		const flat::MaterialShaderParam* materialShaderParam= materialParamsData->shaderParams()->Get(i);
	//		shaderStageParams.push_back(flat::CreateShaderStageParam(builder, static_cast<flat::ShaderPropertyType>(materialShaderParam->propertyType()), materialShaderParam->shader(), materialShaderParam->shaderStageName(), builder.CreateString(materialShaderParam->value())));
	//	}

	//	auto shaderParams = flat::CreateShaderParams(
	//		builder,
	//		builder.CreateVector(shaderULongParams),
	//		builder.CreateVector(shaderBoolParams),
	//		builder.CreateVector(shaderFloatParams),
	//		builder.CreateVector(shaderColorParams),
	//		builder.CreateVector(shaderTextureParams),
	//		builder.CreateVector(shaderStringParams),
	//		builder.CreateVector(shaderStageParams));

	//	//@TODO(Serge): now we need to figure out the ShaderEffectPasses
	//	const auto& shaderEffectPasses = s_shaderAssetNameToShaderEffectPassesMap[shaderName];

	//	std::vector<flatbuffers::Offset<flat::ShaderEffect>> flatShaderEffects;

	//	for (s32 i = 0; i < flat::eMeshPass::eMeshPass_NUM_SHADER_EFFECT_PASSES; ++i)
	//	{
	//		const auto& effect = shaderEffectPasses.shaderEffectPasses[i];
	//		const auto effectName = effect.shaderEffectName;
	//		flatShaderEffects.push_back(flat::CreateShaderEffect(builder, STRING_ID(effectName.c_str()), builder.CreateString(effectName), effect.staticShader, effect.dynamicShader));
	//	}

	//	const auto flatShaderEffectPasses = flat::CreateShaderEffectPasses(builder, builder.CreateVector(flatShaderEffects));

	//	const auto flatMaterial = flat::CreateMaterial(
	//		builder,
	//		STRING_ID(materialName.c_str()),
	//		builder.CreateString(materialName),
	//		shaderEffectPasses.isOpaque ? flat::eTransparencyType_OPAQUE : flat::eTransparencyType_TRANSPARENT,
	//		flatShaderEffectPasses, shaderParams);

	//	builder.Finish(flatMaterial);


	//	byte* buf = builder.GetBufferPointer();
	//	u32 size = builder.GetSize();

	//	std::string tempFile = (materialDir / materialName).string() + ".bin";
	//	
	//	bool wroteTempFile = utils::WriteFile(tempFile, (char*)buf, size);

	//	std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

	//	char materialSchemaPath[r2::fs::FILE_PATH_LENGTH];

	//	r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialSchemaPath, MATERIAL_FBS.c_str());

	//	bool generatedJSON = flathelp::GenerateFlatbufferJSONFile(materialDir.string(), materialSchemaPath, tempFile);
	//	
	//	R2_CHECK(generatedJSON, "We didn't generate the JSON file from: %s", tempFile);

	//	std::filesystem::remove(tempFile);

	//	return generatedJSON;
	//}

	bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{

		//first generate all of the materials from JSON if they need to be generated
		for (const auto& materialDir : std::filesystem::directory_iterator(rawDir))
		{
			if (materialDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			//look for a json file in the raw dir
			std::filesystem::path jsonDir = materialDir;

			std::filesystem::path binDir = std::filesystem::path(binaryDir) / materialDir.path().stem();

			char sanitizedPath[r2::fs::FILE_PATH_LENGTH];

			//make the directory if we need it

			r2::fs::utils::SanitizeSubPath(binDir.string().c_str(), sanitizedPath);

			std::filesystem::create_directory(sanitizedPath);

			for (const auto& jsonfile : std::filesystem::directory_iterator(jsonDir))
			{
				if (jsonfile.path().filename() == ".DS_Store" ||
					std::filesystem::file_size(jsonfile.path()) <= 0 ||
					std::filesystem::path(jsonfile.path()).extension().string() != JSON_EXT)
				{
					continue;
				}

				//check to see if we have the corresponding .mprm file
				bool found = false;
				for (const auto& binFile : std::filesystem::directory_iterator(sanitizedPath))
				{
					if (std::filesystem::file_size(binFile.path()) > 0 &&
						binFile.path().stem() == jsonfile.path().stem() &&
						std::filesystem::path(binFile.path()).extension().string() == MTRL_EXT)
					{
						found = true;
						break;
					}
				}

				if (found)
					continue;

				//we didn't find it so generate it
				bool generated = GenerateMaterialFromJSON(std::string(sanitizedPath), jsonfile.path().string());
				R2_CHECK(generated, "Failed to generate the material params: %s", jsonfile.path().string().c_str());
			}
		}

		return GenerateBinaryMaterialPackManifest(binaryDir, binFilePath, rawFilePath);
	}

	bool GenerateBinaryMaterialPackManifest(const std::string& binaryDir, const std::string& binFilePath, const std::string& rawFilePath)
	{
		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::Material >> flatMaterials;

		//okay now we know that we have all of the material generated properly
		//so now we need to make the manifests by reading in all of the material.mmat files in the binary directory
		//we do that for each pack
		for (const auto& materialParamsPackDir : std::filesystem::directory_iterator(binaryDir))
		{
			if (materialParamsPackDir.path().filename() == ".DS_Store" || std::filesystem::is_empty(materialParamsPackDir))
			{
				continue;
			}

			//Get the path
			std::filesystem::path binDir = materialParamsPackDir;
			std::string packToRead = "";
			for (const auto& binFile : std::filesystem::directory_iterator(binDir))
			{
				if (std::filesystem::file_size(binFile.path()) > 0 &&
					std::filesystem::path(binFile.path()).extension().string() == MTRL_EXT)
				{
					packToRead = binFile.path().string();
					break;
				}
			}

			//read the file
			char* materialData = utils::ReadFile(packToRead);

			const auto binMaterial = flat::GetMaterial(materialData);

			std::vector<flatbuffers::Offset<flat::ShaderEffect>> flatShaderEffects;

			for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderEffectPasses()->shaderEffectPasses()->size(); ++i)
			{
				const auto* shaderEffect = binMaterial->shaderEffectPasses()->shaderEffectPasses()->Get(i);

				flatShaderEffects.push_back(flat::CreateShaderEffect(
					builder,
					shaderEffect->assetName(),
					builder.CreateString(shaderEffect->assetNameString()),
					shaderEffect->staticShader(),
					shaderEffect->dynamicShader(),
					shaderEffect->staticVertexLayout(),
					shaderEffect->dynamicVertexLayout()));
			}

			auto shaderEffectPasses = flat::CreateShaderEffectPasses(builder, builder.CreateVector(flatShaderEffects), 0); //@NOTE(Serge): we should probably have some kind of defaults here

			std::vector<flatbuffers::Offset<flat::ShaderULongParam>> shaderULongParams;
			std::vector<flatbuffers::Offset<flat::ShaderBoolParam>> shaderBoolParams;
			std::vector<flatbuffers::Offset<flat::ShaderFloatParam>> shaderFloatParams;
			std::vector<flatbuffers::Offset<flat::ShaderColorParam>> shaderColorParams;
			std::vector<flatbuffers::Offset<flat::ShaderTextureParam>> shaderTextureParams;
			std::vector<flatbuffers::Offset<flat::ShaderStringParam>> shaderStringParams;
			std::vector<flatbuffers::Offset<flat::ShaderStageParam>> shaderStageParams;

			if (binMaterial->shaderParams()->ulongParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->ulongParams()->size(); ++i)
				{
					const auto* shaderULongParam = binMaterial->shaderParams()->ulongParams()->Get(i);
					shaderULongParams.push_back(flat::CreateShaderULongParam(builder, shaderULongParam->propertyType(), shaderULongParam->value()));
				}
			}

			if (binMaterial->shaderParams()->boolParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->boolParams()->size(); ++i)
				{
					const auto* shaderBoolParam = binMaterial->shaderParams()->boolParams()->Get(i);
					shaderBoolParams.push_back(flat::CreateShaderBoolParam(builder, shaderBoolParam->propertyType(), shaderBoolParam->value()));
				}
			}

			if (binMaterial->shaderParams()->floatParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->floatParams()->size(); ++i)
				{
					const auto* shaderFloatParam = binMaterial->shaderParams()->floatParams()->Get(i);
					shaderFloatParams.push_back(flat::CreateShaderFloatParam(builder, shaderFloatParam->propertyType(), shaderFloatParam->value()));
				}
			}

			if (binMaterial->shaderParams()->colorParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->colorParams()->size(); ++i)
				{
					const auto* shaderColorParam = binMaterial->shaderParams()->colorParams()->Get(i);
					shaderColorParams.push_back(flat::CreateShaderColorParam(builder, shaderColorParam->propertyType(), shaderColorParam->value()));
				}
			}

			if (binMaterial->shaderParams()->textureParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->textureParams()->size(); ++i)
				{
					const auto* shaderTextureParam = binMaterial->shaderParams()->textureParams()->Get(i);
					//@TODO(Serge): UUID

					std::string textureName = "";
					if (shaderTextureParam->value()->stringName())
					{
						textureName = shaderTextureParam->value()->stringName()->str();
					}

					auto textureAssetName = flat::CreateAssetName(builder, 0, shaderTextureParam->value()->assetName(), builder.CreateString(textureName));
					auto texturePackAssetName = flat::CreateAssetName(builder, 0, shaderTextureParam->texturePack()->assetName(), builder.CreateString(shaderTextureParam->texturePack()->stringName()->str()));

					shaderTextureParams.push_back(
						flat::CreateShaderTextureParam(
							builder,
							shaderTextureParam->propertyType(),
							textureAssetName,
							shaderTextureParam->packingType(),
							texturePackAssetName,
							shaderTextureParam->minFilter(),
							shaderTextureParam->magFilter(),
							shaderTextureParam->anisotropicFiltering(),
							shaderTextureParam->wrapS(), shaderTextureParam->wrapT(), shaderTextureParam->wrapR()));
				}
			}

			if (binMaterial->shaderParams()->stringParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->stringParams()->size(); ++i)
				{
					const auto* shaderStringParam = binMaterial->shaderParams()->stringParams()->Get(i);
					shaderStringParams.push_back(flat::CreateShaderStringParam(builder, shaderStringParam->propertyType(), builder.CreateString(shaderStringParam->value())));
				}
			}

			if (binMaterial->shaderParams()->shaderStageParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < binMaterial->shaderParams()->shaderStageParams()->size(); ++i)
				{
					const auto* shaderStageParam = binMaterial->shaderParams()->shaderStageParams()->Get(i);
					shaderStageParams.push_back(flat::CreateShaderStageParam(builder, shaderStageParam->propertyType(), shaderStageParam->shader(), shaderStageParam->shaderStageName(), builder.CreateString(shaderStageParam->value())));
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

			//@TODO(Serge): UUID
			auto assetName = flat::CreateAssetName(builder,0, binMaterial->assetName()->assetName(), builder.CreateString(binMaterial->assetName()->stringName()->str()));

			flatMaterials.push_back(flat::CreateMaterial(
				builder,
				assetName,
				binMaterial->transparencyType(),
				shaderEffectPasses,
				shaderParams));

			delete[] materialData;
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest

		const auto materialPackName = r2::asset::Asset::GetAssetNameForFilePath(binPath.string().c_str(), r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

		char materialPackNameStr[r2::fs::FILE_PATH_LENGTH];

		r2::asset::MakeAssetNameStringForFilePath(binPath.string().c_str(), materialPackNameStr, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

		//@TODO(Serge): UUID
		auto assetName = flat::CreateAssetName(builder, 0, materialPackName, builder.CreateString(materialPackNameStr));

		//@TODO(Serge): version
		auto manifest = flat::CreateMaterialPack(builder, 1, assetName, builder.CreateVector(flatMaterials));

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(binPath.string(), (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];



		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialPackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path jsonPath = rawFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), materialPackManifestSchemaPath, binPath.string());

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binPath.parent_path().string(), materialPackManifestSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}
}

#endif