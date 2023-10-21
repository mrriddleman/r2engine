#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
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

namespace r2::asset::pln
{
	const std::string MATERIAL_PACK_MANIFEST_NAME_FBS = "MaterialPack.fbs";
	const std::string MATERIAL_NAME_FBS = "Material.fbs";

	const std::string MATERIAL_PARAMS_FBS = "MaterialParams.fbs";
	const std::string MATERIAL_PARAMS_PACK_MANIFEST_FBS = "MaterialParamsPack.fbs";

	const std::string JSON_EXT = ".json";
	
	const std::string MPAK_EXT = ".mpak";
	const std::string MTRL_EXT = ".mtrl";

	const std::string MPPK_EXT = ".mppk";
	const std::string MPRM_EXT = ".mprm";

	const std::string MATERIAL_JSON_DIR = "material_raw";
	const std::string MATERIAL_BIN_DIR = "material_bin";

	const bool GENERATE_PARAMS = false;
	const bool GENERATE_MATERIAL_PACK = false;

	bool GenerateMaterialFromJSON(const std::string& outputDir, const std::string& path)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialSchemaPath, MATERIAL_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, materialSchemaPath, path);
	}


	bool GenerateMaterialPackManifestFromJson(const std::string& jsonMaterialPackManifestFilePath, const std::string& outputDir)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, texturePackManifestSchemaPath, jsonMaterialPackManifestFilePath);
	}

	//bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	//{
	//	//the directory we get passed in is the top level directory for all of the materials in the pack
	//	
	//	char sanitizeRootMaterialParamsDirPath[r2::fs::FILE_PATH_LENGTH];

	//	if (GENERATE_PARAMS)
	//	{
	//		std::filesystem::path rootMaterialParamsDir = std::filesystem::path(rawDir).parent_path() / "params_packs";
	//		r2::fs::utils::SanitizeSubPath(rootMaterialParamsDir.string().c_str(), sanitizeRootMaterialParamsDirPath);
	//		std::filesystem::create_directory(sanitizeRootMaterialParamsDirPath);
	//	}

	//	//first generate all of the materials from JSON if they need to be generated
	//	for (const auto& materialPackDir : std::filesystem::directory_iterator(rawDir))
	//	{
	//		if (materialPackDir.path().filename() == ".DS_Store")
	//		{
	//			continue;
	//		}

	//		//look for a json file in the raw dir
	//		std::filesystem::path jsonDir = materialPackDir / MATERIAL_JSON_DIR;

	//		std::filesystem::path tempDir = std::filesystem::path(binaryDir) / materialPackDir.path().stem();
	//		std::filesystem::path binDir = std::filesystem::path(binaryDir) / materialPackDir.path().stem() / MATERIAL_BIN_DIR;

	//		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
	//		r2::fs::utils::SanitizeSubPath(tempDir.string().c_str(), sanitizedPath);

	//		//make the directory if we need it
	//		std::filesystem::create_directory(sanitizedPath);

	//		r2::fs::utils::SanitizeSubPath(binDir.string().c_str(), sanitizedPath);

	//		std::filesystem::create_directory(sanitizedPath);

	//		for (const auto& jsonfile : std::filesystem::directory_iterator(jsonDir))
	//		{
	//			if (jsonfile.path().filename() == ".DS_Store" ||
	//				std::filesystem::file_size(jsonfile.path()) <= 0 ||
	//				std::filesystem::path(jsonfile.path()).extension().string() != JSON_EXT)
	//			{
	//				continue;
	//			}

	//			//check to see if we have the corresponding .mmat file
	//			bool found = false;
	//			for (const auto& binFile : std::filesystem::directory_iterator(sanitizedPath))
	//			{
	//				if (std::filesystem::file_size(binFile.path()) > 0 &&
	//					binFile.path().stem() == jsonfile.path().stem() &&
	//					std::filesystem::path(binFile.path()).extension().string() == MMAT_EXT)
	//				{
	//					found = true;
	//					break;
	//				}
	//			}

	//			if(found)
	//				continue;

	//			//we didn't find it so generate it
	//			bool generated = GenerateMaterialFromJSON(std::string(sanitizedPath), jsonfile.path().string());
	//			R2_CHECK(generated, "Failed to generate the material: %s", jsonfile.path().string().c_str());
	//		}
	//	}

	//	flatbuffers::FlatBufferBuilder builder;
	//	std::vector < flatbuffers::Offset< flat::Material >> flatMaterials;

	//	//okay now we know that we have all of the material generated properly
	//	//so now we need to make the manifests by reading in all of the material.mmat files in the binary directory
	//	//we do that for each pack
	//	for (const auto& materialPackDir : std::filesystem::directory_iterator(binaryDir))
	//	{
	//		if (materialPackDir.path().filename() == ".DS_Store")
	//		{
	//			continue;
	//		}

	//		//Get the path
	//		std::filesystem::path binDir = materialPackDir / MATERIAL_BIN_DIR;
	//		std::string packToRead = "";
	//		for (const auto& binFile : std::filesystem::directory_iterator(binDir))
	//		{
	//			if (std::filesystem::file_size(binFile.path()) > 0 &&
	//				std::filesystem::path(binFile.path()).extension().string() == MMAT_EXT)
	//			{
	//				packToRead = binFile.path().string();
	//				break;
	//			}
	//		}

	//		//read the file
	//		char* materialData = utils::ReadFile(packToRead);
	//		
	//		const auto material = flat::GetMaterial(materialData);

	//		if (GENERATE_PARAMS)
	//		{
	//			GenerateMaterialParamsFromOldMaterialPack(material, packToRead, sanitizeRootMaterialParamsDirPath);
	//		}

	//		flat::Color theColor{ material->baseColor()->r(), material->baseColor()->g(), material->baseColor()->b(), material->baseColor()->a() };

	//		auto packMaterial = flat::CreateMaterial(
	//			builder, material->name(),
	//			material->shader(),
	//			material->texturePackName(),
	//			material->diffuseTexture(),
	//			material->specularTexture(),
	//			material->normalMapTexture(),
	//			material->emissionTexture(),
	//			material->metallicTexture(),
	//			material->roughnessTexture(),
	//			material->aoTexture(),
	//			material->heightTexture(),
	//			material->anisotropyTexture(),
	//			&theColor,
	//			material->specular(),
	//			material->roughness(),
	//			material->metallic(),
	//			material->reflectance(),
	//			material->ambientOcclusion(),
	//			material->clearCoat(),
	//			material->clearCoatRoughness(),
	//			material->anisotropy(),
	//			material->heightScale());

	//		flatMaterials.push_back(packMaterial);

	//		delete[] materialData;
	//	}

	//	std::filesystem::path binPath = binFilePath;

	//	//add the texture packs to the manifest
	//	auto manifest = flat::CreateMaterialPack(builder, STRING_ID(binPath.stem().string().c_str()), builder.CreateVector(flatMaterials));

	//	//generate the manifest
	//	builder.Finish(manifest);

	//	byte* buf = builder.GetBufferPointer();
	//	u32 size = builder.GetSize();

	//	utils::WriteFile(binPath.string(), (char*)buf, size);

	//	std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

	//	char materialPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

	//	r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialPackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

	//	std::filesystem::path jsonPath = rawFilePath;

	//	bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), materialPackManifestSchemaPath, binPath.string());

	//	bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binPath.parent_path().string(), materialPackManifestSchemaPath, jsonPath.string());

	//	return generatedJSON && generatedBinary;
	//}

	//bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)
	//{
	//	for (auto& file : std::filesystem::recursive_directory_iterator(directory))
	//	{
	//		//UGH MAC - ignore .DS_Store
	//		if (std::filesystem::file_size(file.path()) <= 0 ||
	//			(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
	//				std::filesystem::path(file.path()).extension().string() != MPAK_EXT &&
	//				file.path().stem().string() != stemName))
	//		{
	//			continue;
	//		}

	//		if ((isBinary && std::filesystem::path(file.path()).extension().string() == MPAK_EXT) || 
	//			(!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT))
	//		{
	//			outPath = file.path().string();
	//			return true;
	//		}
	//	}

	//	return false;
	//}


	//@Temporary
	//bool GenerateMaterialParamsFromOldMaterialPack(const flat::Material* const materialData, const std::string& pathOfSource, const std::string& outputDir)
	//{
	//	//create the folder
	//	std::filesystem::path sourceFilePath = pathOfSource;

	//	std::filesystem::path paramsPackDir = outputDir / sourceFilePath.stem();

	//	std::filesystem::create_directory(paramsPackDir);

	//	//now convert the materialData to MaterialParams
	//	flatbuffers::FlatBufferBuilder builder;

	//	std::vector<flatbuffers::Offset<flat::MaterialULongParam>> ulongParams;
	//	std::vector<flatbuffers::Offset<flat::MaterialFloatParam>> floatParams;
	//	std::vector<flatbuffers::Offset<flat::MaterialColorParam>> colorParams;
	//	std::vector<flatbuffers::Offset<flat::MaterialTextureParam>> textureParams;

	//	ulongParams.push_back(flat::CreateMaterialULongParam(builder, flat::MaterialPropertyType_SHADER, materialData->shader()));

	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_ROUGHNESS, materialData->roughness()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_METALLIC, materialData->metallic()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_REFLECTANCE, materialData->reflectance()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_AMBIENT_OCCLUSION, materialData->ambientOcclusion()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_CLEAR_COAT, materialData->clearCoat()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS, materialData->clearCoatRoughness()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_HEIGHT_SCALE, materialData->heightScale()));
	//	floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_ANISOTROPY, materialData->anisotropy()));

	//	flat::Colour colour = { materialData->baseColor()->r(), materialData->baseColor()->g(), materialData->baseColor()->b(), materialData->baseColor()->a() };
	//	
	//	colorParams.push_back(flat::CreateMaterialColorParam(builder, flat::MaterialPropertyType_ALBEDO, &colour));

	//	if (materialData->diffuseTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_ALBEDO,
	//			materialData->diffuseTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->normalMapTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_NORMAL,
	//			materialData->normalMapTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}
	//	
	//	if (materialData->emissionTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_EMISSION,
	//			materialData->emissionTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->metallicTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_METALLIC,
	//			materialData->metallicTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->roughnessTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_ROUGHNESS,
	//			materialData->roughnessTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->aoTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_AMBIENT_OCCLUSION,
	//			materialData->aoTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->heightTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_HEIGHT,
	//			materialData->heightTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	if (materialData->anisotropyTexture())
	//	{
	//		textureParams.push_back(flat::CreateMaterialTextureParam(
	//			builder,
	//			flat::MaterialPropertyType_ANISOTROPY,
	//			materialData->anisotropyTexture(),
	//			flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
	//	}

	//	//create the material params
	//	const auto materialParams = flat::CreateMaterialParams(
	//		builder,
	//		materialData->name(),
	//		builder.CreateVector(ulongParams), 0,
	//		builder.CreateVector(floatParams),
	//		builder.CreateVector(colorParams),
	//		builder.CreateVector(textureParams), 0);

	//	//Write the temp file
	//	builder.Finish(materialParams);

	//	byte* buf = builder.GetBufferPointer();
	//	u32 size = builder.GetSize();

	//	std::string tempFile = (paramsPackDir / sourceFilePath.stem()).string() + ".bin";
	//	
	//	bool wroteTempFile = utils::WriteFile(tempFile, (char*)buf, size);

	//	std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

	//	char materialParamsSchemaPath[r2::fs::FILE_PATH_LENGTH];

	//	r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialParamsSchemaPath, MATERIAL_PARAMS_FBS.c_str());

	//	bool generatedJSON = flathelp::GenerateFlatbufferJSONFile(paramsPackDir.string(), materialParamsSchemaPath, tempFile);
	//	
	//	R2_CHECK(generatedJSON, "We didn't generate the JSON file from: %s", tempFile);

	//	std::filesystem::remove(tempFile);

	//	return generatedJSON;
	//}
	bool GenerateMaterialFromOldMaterialParamsPack(const flat::MaterialParams* const materialParamsData, const std::string& pathOfSource, const std::string& outputDir)
	{
		R2_CHECK(materialParamsData != nullptr, "Passed in nullptr for the materialParamsData!");

		std::filesystem::path sourceFilePath = pathOfSource;

		//For the engine materials: First we need to check to see if the string "Dynamic" or "Static" is at the start of the material name
		//if it is then we strip it off
		std::string materialName = sourceFilePath.stem().string();

		static const std::string STATIC_STRING = "Static";
		static const std::string DYNAMIC_STRING = "Dynamic";

		

		//for engine materials only
		if (materialName.find(STATIC_STRING, 0))
		{
			materialName = materialName.substr(STATIC_STRING.size());
		}
		else if (materialName.find(DYNAMIC_STRING, 0))
		{
			materialName = materialName.substr(DYNAMIC_STRING.size());
		}

		std::filesystem::path materialDir = std::filesystem::path(outputDir) / materialName;

		if (std::filesystem::exists(materialDir))
		{
			//we already made the material - don't do it again
			return true;
		}

		struct ShaderEffect
		{
			std::string shaderEffectName = "";
			u64 staticShader = 0;
			u64 dynamicShader = 0;
		};

		struct ShaderEffectPasses
		{
			ShaderEffect shaderEffectPasses[flat::eMeshPass::eMeshPass_NUM_SHADER_EFFECT_PASSES];
			bool isOpaque;
		};

		static ShaderEffect forwardOpaqueShaderEffect = {
			"forward_opaque_shader_effect",
			STRING_ID("Sandbox"),
			STRING_ID("AnimModel")
		};

		static ShaderEffect forwardTransparentShaderEffect = {
			"forward_transparent_shader_effect",
			STRING_ID("TransparentModel"),
			STRING_ID("TransparentAnimModel")
		};

		static ShaderEffect skyboxShaderEffect = {
			"default_skybox_shader_effect",
			STRING_ID("Skybox"),
			0
		};

		static ShaderEffect ellenEyeShaderEffect = {
			"ellen_eye_shader_effect",
			0,
			STRING_ID("EllenEyeShader")
		};

		static ShaderEffect debugShaderEffect = {
			"debug_shader_effect",
			STRING_ID("Debug"),
			0
		};

		static ShaderEffect debugTransparentShaderEffect = {
			"debug_transparent_shader_effect",
			STRING_ID("TransparentDebug"),
			0
		};

		static ShaderEffect debugModelShaderEffect = {
			"debug_model_shader_effect",
			STRING_ID("DebugModel"),
			0
		};

		static ShaderEffect debugModelTransparentShaderEffect = {
			"debug_model_transparent_shader_effect",
			STRING_ID("TransparentDebugModel"),
			0
		};

		static ShaderEffect depthShaderEffect = {
			"depth_shader_effect",
			STRING_ID("StaticDepth"),
			STRING_ID("DynamicDepth")
		};

		static ShaderEffect entityColorEffect = {
			"entity_color_effect",
			STRING_ID("StaticEntityColor"),
			STRING_ID("DynamicEntityColor")
		};

		static ShaderEffect defaultOutlineEffect = {
			"default_outline_effect",
			STRING_ID("StaticOutline"),
			STRING_ID("DynamicOutline")
		};

		static ShaderEffect directionShadowEffect = {
			"direction_shadow_effect",
			STRING_ID("StaticShadowDepth"),
			STRING_ID("DynamicShaderDepth")
		};

		static ShaderEffect screenCompositeEffect = {
			"screen_composite_effect",
			STRING_ID("Screen"),
			0
		};

		static ShaderEffect pointShadowEffect = {
			"point_shadow_effect",
			STRING_ID("PointLightStaticShadowDepth"),
			STRING_ID("PointLightDynamicShadowDepth")
		};

		static ShaderEffect spotLightShadowEffect = {
			"spotlight_shadow_effect",
			STRING_ID("SpotLightStaticShadowDepth"),
			STRING_ID("SpotLightDynamicShadowDepth")
		};


		static ShaderEffectPasses opaqueForwardPass = {
			{forwardOpaqueShaderEffect, {}, depthShaderEffect, directionShadowEffect, pointShadowEffect, spotLightShadowEffect},
			true
		};

		static ShaderEffectPasses transparentForwardPass = {
			{{}, forwardTransparentShaderEffect, {}, {}, {}, {}},
			false
		};

		static ShaderEffectPasses skyboxPass = {
			{skyboxShaderEffect, {}, {}, {}, {}, {}},
			true
		};

		static ShaderEffectPasses ellenEyePass = {
			{ellenEyeShaderEffect,{}, depthShaderEffect, {}, {}, {}},
			true
		};

		static ShaderEffectPasses debugOpaquePass = {
			{debugShaderEffect,{},{},{}, {}, {} },
			true
		};

		static ShaderEffectPasses debugTransparentPass = {
			{{}, debugTransparentShaderEffect, {}, {}, {}, {}},
			false
		};

		static ShaderEffectPasses debugModelOpaquePass = {
			{debugModelShaderEffect, {}, depthShaderEffect, {}, {}, {}},
			true
		};

		static ShaderEffectPasses debugModelTransparentPass = {
			{{},debugModelTransparentShaderEffect,{}, {}, {}, {}},
			false
		};

		static ShaderEffectPasses depthPass = {
			{{}, {}, depthShaderEffect, {}, {}, {}},
			true
		};

		static ShaderEffectPasses entityColorPass = {
			{entityColorEffect, {}, {}, {}, {}, {}},
			true
		};

		static ShaderEffectPasses defaultOutlinePass = {
			{defaultOutlineEffect, {}, {}, {}, {}, {}},
			true
		};

		static ShaderEffectPasses shadowDepthPass = {
			{{}, {}, {}, directionShadowEffect, {}, {}},
			true
		};

		static ShaderEffectPasses screenCompositePass = {
			{screenCompositeEffect, {}, {}, {}, {}, {}},
			true
		};

		static std::unordered_map<u64, ShaderEffectPasses> s_shaderAssetNameToShaderEffectPassesMap =
		{
			{STRING_ID("AnimModel"), opaqueForwardPass},
			{STRING_ID("Sandbox"), opaqueForwardPass},
			{STRING_ID("TransparentModel"), transparentForwardPass},
			{STRING_ID("TransparentAnimModel"), transparentForwardPass},
			{STRING_ID("Skybox"), skyboxPass},
			{STRING_ID("EllenEyeShader"), ellenEyePass},

			//@TODO(Serge): the internal renderer materials
			{STRING_ID("Debug"), debugOpaquePass},
			{STRING_ID("DebugModel"), debugModelOpaquePass},
			{STRING_ID("TransparentDebug"), debugTransparentPass},
			{STRING_ID("TransparentDebugModel"), debugModelTransparentPass},
			{STRING_ID("DynamicDepth"), depthPass},
			{STRING_ID("StaticDepth"), depthPass},
			{STRING_ID("DynamicEntityColor"), entityColorPass},
			{STRING_ID("StaticEntityColor"), entityColorPass},
			{STRING_ID("DynamicOutline"), defaultOutlinePass},
			{STRING_ID("StaticOutline"), defaultOutlinePass},
			{STRING_ID("DynamicShadowDepth"), shadowDepthPass},
			{STRING_ID("StaticShadowDepth"), shadowDepthPass},
			{STRING_ID("Screen"), screenCompositePass}
		};

		std::filesystem::create_directory(materialDir);

		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::ShaderULongParam>> shaderULongParams;
		std::vector<flatbuffers::Offset<flat::ShaderBoolParam>>  shaderBoolParams;
		std::vector<flatbuffers::Offset<flat::ShaderFloatParam>> shaderFloatParams;
		std::vector<flatbuffers::Offset<flat::ShaderColorParam>> shaderColorParams;
		std::vector<flatbuffers::Offset<flat::ShaderTextureParam>> shaderTextureParams;
		std::vector<flatbuffers::Offset<flat::ShaderStringParam>> shaderStringParams;
		std::vector<flatbuffers::Offset<flat::ShaderStageParam>> shaderStageParams;

		const flatbuffers::uoffset_t numULongParams = materialParamsData->ulongParams()->size();
		const flatbuffers::uoffset_t numBoolParams = materialParamsData->boolParams()->size();
		const flatbuffers::uoffset_t numFloatParams = materialParamsData->floatParams()->size();
		const flatbuffers::uoffset_t numColorParams = materialParamsData->colorParams()->size();
		const flatbuffers::uoffset_t numTextureParams = materialParamsData->textureParams()->size();
		const flatbuffers::uoffset_t numStringParams = materialParamsData->stringParams()->size();
		const flatbuffers::uoffset_t numShaderStageParams = materialParamsData->shaderParams()->size();

		u64 shaderName = 0;

		for (flatbuffers::uoffset_t i = 0; i < numULongParams; ++i)
		{
			const flat::MaterialULongParam* materialULongParam = materialParamsData->ulongParams()->Get(i);

			if (materialULongParam->propertyType() == flat::MaterialPropertyType::MaterialPropertyType_SHADER)
			{
				//don't bother with shaders though we'll need to use this later to create the shader effect data
				shaderName = materialULongParam->value();
				continue;
			}

			shaderULongParams.push_back(flat::CreateShaderULongParam(builder, static_cast<flat::ShaderPropertyType>(materialULongParam->propertyType()), materialULongParam->value()));
		}

		for (flatbuffers::uoffset_t i = 0; i < numBoolParams; ++i)
		{
			const flat::MaterialBoolParam* materialBoolParam = materialParamsData->boolParams()->Get(i);
			shaderBoolParams.push_back(flat::CreateShaderBoolParam(builder, static_cast<flat::ShaderPropertyType>(materialBoolParam->propertyType()), materialBoolParam->value()));
		}

		for (flatbuffers::uoffset_t i = 0; i < numFloatParams; ++i)
		{
			const flat::MaterialFloatParam* materialFloatParam = materialParamsData->floatParams()->Get(i);
			shaderFloatParams.push_back(flat::CreateShaderFloatParam(builder, static_cast<flat::ShaderPropertyType>(materialFloatParam->propertyType()), materialFloatParam->value()));
		}

		for (flatbuffers::uoffset_t i = 0; i < numColorParams; ++i)
		{
			const flat::MaterialColorParam* materialColorParam = materialParamsData->colorParams()->Get(i);
			shaderColorParams.push_back(flat::CreateShaderColorParam(builder, static_cast<flat::ShaderPropertyType>(materialColorParam->propertyType()), materialColorParam->value()));
		}

		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const flat::MaterialTextureParam* materialTextureParam = materialParamsData->textureParams()->Get(i);

			shaderTextureParams.push_back(flat::CreateShaderTextureParam(
				builder,
				static_cast<flat::ShaderPropertyType>(materialTextureParam->propertyType()),
				materialTextureParam->value(),
				static_cast<flat::ShaderPropertyPackingType>(materialTextureParam->packingType()),
				materialTextureParam->texturePackName(),
				builder.CreateString(materialTextureParam->texturePackNameStr()),
				materialTextureParam->minFilter(),
				materialTextureParam->magFilter(),
				materialTextureParam->anisotropicFiltering(),
				materialTextureParam->wrapS(),
				materialTextureParam->wrapT(),
				materialTextureParam->wrapR()));
		}

		for (flatbuffers::uoffset_t i = 0; i < numStringParams; ++i)
		{
			const flat::MaterialStringParam* materialStringParam = materialParamsData->stringParams()->Get(i);
			shaderStringParams.push_back(flat::CreateShaderStringParam(builder, static_cast<flat::ShaderPropertyType>(materialStringParam->propertyType()), builder.CreateString(materialStringParam->value())));
		}

		for (flatbuffers::uoffset_t i = 0; i < numShaderStageParams; ++i)
		{
			const flat::MaterialShaderParam* materialShaderParam= materialParamsData->shaderParams()->Get(i);
			shaderStageParams.push_back(flat::CreateShaderStageParam(builder, static_cast<flat::ShaderPropertyType>(materialShaderParam->propertyType()), materialShaderParam->shader(), materialShaderParam->shaderStageName(), builder.CreateString(materialShaderParam->value())));
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

		//@TODO(Serge): now we need to figure out the ShaderEffectPasses
		const auto& shaderEffectPasses = s_shaderAssetNameToShaderEffectPassesMap[shaderName];

		std::vector<flatbuffers::Offset<flat::ShaderEffect>> flatShaderEffects;

		for (s32 i = 0; i < flat::eMeshPass::eMeshPass_NUM_SHADER_EFFECT_PASSES; ++i)
		{
			const auto& effect = shaderEffectPasses.shaderEffectPasses[i];
			const auto effectName = effect.shaderEffectName;
			flatShaderEffects.push_back(flat::CreateShaderEffect(builder, STRING_ID(effectName.c_str()), builder.CreateString(effectName), effect.staticShader, effect.dynamicShader));
		}

		const auto flatShaderEffectPasses = flat::CreateShaderEffectPasses(builder, builder.CreateVector(flatShaderEffects));

		const auto flatMaterial = flat::CreateMaterial(
			builder,
			STRING_ID(materialName.c_str()),
			builder.CreateString(materialName),
			shaderEffectPasses.isOpaque ? flat::eTransparencyType_OPAQUE : flat::eTransparencyType_TRANSPARENT,
			flatShaderEffectPasses, shaderParams);

		builder.Finish(flatMaterial);


		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		std::string tempFile = (materialDir / sourceFilePath.stem()).string() + ".bin";
		
		bool wroteTempFile = utils::WriteFile(tempFile, (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialSchemaPath, MATERIAL_NAME_FBS.c_str());

		bool generatedJSON = flathelp::GenerateFlatbufferJSONFile(materialDir.string(), materialSchemaPath, tempFile);
		
		R2_CHECK(generatedJSON, "We didn't generate the JSON file from: %s", tempFile);

		std::filesystem::remove(tempFile);

		return generatedJSON;
	}


	bool RegenerateMaterialParamsPackManifest(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		return GenerateBinaryParamsPackManifest(binaryDir, binFilePath, rawFilePath);
	}

	bool GenerateMaterialParamsFromJSON(const std::string& outputDir, const std::string& path)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialParamsSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialParamsSchemaPath, MATERIAL_PARAMS_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, materialParamsSchemaPath, path);
	}

	bool GenerateMaterialParamsPackManifestFromJson(const std::string& jsonMaterialParamsPackManifestFilePath, const std::string& outputDir)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialParamsPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialParamsPackManifestSchemaPath, MATERIAL_PARAMS_PACK_MANIFEST_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, materialParamsPackManifestSchemaPath, jsonMaterialParamsPackManifestFilePath);
	}

	bool GenerateMaterialParamsPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		//the directory we get passed in is the top level directory for all of the materials in the pack

		//first generate all of the materials from JSON if they need to be generated
		for (const auto& materialParamsPackDir : std::filesystem::directory_iterator(rawDir))
		{
			if (materialParamsPackDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			//look for a json file in the raw dir
			std::filesystem::path jsonDir = materialParamsPackDir;

			//std::filesystem::path tempDir = std::filesystem::path(binaryDir) / materialParamsPackDir.path().stem();
			std::filesystem::path binDir = std::filesystem::path(binaryDir) / materialParamsPackDir.path().stem();

			char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
			//r2::fs::utils::SanitizeSubPath(tempDir.string().c_str(), sanitizedPath);

			//make the directory if we need it
			//std::filesystem::create_directory(sanitizedPath);

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
						std::filesystem::path(binFile.path()).extension().string() == MPRM_EXT)
					{
						found = true;
						break;
					}
				}

				if (found)
					continue;

				//we didn't find it so generate it
				bool generated = GenerateMaterialParamsFromJSON(std::string(sanitizedPath), jsonfile.path().string());
				R2_CHECK(generated, "Failed to generate the material params: %s", jsonfile.path().string().c_str());
			}
		}

		return GenerateBinaryParamsPackManifest(binaryDir, binFilePath, rawFilePath);
	}

	bool GenerateBinaryParamsPackManifest(const std::string& binaryDir, const std::string& binFilePath, const std::string& rawFilePath)
	{
		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::MaterialParams >> flatMaterialParams;

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
					std::filesystem::path(binFile.path()).extension().string() == MPRM_EXT)
				{
					packToRead = binFile.path().string();
					break;
				}
			}

			//read the file
			char* materialParamsData = utils::ReadFile(packToRead);

			const auto materialParams = flat::GetMaterialParams(materialParamsData);

			std::vector<flatbuffers::Offset<flat::MaterialULongParam>> ulongParams;
			std::vector<flatbuffers::Offset<flat::MaterialBoolParam>> boolParams;
			std::vector<flatbuffers::Offset<flat::MaterialFloatParam>> floatParams;
			std::vector<flatbuffers::Offset<flat::MaterialColorParam>> colorParams;
			std::vector<flatbuffers::Offset<flat::MaterialTextureParam>> textureParams;
			std::vector<flatbuffers::Offset<flat::MaterialStringParam>> stringParams;
			std::vector<flatbuffers::Offset<flat::MaterialShaderParam>> shaderParams;

			if (materialParams->ulongParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->ulongParams()->size(); ++i)
				{
					ulongParams.push_back(flat::CreateMaterialULongParam(builder, materialParams->ulongParams()->Get(i)->propertyType(), materialParams->ulongParams()->Get(i)->value()));
				}
			}

			if (materialParams->boolParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->boolParams()->size(); ++i)
				{
					const auto boolParam = materialParams->boolParams()->Get(i);
					boolParams.push_back(flat::CreateMaterialBoolParam(builder, boolParam->propertyType(), boolParam->value()));
				}
			}

			if (materialParams->floatParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->floatParams()->size(); ++i)
				{
					const auto floatParam = materialParams->floatParams()->Get(i);
					floatParams.push_back(flat::CreateMaterialFloatParam(builder, floatParam->propertyType(), floatParam->value()));
				}
			}

			if (materialParams->colorParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->colorParams()->size(); ++i)
				{
					const auto colorParam = materialParams->colorParams()->Get(i);
					colorParams.push_back(flat::CreateMaterialColorParam(builder, colorParam->propertyType(), colorParam->value()));
				}
			}

			if (materialParams->textureParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->textureParams()->size(); ++i)
				{
					const auto textureParam = materialParams->textureParams()->Get(i);

					std::string textureParamNameString = textureParam->texturePackNameStr()->str();

					textureParams.push_back(flat::CreateMaterialTextureParam(
						builder,
						textureParam->propertyType(),
						textureParam->value(),
						textureParam->packingType(),
						textureParam->texturePackName(),
						builder.CreateString(textureParamNameString),
						textureParam->minFilter(),
						textureParam->magFilter(),
						textureParam->anisotropicFiltering(),
						textureParam->wrapS(),
						textureParam->wrapT(),
						textureParam->wrapR()));
				}
			}

			if (materialParams->stringParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->stringParams()->size(); ++i)
				{
					const auto stringParam = materialParams->stringParams()->Get(i);
					stringParams.push_back(flat::CreateMaterialStringParam(builder, stringParam->propertyType(), builder.CreateString(stringParam->value())));
				}
			}

			if (materialParams->shaderParams())
			{
				for (flatbuffers::uoffset_t i = 0; i < materialParams->shaderParams()->size(); ++i)
				{
					const auto shaderParam = materialParams->shaderParams()->Get(i);
					shaderParams.push_back(flat::CreateMaterialShaderParam(builder, shaderParam->propertyType(), shaderParam->shader(), shaderParam->shaderStageName(), builder.CreateString(shaderParam->value())));
				}
			}

			auto pack = flat::CreateMaterialParams(
				builder,
				materialParams->name(),
				builder.CreateVector(ulongParams),
				builder.CreateVector(boolParams),
				builder.CreateVector(floatParams),
				builder.CreateVector(colorParams),
				builder.CreateVector(textureParams),
				builder.CreateVector(stringParams),
				builder.CreateVector(shaderParams));

			flatMaterialParams.push_back(pack);

			delete[] materialParamsData;
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest

		const auto materialPackName = r2::asset::Asset::GetAssetNameForFilePath(binPath.string().c_str(), r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);


		auto manifest = flat::CreateMaterialParamsPack(
			builder, materialPackName,
			builder.CreateVector(flatMaterialParams));

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(binPath.string(), (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialParamsPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialParamsPackManifestSchemaPath, MATERIAL_PARAMS_PACK_MANIFEST_FBS.c_str());

		std::filesystem::path jsonPath = rawFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), materialParamsPackManifestSchemaPath, binPath.string());

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binPath.parent_path().string(), materialParamsPackManifestSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}

	bool FindMaterialParamsPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			//UGH MAC - ignore .DS_Store
			if (std::filesystem::file_size(file.path()) <= 0 ||
				(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
					std::filesystem::path(file.path()).extension().string() != MPPK_EXT &&
					file.path().stem().string() != stemName))
			{
				continue;
			}

			if ((isBinary && std::filesystem::path(file.path()).extension().string() == MPPK_EXT) ||
				(!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT))
			{
				outPath = file.path().string();
				return true;
			}
		}

		return false;
	}

	/*std::vector<r2::draw::MaterialHandle> FindMaterialHandleForTexturePaths(const std::string& binFilePath, const std::vector<std::string>& texturePaths)
	{
		return {};
	}*/
}

#endif