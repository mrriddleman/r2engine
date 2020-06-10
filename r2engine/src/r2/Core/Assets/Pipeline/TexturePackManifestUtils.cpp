#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Flags.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln::tex
{
	const std::string TEXTURE_PACK_MANIFEST_NAME_FBS = "TexturePackManifest.fbs";
	const std::string JSON_EXT = ".json";
	const std::string TMAN_EXT = ".tman";

	bool GenerateTexturePacksManifestFromJson(const std::string& texturePacksManifestFilePath)
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePackManifestSchemaPath, TEXTURE_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path p = texturePacksManifestFilePath;

		return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), texturePackManifestSchemaPath, texturePacksManifestFilePath);

	}

	bool GenerateTexturePacksManifestFromDirectories(const std::string& filePath, const std::string& parentDirectory)
	{
		flatbuffers::FlatBufferBuilder builder;
		u64 manifestTotalTextureSize = 0;
		u64 totalNumberOfTextures = 0;
		u64 maxNumTexturesInAPack = 0;
		std::vector < flatbuffers::Offset< flat::TexturePack >> texturePacks;

		for (const auto& texturePackDir : std::filesystem::directory_iterator(parentDirectory)) //this will be the texture pack level
		{
			//UGH MAC - ignore .DS_Store
			if (texturePackDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			u64 numTexturesInPack = 0;
			u64 packSize = 0;
			std::vector<flatbuffers::Offset<flatbuffers::String>> albedos;
			std::vector<flatbuffers::Offset<flatbuffers::String>> normals;
			std::vector<flatbuffers::Offset<flatbuffers::String>> speculars;
			std::vector<flatbuffers::Offset<flatbuffers::String>> emissives;
			std::vector<flatbuffers::Offset<flatbuffers::String>> metalics;
			std::vector<flatbuffers::Offset<flatbuffers::String>> occlusions;
			std::vector<flatbuffers::Offset<flatbuffers::String>> micros;
			std::vector<flatbuffers::Offset<flatbuffers::String>> heights;

			u64 packName = STRING_ID(texturePackDir.path().stem().string().c_str());

			for (const auto& file : std::filesystem::recursive_directory_iterator(texturePackDir.path().string()))
			{
				if(file.is_directory())
					continue;

				++numTexturesInPack;
				packSize += file.file_size();

				char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::SanitizeSubPath(file.path().string().c_str(), sanitizedPath);

				if (file.path().parent_path().stem().string() == "albedo")
				{
					albedos.push_back( builder.CreateString(sanitizedPath) );
				}
				else if (file.path().parent_path().stem().string() == "normal")
				{
					normals.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "specular")
				{
					speculars.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "emissive")
				{
					emissives.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "metalic")
				{
					metalics.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "occlusion")
				{
					occlusions.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "micro")
				{
					micros.push_back(builder.CreateString(sanitizedPath));
				}
				else if (file.path().parent_path().stem().string() == "height")
				{
					heights.push_back(builder.CreateString(sanitizedPath));
				}
			}

			//make the texture pack and add it to the vector
			auto texturePack = flat::CreateTexturePack(builder, packName,
				builder.CreateVector(albedos),
				builder.CreateVector(normals),
				builder.CreateVector(speculars),
				builder.CreateVector(emissives),
				builder.CreateVector(metalics),
				builder.CreateVector(occlusions),
				builder.CreateVector(micros),
				builder.CreateVector(heights), packSize, numTexturesInPack);

			texturePacks.push_back(texturePack);

			manifestTotalTextureSize += packSize;
			totalNumberOfTextures += numTexturesInPack;
			maxNumTexturesInAPack = std::max(maxNumTexturesInAPack, numTexturesInPack);
		}
		//add the texture packs to the manifest
 		auto manifest = flat::CreateTexturePacksManifest(builder, builder.CreateVector(texturePacks), totalNumberOfTextures, manifestTotalTextureSize, maxNumTexturesInAPack);

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(filePath, (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePacksManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePacksManifestSchemaPath, TEXTURE_PACK_MANIFEST_NAME_FBS.c_str());

		std::filesystem::path p = filePath;

		std::filesystem::path jsonPath = p.parent_path() / std::filesystem::path(p.stem().string() + JSON_EXT);

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(p.parent_path().string(), texturePacksManifestSchemaPath, filePath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), texturePacksManifestSchemaPath, jsonPath.string());

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