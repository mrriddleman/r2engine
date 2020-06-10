#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/Material_generated.h"
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
	const std::string JSON_EXT = ".json";
	const std::string MPAK_EXT = ".mpak";
	const std::string MMAT_EXT = ".mmat";
	const std::string MATERIAL_JSON_DIR = "material_raw";
	const std::string MATERIAL_BIN_DIR = "material_bin";

	bool GenerateMaterialFromJSON(const std::string& outputDir, const std::string& path)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialSchemaPath, MATERIAL_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, materialSchemaPath, path);
	}


	bool GenerateMaterialPackManifestFromJson(const std::string& materialPackManifestFilePath)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path p = materialPackManifestFilePath;

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), texturePackManifestSchemaPath, materialPackManifestFilePath);
	}

	bool GenerateMaterialPackManifestFromDirectories(const std::string& filePath, const std::string& directory)
	{
		//the directory we get passed in is the top level directory for all of the materials in the pack
		
		//first generate all of the materials from JSON if they need to be generated
		for (const auto& materialPackDir : std::filesystem::directory_iterator(directory))
		{
			if (materialPackDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			//look for a json file in the raw dir
			std::filesystem::path jsonDir = materialPackDir / MATERIAL_JSON_DIR;
			std::filesystem::path binDir = materialPackDir / MATERIAL_BIN_DIR;

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
				for (const auto& binFile : std::filesystem::directory_iterator(binDir))
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
				bool generated = GenerateMaterialFromJSON(binDir.string(), jsonfile.path().string());
				R2_CHECK(generated, "Failed to generate the material: %s", jsonfile.path().string().c_str());
			}
		}

		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::Material >> flatMaterials;

		//okay now we know that we have all of the material generated properly
		//so now we need to make the manifests by reading in all of the material.mmat files in the binary directory
		//we do that for each pack
		for (const auto& materialPackDir : std::filesystem::directory_iterator(directory))
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

			flat::Color theColor{ material->color()->r(), material->color()->g(), material->color()->b(), material->color()->a() };

			auto packMaterial = flat::CreateMaterial(builder, material->name(), material->shader(), &theColor, material->texturePackName());

			flatMaterials.push_back(packMaterial);

			delete[] materialData;
		}

		std::filesystem::path p = filePath;

		//add the texture packs to the manifest
		auto manifest = flat::CreateMaterialPack(builder, STRING_ID(p.stem().string().c_str()), builder.CreateVector(flatMaterials));

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(filePath, (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char materialPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), materialPackManifestSchemaPath, MATERIAL_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path jsonPath = p.parent_path() / std::filesystem::path(p.stem().string() + JSON_EXT);

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(p.parent_path().string(), materialPackManifestSchemaPath, filePath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), materialPackManifestSchemaPath, jsonPath.string());

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
}

#endif