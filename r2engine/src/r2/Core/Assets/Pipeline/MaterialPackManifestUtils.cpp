#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/Material_generated.h"
#include "r2/Render/Model/MaterialParams_generated.h"
#include "r2/Render/Model/MaterialParamsPack_generated.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Utils/Flags.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln
{
	const std::string MATERIAL_PACK_MANIFEST_NAME_FBS = "MaterialPack.fbs";
	const std::string MATERIAL_NAME_FBS = "Material.fbs";

	const std::string MATERIAL_PARAMS_FBS = "MaterialParams.fbs";
	const std::string MATERIAL_PARAMS_PACK_MANIFEST_FBS = "MaterialParamsPack.fbs";

	const std::string JSON_EXT = ".json";
	
	const std::string MPAK_EXT = ".mpak";
	const std::string MMAT_EXT = ".mmat";

	const std::string MPPK_EXT = ".mppk";
	const std::string MPRM_EXT = ".mprm";

	const std::string MATERIAL_JSON_DIR = "material_raw";
	const std::string MATERIAL_BIN_DIR = "material_bin";

	const bool GENERATE_PARAMS = false;

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

	bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		//the directory we get passed in is the top level directory for all of the materials in the pack
		
		char sanitizeRootMaterialParamsDirPath[r2::fs::FILE_PATH_LENGTH];

		if (GENERATE_PARAMS)
		{
			std::filesystem::path rootMaterialParamsDir = std::filesystem::path(rawDir).parent_path() / "params_packs";
			r2::fs::utils::SanitizeSubPath(rootMaterialParamsDir.string().c_str(), sanitizeRootMaterialParamsDirPath);
			std::filesystem::create_directory(sanitizeRootMaterialParamsDirPath);
		}

		//first generate all of the materials from JSON if they need to be generated
		for (const auto& materialPackDir : std::filesystem::directory_iterator(rawDir))
		{
			if (materialPackDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			//look for a json file in the raw dir
			std::filesystem::path jsonDir = materialPackDir / MATERIAL_JSON_DIR;

			std::filesystem::path tempDir = std::filesystem::path(binaryDir) / materialPackDir.path().stem();
			std::filesystem::path binDir = std::filesystem::path(binaryDir) / materialPackDir.path().stem() / MATERIAL_BIN_DIR;

			char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(tempDir.string().c_str(), sanitizedPath);

			//make the directory if we need it
			std::filesystem::create_directory(sanitizedPath);

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

				//check to see if we have the corresponding .mmat file
				bool found = false;
				for (const auto& binFile : std::filesystem::directory_iterator(sanitizedPath))
				{
					if (std::filesystem::file_size(binFile.path()) > 0 &&
						binFile.path().stem() == jsonfile.path().stem() &&
						std::filesystem::path(binFile.path()).extension().string() == MMAT_EXT)
					{
						found = true;
						break;
					}
				}

				if(found)
					continue;

				//we didn't find it so generate it
				bool generated = GenerateMaterialFromJSON(std::string(sanitizedPath), jsonfile.path().string());
				R2_CHECK(generated, "Failed to generate the material: %s", jsonfile.path().string().c_str());
			}
		}

		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::Material >> flatMaterials;

		//okay now we know that we have all of the material generated properly
		//so now we need to make the manifests by reading in all of the material.mmat files in the binary directory
		//we do that for each pack
		for (const auto& materialPackDir : std::filesystem::directory_iterator(binaryDir))
		{
			if (materialPackDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			//Get the path
			std::filesystem::path binDir = materialPackDir / MATERIAL_BIN_DIR;
			std::string packToRead = "";
			for (const auto& binFile : std::filesystem::directory_iterator(binDir))
			{
				if (std::filesystem::file_size(binFile.path()) > 0 &&
					std::filesystem::path(binFile.path()).extension().string() == MMAT_EXT)
				{
					packToRead = binFile.path().string();
					break;
				}
			}

			//read the file
			char* materialData = utils::ReadFile(packToRead);
			
			const auto material = flat::GetMaterial(materialData);

			if (GENERATE_PARAMS)
			{
				GenerateMaterialParamsFromOldMaterialPack(material, packToRead, sanitizeRootMaterialParamsDirPath);
			}

			flat::Color theColor{ material->baseColor()->r(), material->baseColor()->g(), material->baseColor()->b(), material->baseColor()->a() };

			auto packMaterial = flat::CreateMaterial(
				builder, material->name(),
				material->shader(),
				material->texturePackName(),
				material->diffuseTexture(),
				material->specularTexture(),
				material->normalMapTexture(),
				material->emissionTexture(),
				material->metallicTexture(),
				material->roughnessTexture(),
				material->aoTexture(),
				material->heightTexture(),
				material->anisotropyTexture(),
				&theColor,
				material->specular(),
				material->roughness(),
				material->metallic(),
				material->reflectance(),
				material->ambientOcclusion(),
				material->clearCoat(),
				material->clearCoatRoughness(),
				material->anisotropy(),
				material->heightScale());

			flatMaterials.push_back(packMaterial);

			delete[] materialData;
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest
		auto manifest = flat::CreateMaterialPack(builder, STRING_ID(binPath.stem().string().c_str()), builder.CreateVector(flatMaterials));

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

	bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			//UGH MAC - ignore .DS_Store
			if (std::filesystem::file_size(file.path()) <= 0 ||
				(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
					std::filesystem::path(file.path()).extension().string() != MPAK_EXT &&
					file.path().stem().string() != stemName))
			{
				continue;
			}

			if ((isBinary && std::filesystem::path(file.path()).extension().string() == MPAK_EXT) || 
				(!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT))
			{
				outPath = file.path().string();
				return true;
			}
		}

		return false;
	}


	//@Temporary
	bool GenerateMaterialParamsFromOldMaterialPack(const flat::Material* const materialData, const std::string& pathOfSource, const std::string& outputDir)
	{
		//create the folder
		std::filesystem::path sourceFilePath = pathOfSource;

		std::filesystem::path paramsPackDir = outputDir / sourceFilePath.stem();

		std::filesystem::create_directory(paramsPackDir);

		//now convert the materialData to MaterialParams
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::MaterialULongParam>> ulongParams;
		std::vector<flatbuffers::Offset<flat::MaterialFloatParam>> floatParams;
		std::vector<flatbuffers::Offset<flat::MaterialColorParam>> colorParams;
		std::vector<flatbuffers::Offset<flat::MaterialTextureParam>> textureParams;

		ulongParams.push_back(flat::CreateMaterialULongParam(builder, flat::MaterialPropertyType_SHADER, materialData->shader()));

		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_ROUGHNESS, materialData->roughness()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_METALLIC, materialData->metallic()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_REFLECTANCE, materialData->reflectance()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_AMBIENT_OCCLUSION, materialData->ambientOcclusion()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_CLEAR_COAT, materialData->clearCoat()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_CLEAR_COAT_ROUGHNESS, materialData->clearCoatRoughness()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_HEIGHT_SCALE, materialData->heightScale()));
		floatParams.push_back(flat::CreateMaterialFloatParam(builder, flat::MaterialPropertyType_ANISOTROPY, materialData->anisotropy()));

		flat::Colour colour = { materialData->baseColor()->r(), materialData->baseColor()->g(), materialData->baseColor()->b(), materialData->baseColor()->a() };
		
		colorParams.push_back(flat::CreateMaterialColorParam(builder, flat::MaterialPropertyType_ALBEDO, &colour));

		if (materialData->diffuseTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_ALBEDO,
				materialData->diffuseTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->normalMapTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_NORMAL,
				materialData->normalMapTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}
		
		if (materialData->emissionTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_EMISSION,
				materialData->emissionTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->metallicTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_METALLIC,
				materialData->metallicTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->roughnessTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_ROUGHNESS,
				materialData->roughnessTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->aoTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_AMBIENT_OCCLUSION,
				materialData->aoTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->heightTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_HEIGHT,
				materialData->heightTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		if (materialData->anisotropyTexture())
		{
			textureParams.push_back(flat::CreateMaterialTextureParam(
				builder,
				flat::MaterialPropertyType_ANISOTROPY,
				materialData->anisotropyTexture(),
				flat::MaterialPropertyPackingType_RGBA, materialData->texturePackName()));
		}

		//create the material params
		const auto materialParams = flat::CreateMaterialParams(
			builder,
			materialData->name(),
			builder.CreateVector(ulongParams), 0,
			builder.CreateVector(floatParams),
			builder.CreateVector(colorParams),
			builder.CreateVector(textureParams), 0);

		//Write the temp file
		builder.Finish(materialParams);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		std::string tempFile = (paramsPackDir / sourceFilePath.stem()).string() + ".bin";
		
		bool wroteTempFile = utils::WriteFile(tempFile, (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialParamsSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialParamsSchemaPath, MATERIAL_PARAMS_FBS.c_str());

		bool generatedJSON = flathelp::GenerateFlatbufferJSONFile(paramsPackDir.string(), materialParamsSchemaPath, tempFile);
		
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
			if (materialParamsPackDir.path().filename() == ".DS_Store")
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
					textureParams.push_back(flat::CreateMaterialTextureParam(
						builder,
						textureParam->propertyType(),
						textureParam->value(),
						textureParam->packingType(),
						textureParam->texturePackName(),
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

			auto pack = flat::CreateMaterialParams(
				builder,
				materialParams->name(),
				builder.CreateVector(ulongParams),
				builder.CreateVector(boolParams),
				builder.CreateVector(floatParams),
				builder.CreateVector(colorParams),
				builder.CreateVector(textureParams),
				builder.CreateVector(stringParams));

			flatMaterialParams.push_back(pack);

			delete[] materialParamsData;
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest
		auto manifest = flat::CreateMaterialParamsPack(
			builder, STRING_ID(binPath.stem().string().c_str()),
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
}

#endif