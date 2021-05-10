#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Flags.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln::tex
{
	const std::string TEXTURE_PACK_MANIFEST_NAME_FBS = "TexturePackManifest.fbs";
	const std::string TEXTURE_PACK_META_DATA_NAME_FBS = "TexturePackMetaData.fbs";
	const std::string JSON_EXT = ".json";
	const std::string TMAN_EXT = ".tman";
	const std::string PACKS_DIR = "packs";
	

	bool GenerateTexturePackMetaDataFromJSON(const std::string& jsonFile, const std::string& outputDir)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, TEXTURE_PACK_META_DATA_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, texturePackManifestSchemaPath, jsonFile);
	}

	bool GenerateTexturePacksManifestFromJson(const std::string& jsonManifestFilePath, const std::string& outputDir)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, TEXTURE_PACK_MANIFEST_NAME_FBS.c_str());

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(outputDir, texturePackManifestSchemaPath, jsonManifestFilePath);
	}

	bool GenerateTexturePacksManifestFromDirectories(const std::string& binFilePath, const std::string& jsonFilePath, const std::string& directory, const std::string& binDir)
	{
		flatbuffers::FlatBufferBuilder builder;
		u64 manifestTotalTextureSize = 0;
		u64 totalNumberOfTextures = 0;
		u64 maxNumTexturesInAPack = 0;
		std::vector < flatbuffers::Offset< flat::TexturePack >> texturePacks;

		std::filesystem::path binPacksPath = std::filesystem::path(binDir) / PACKS_DIR;

		std::filesystem::create_directory(binPacksPath);

		for (const auto& texturePackDir : std::filesystem::directory_iterator(directory)) //this will be the texture pack level
		{
			
			if (texturePackDir.path().filename() == ".DS_Store" || //UGH MAC - ignore .DS_Store
				texturePackDir.path().stem().string() == "hdr") //skip the hdr directory since we don't use it directly
			{
				continue;
			}

			std::filesystem::path texturePackBinPath = binPacksPath / texturePackDir.path().stem();
			std::filesystem::path metaFilePath = texturePackBinPath / "meta.tmet";

			//create the corresponding folder in the bin dir
			if (texturePackDir.is_directory())
			{
				std::filesystem::create_directory(texturePackBinPath);

				//we need to do something similar to what we do with the materials
				//-> generate the corresponding tmet file if it does not exist based on the json file

				if (!std::filesystem::exists(metaFilePath))
				{
					//make sure we have the .json file
					std::filesystem::path metaJsonFilePath = texturePackDir.path() / "meta.json";

					if (!std::filesystem::exists(metaJsonFilePath))
					{
						R2_CHECK(false, "We require all texture packs to have a meta data file, we don't have one for: %s\n", texturePackDir.path().c_str());
						return false;
					}

					bool generated = GenerateTexturePackMetaDataFromJSON(metaJsonFilePath.string(), texturePackBinPath.string());

					R2_CHECK(generated, "We couldn't generate the .tmet file for: %s\n", metaJsonFilePath.string().c_str());
				}

			}

			

			u64 numTexturesInPack = 0;
			u64 packSize = 0;
			std::vector<flatbuffers::Offset<flatbuffers::String>> albedos;
			std::vector<flatbuffers::Offset<flatbuffers::String>> normals;
			std::vector<flatbuffers::Offset<flatbuffers::String>> speculars;
			std::vector<flatbuffers::Offset<flatbuffers::String>> emissives;
			std::vector<flatbuffers::Offset<flatbuffers::String>> metallics;
			std::vector<flatbuffers::Offset<flatbuffers::String>> occlusions;
			std::vector<flatbuffers::Offset<flatbuffers::String>> micros;
			std::vector<flatbuffers::Offset<flatbuffers::String>> heights;
			std::vector<std::string> cubemapTexturePaths;

			u64 packName = STRING_ID(texturePackDir.path().stem().string().c_str());

			for (const auto& file : std::filesystem::recursive_directory_iterator(texturePackDir.path().string()))
			{
				if(file.is_directory())
					continue;

				
				packSize += file.file_size();

				char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::SanitizeSubPath(file.path().string().c_str(), sanitizedPath);

				if (file.path().parent_path().stem().string() == "albedo")
				{
					albedos.push_back( builder.CreateString(sanitizedPath) );
					cubemapTexturePaths.push_back(sanitizedPath);

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "normal")
				{
					normals.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "specular")
				{
					speculars.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "emissive")
				{
					emissives.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "metallic")
				{
					metallics.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "occlusion")
				{
					occlusions.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "micro")
				{
					micros.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "height")
				{
					heights.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
			}

			char* texturePackMetaData = utils::ReadFile(metaFilePath.string());

			const auto packMetaData = flat::GetTexturePackMetaData(texturePackMetaData);

			auto textureType = packMetaData->type();

			flatbuffers::Offset<flat::CubemapMetaData> cubemapMetaData = 0;

			if (packMetaData->cubemapMetaData())
			{
				flatbuffers::uoffset_t numSides = packMetaData->cubemapMetaData()->sides()->size();

				
				std::vector<flatbuffers::Offset<flat::CubemapSideEntry>> sides;

				for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
				{
					std::string path;
					for (const auto& cubemapSide : cubemapTexturePaths)
					{
						std::filesystem::path cubemapSidePath = cubemapSide;
						auto sideImageName = packMetaData->cubemapMetaData()->sides()->Get(i)->textureName()->str();
						auto stemName = cubemapSidePath.filename().string();
						if (stemName == sideImageName)
						{
							path = cubemapSidePath.string();
							break;
						}
					}

					sides.push_back(flat::CreateCubemapSideEntry(builder, builder.CreateString(path), packMetaData->cubemapMetaData()->sides()->Get(i)->side()));
				}

				cubemapMetaData = flat::CreateCubemapMetaData(builder, builder.CreateVector(sides));
			}

			//make the texture pack and add it to the vector
			auto texturePack = flat::CreateTexturePack(builder, packName,
				builder.CreateVector(albedos),
				builder.CreateVector(normals),
				builder.CreateVector(speculars),
				builder.CreateVector(emissives),
				builder.CreateVector(metallics),
				builder.CreateVector(occlusions),
				builder.CreateVector(micros),
				builder.CreateVector(heights), packSize, numTexturesInPack,
				CreateTexturePackMetaData(builder, textureType, cubemapMetaData));

			texturePacks.push_back(texturePack);

			manifestTotalTextureSize += packSize;
			totalNumberOfTextures += numTexturesInPack;
			maxNumTexturesInAPack = std::max(maxNumTexturesInAPack, numTexturesInPack);

			delete[] texturePackMetaData;
		}
		//add the texture packs to the manifest
 		auto manifest = flat::CreateTexturePacksManifest(builder, builder.CreateVector(texturePacks), totalNumberOfTextures, manifestTotalTextureSize, maxNumTexturesInAPack);

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(binFilePath, (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePacksManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePacksManifestSchemaPath, TEXTURE_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path binaryPath = binFilePath;

		std::filesystem::path jsonPath = jsonFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), texturePacksManifestSchemaPath, binFilePath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binaryPath.parent_path().string(), texturePacksManifestSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;

	}

	bool FindTexturePacksManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			//UGH MAC - ignore .DS_Store
			if (std::filesystem::file_size(file.path()) <= 0 || 
				(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
				 std::filesystem::path(file.path()).extension().string() != TMAN_EXT &&
				 file.path().stem().string() != stemName))
			{
				continue;
			}

			if ((isBinary && std::filesystem::path(file.path()).extension().string() == TMAN_EXT) || (!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT))
			{
				outPath = file.path().string();
				return true;
			}
		}

		return false;
	}

}



#endif