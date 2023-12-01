#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"

#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Flags.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>
#include "assetlib/TextureMetaData_generated.h"
#include "assetlib/TextureAsset.h"
#include "assetlib/AssetFile.h"
#include "r2/Core/Assets/Asset.h"

namespace
{
	class RTEXAssetFile : public r2::assets::assetlib::AssetFile
	{
	public:
		RTEXAssetFile()
			:mFilePath("")
		{
		}

		~RTEXAssetFile()
		{
			Close();

			if (binaryBlob.data)
			{
				FreeForBlob(binaryBlob);
				binaryBlob.data = nullptr;
				binaryBlob.size = 0;
			}

			if (metaData.data)
			{
				FreeForBlob(metaData);
				metaData.data = nullptr;
				metaData.size = 0;
			}
		}

		virtual bool OpenForRead(const char* filePath) override
		{
			Close();

			mFile.open(filePath, std::ios::binary | std::ios::in);

			bool isOpen = mFile.is_open();

			if (isOpen)
			{
				mFilePath = filePath;
			}

			return isOpen;
		}

		virtual bool OpenForWrite(const char* filePath) const override
		{
			Close();

			mFile.open(filePath, std::ios::binary | std::ios::out);

			bool isOpen = mFile.is_open();

			if (isOpen)
			{
				mFilePath = filePath;
			}

			return isOpen;
		}

		virtual bool OpenForReadWrite(const char* filePath) override
		{
			Close();

			mFile.open(filePath, std::ios::binary | std::ios::out | std::ios::in);

			bool isOpen = mFile.is_open();

			if (isOpen)
			{
				mFilePath = filePath;
			}

			return isOpen;
		}

		virtual bool Close() const override
		{
			if (IsOpen())
			{
				mFile.close();
				mFilePath = "";
			}

			return true;
		}

		virtual bool IsOpen() const override
		{
			return mFile.is_open();
		}

		virtual uint32_t Size() override
		{
			if (!IsOpen())
				return 0;

			auto currentOffset = mFile.tellg();

			mFile.seekg(0, std::ios::end);

			std::streampos fSize = mFile.tellg();

			mFile.seekg(currentOffset);

			return fSize;
		}

		virtual uint32_t Read(char* data, uint32_t dataBufSize) override
		{
			mFile.read(data, dataBufSize);
			return dataBufSize;
		}

		virtual uint32_t Read(char* data, uint32_t offset, uint32_t dataBufSize) override
		{
			mFile.seekg(offset, std::ios::beg);
			mFile.read(data, dataBufSize);
			return dataBufSize;
		}

		virtual uint32_t Write(const char* data, uint32_t size) const override
		{
			mFile.write(data, size);
			return size;
		}

		virtual bool AllocateForBlob(r2::assets::assetlib::BinaryBlob& blob) override
		{
			if (blob.size > 0)
				blob.data = new char[blob.size];

			return blob.size > 0;
		}

		virtual void FreeForBlob(const r2::assets::assetlib::BinaryBlob& blob) override
		{
			if (blob.data)
			{
				delete[] blob.data;
			}
		}

		virtual const char* FilePath() const override
		{
			return mFilePath.c_str();
		}

	private:
		mutable std::string mFilePath;
		mutable std::fstream mFile;
	};
}

namespace r2::asset::pln::tex
{
	const std::string TEXTURE_PACK_MANIFEST_NAME_FBS = "TexturePackManifest.fbs";
	const std::string TEXTURE_PACK_META_DATA_NAME_FBS = "TexturePackMetaData.fbs";
	const std::string JSON_EXT = ".json";
	const std::string TMAN_EXT = ".tman";
	const std::string PACKS_DIR = "packs";
	const std::string RTEX_EXT = ".rtex";
	

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

	std::filesystem::path GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir)
	{
		std::vector<std::filesystem::path> pathsSeen;

		std::filesystem::path thePath = inputPath;
		thePath.make_preferred();

		std::filesystem::path outputFilenamePath = "";

		if (inputPath.has_extension())
		{
			thePath = inputPath.parent_path();

			outputFilenamePath = inputPath.filename().replace_extension(RTEX_EXT);
		}

		bool found = false;

		while (!found && !thePath.empty() && thePath != thePath.root_path())
		{
			if (inputPathRootDir == thePath)
			{
				found = true;
				break;
			}

			pathsSeen.push_back(thePath.stem());
			thePath = thePath.parent_path();
		}

		if (!found)
		{
			R2_CHECK(false, "We couldn't get the output path!");
			return "";
		}

		std::filesystem::path output = outputDir;

		auto rIter = pathsSeen.rbegin();

		for (; rIter != pathsSeen.rend(); ++rIter)
		{
			output = output / *rIter;
		}

		if (!outputFilenamePath.empty())
		{
			output = output / outputFilenamePath;
		}

		return output;
	}

	struct RawFormatMetaData
	{
		flat::TextureFormat textureFormat;
		u32 maxWidth;
		u32 maxHeight;
		u32 maxMips;
		u32 numTextures;
		bool isCubemap;
		bool isAnisotropic;
	};


	s64 HasTextureFormat(const std::vector<RawFormatMetaData>& metaData, const flat::TextureFormat& format, bool isCubemap)
	{
		for (u32 i = 0; i < metaData.size(); ++i)
		{
			if (metaData[i].textureFormat == format && metaData[i].isCubemap == isCubemap)
				return i;
		}

		return -1;
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

		std::filesystem::path inputRootDir = directory;

		std::vector<RawFormatMetaData> rawFormatMetaData;


	
		std::filesystem::path binMetaFilePath;

		for (const auto& texturePackDir : std::filesystem::directory_iterator(directory)) //this will be the texture pack level
		{
			
			if (texturePackDir.path().filename() == ".DS_Store" || //UGH MAC - ignore .DS_Store
				texturePackDir.path().stem().string() == "hdr" ||
				texturePackDir.is_regular_file()) //skip the hdr directory since we don't use it directly
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

				binMetaFilePath = metaFilePath;

			}

			

			char* metaData = r2::asset::pln::utils::ReadFile(binMetaFilePath.string().c_str());
			R2_CHECK(metaData != nullptr, "Should have read the file: %s!\n", binMetaFilePath.string().c_str());
			const flat::TexturePackMetaData* texturePackMetaData = flat::GetTexturePackMetaData(metaData);
			R2_CHECK(texturePackMetaData != nullptr, "");

			bool isCubemap = texturePackMetaData->type() == flat::TextureType_CUBEMAP;
			u32 numMips = std::max((u32)1,(u32)texturePackMetaData->mipLevels()->size());
			bool isAnisotropic = false;
			bool hasIncrementedForCubemap = false;


			


			u64 numTexturesInPack = 0;
			u64 packSize = 0;
			std::vector<flatbuffers::Offset<flatbuffers::String>> albedos;
			std::vector<flatbuffers::Offset<flatbuffers::String>> normals;
			std::vector<flatbuffers::Offset<flatbuffers::String>> emissives;
			std::vector<flatbuffers::Offset<flatbuffers::String>> metallics;
			std::vector<flatbuffers::Offset<flatbuffers::String>> occlusions;
			std::vector<flatbuffers::Offset<flatbuffers::String>> details;
			std::vector<flatbuffers::Offset<flatbuffers::String>> heights;
			std::vector<flatbuffers::Offset<flatbuffers::String>> anisotropys;
			std::vector<flatbuffers::Offset<flatbuffers::String>> roughnesses;
			std::vector<flatbuffers::Offset<flatbuffers::String>> clearCoats;
			std::vector<flatbuffers::Offset<flatbuffers::String>> clearCoatRoughnesses;
			std::vector<flatbuffers::Offset<flatbuffers::String>> clearCoatNormals;


			std::vector<std::string> cubemapTexturePaths;

			u64 packName = r2::asset::Asset::GetAssetNameForFilePath(texturePackDir.path().stem().string().c_str(), r2::asset::TEXTURE_PACK);

			for (const auto& file : std::filesystem::recursive_directory_iterator(texturePackDir.path().string()))
			{
				if(file.is_directory() || file.path().extension() == JSON_EXT || file.path().parent_path().stem().string() == "hdr")
					continue;

				std::filesystem::path outputFilePath = GetOutputFilePath(file.path(), inputRootDir, binPacksPath);

				R2_CHECK(std::filesystem::exists(outputFilePath), "This should already exist right now");

				//@TODO(Serge): read the .rtex file to get the info out of it
				RTEXAssetFile rtexAssetFile;

				r2::assets::assetlib::load_meta_data(outputFilePath.string().c_str(), rtexAssetFile);

				const flat::TextureMetaData* textureMetaData = r2::assets::assetlib::read_texture_meta_data(rtexAssetFile);

				R2_CHECK(textureMetaData != nullptr, "We should have the meta data here!");


				s64 index = HasTextureFormat(rawFormatMetaData, textureMetaData->textureFormat(), isCubemap);

				if (index < 0)
				{
					RawFormatMetaData rawMetaData;

					rawMetaData.isAnisotropic = isAnisotropic;
					rawMetaData.isCubemap = isCubemap;
					rawMetaData.textureFormat = textureMetaData->textureFormat();
					rawMetaData.maxMips = numMips;

					rawMetaData.numTextures = 1;
					rawMetaData.maxWidth = textureMetaData->mips()->Get(0)->width();
					rawMetaData.maxHeight = textureMetaData->mips()->Get(0)->height();

					rawFormatMetaData.push_back(rawMetaData);

					if(isCubemap)
						hasIncrementedForCubemap = true;
				}
				else
				{
					RawFormatMetaData& rawMetaData = rawFormatMetaData.at(index);

					rawMetaData.maxWidth = std::max(rawMetaData.maxWidth, textureMetaData->mips()->Get(0)->width());
					rawMetaData.maxHeight = std::max(rawMetaData.maxHeight, textureMetaData->mips()->Get(0)->height());
					rawMetaData.maxMips = std::max(rawMetaData.maxMips, numMips);

					if ((rawMetaData.isCubemap && !hasIncrementedForCubemap) || !rawMetaData.isCubemap)
					{
						if (rawMetaData.isCubemap && !hasIncrementedForCubemap)
							hasIncrementedForCubemap = true;

						rawMetaData.numTextures++;
					}
				}

				packSize += std::filesystem::file_size(outputFilePath);

				char sanitizedPath[r2::fs::FILE_PATH_LENGTH];

				r2::fs::utils::SanitizeSubPath(outputFilePath.string().c_str(), sanitizedPath);

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
				else if (file.path().parent_path().stem().string() == "detail")
				{
					details.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "height")
				{
					heights.push_back(builder.CreateString(sanitizedPath));

					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "anisotropy")
				{
					anisotropys.push_back(builder.CreateString(sanitizedPath));
					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "roughness")
				{
					roughnesses.push_back(builder.CreateString(sanitizedPath));
					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "clearCoat")
				{
					roughnesses.push_back(builder.CreateString(sanitizedPath));
					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "clearCoatRoughness")
				{
					roughnesses.push_back(builder.CreateString(sanitizedPath));
					++numTexturesInPack;
				}
				else if (file.path().parent_path().stem().string() == "clearCoatNormal")
				{
					roughnesses.push_back(builder.CreateString(sanitizedPath));
					++numTexturesInPack;
				}

			}

		//	char* texturePackMetaData = utils::ReadFile(metaFilePath.string());

		//	const auto packMetaData = flat::GetTexturePackMetaData(texturePackMetaData);

			auto textureType = texturePackMetaData->type();

			std::vector<flatbuffers::Offset<flat::MipLevel>> cubemapMipLevels;

			if (texturePackMetaData->mipLevels())
			{
				auto numMipLevels = texturePackMetaData->mipLevels()->size();

				for (flatbuffers::uoffset_t m = 0; m < numMipLevels; ++m)
				{
					const auto& mip = texturePackMetaData->mipLevels()->Get(m);

					flatbuffers::uoffset_t numSides = mip->sides()->size();

					std::vector<flatbuffers::Offset<flat::CubemapSideEntry>> sides;

					for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
					{
						std::string path;
						for (const auto& cubemapSide : cubemapTexturePaths)
						{
							std::filesystem::path cubemapSidePath = cubemapSide;
							auto sideImageName = mip->sides()->Get(i)->textureName()->str();
							auto stemName = cubemapSidePath.filename().string();
							if (stemName == sideImageName)
							{
								path = cubemapSidePath.string();
								break;
							}
						}

						sides.push_back(flat::CreateCubemapSideEntry(builder, builder.CreateString(path), mip->sides()->Get(i)->side()));
					}

					cubemapMipLevels.push_back(flat::CreateMipLevel(builder, m, builder.CreateVector(sides)));
				}
			}

			//make the texture pack and add it to the vector
			auto texturePack = flat::CreateTexturePack(builder, packName,
				builder.CreateVector(albedos),
				builder.CreateVector(normals),
				builder.CreateVector(emissives),
				builder.CreateVector(metallics),
				builder.CreateVector(occlusions),
				builder.CreateVector(roughnesses),
				builder.CreateVector(heights),
				builder.CreateVector(anisotropys),
				builder.CreateVector(details),
				builder.CreateVector(clearCoats),
				builder.CreateVector(clearCoatRoughnesses),
				builder.CreateVector(clearCoatNormals),
				packSize, numTexturesInPack,
				CreateTexturePackMetaData(builder, textureType, builder.CreateVector(cubemapMipLevels)));

			texturePacks.push_back(texturePack);

			manifestTotalTextureSize += packSize;
			totalNumberOfTextures += numTexturesInPack;
			maxNumTexturesInAPack = std::max(maxNumTexturesInAPack, numTexturesInPack);

			delete[] metaData;
		}

		std::vector<flatbuffers::Offset<flat::FormatMetaData>> formatMetaData;

		for (const auto& rawFormat : rawFormatMetaData)
		{
			formatMetaData.push_back(flat::CreateFormatMetaData(builder, rawFormat.textureFormat, rawFormat.maxWidth, rawFormat.maxHeight, rawFormat.numTextures, rawFormat.maxMips, rawFormat.isCubemap, rawFormat.isAnisotropic));
		}

		//add the texture packs to the manifest
 		auto manifest = flat::CreateTexturePacksManifest(builder, builder.CreateVector(texturePacks), totalNumberOfTextures, manifestTotalTextureSize, maxNumTexturesInAPack, builder.CreateVector(formatMetaData));

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

	bool HasTexturePackInManifestFile(const std::string& manifestFilePath, const std::string& packName)
	{
		char* manifestFileData = utils::ReadFile(manifestFilePath);

		if (!manifestFileData)
		{
			R2_CHECK(false, "We couldn't read the manifest file path: %s", manifestFilePath.c_str());
			return false;
		}


		auto packNameStringID = r2::asset::Asset::GetAssetNameForFilePath(packName.c_str(), r2::asset::TEXTURE_PACK);

		const flat::TexturePacksManifest* manifest = flat::GetTexturePacksManifest((const void*)manifestFileData);

		R2_CHECK(manifest != nullptr, "We couldn't make the texture pack manifest data!");

		auto numTexturePacks = manifest->texturePacks()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const auto texturePack = manifest->texturePacks()->Get(i);

			if (texturePack->packName() == packNameStringID)
			{
				delete[] manifestFileData;
				return true;
			}
		}

		delete[] manifestFileData;
		return false;
	}

	bool HasTexturePathInManifestFile(const std::string& manifestFilePath, const std::string& packName, const std::string& filePath)
	{
		char* manifestFileData = utils::ReadFile(manifestFilePath);

		if (!manifestFileData)
		{
			R2_CHECK(false, "We couldn't read the manifest file path: %s", manifestFilePath.c_str());
			return false;
		}


		auto packNameStringID = r2::asset::Asset::GetAssetNameForFilePath(packName.c_str(), r2::asset::TEXTURE_PACK);

		const flat::TexturePacksManifest* manifest = flat::GetTexturePacksManifest((const void*)manifestFileData);

		R2_CHECK(manifest != nullptr, "We couldn't make the texture pack manifest data!");

		auto numTexturePacks = manifest->texturePacks()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const auto texturePack = manifest->texturePacks()->Get(i);

			if (texturePack->packName() == packNameStringID)
			{
				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->albedo()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->albedo()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->normal()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->normal()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->metallic()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->metallic()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->roughness()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->roughness()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->occlusion()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->occlusion()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->emissive()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->emissive()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->anisotropy()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->anisotropy()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->height()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->height()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->detail()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->detail()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoat()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->clearCoat()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoatRoughness()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->clearCoatRoughness()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}

				for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoatNormal()->size(); ++filePathIndex)
				{
					if (std::filesystem::path(texturePack->clearCoatNormal()->Get(filePathIndex)->str()) == std::filesystem::path(filePath))
					{
						delete[] manifestFileData;
						return  true;
					}
				}
			}
		}

		delete[] manifestFileData;

		return false;
	}

	/*std::vector<std::vector<std::string>> GetAllTexturesInTexturePack(const std::string& manifestFilePath, const std::string& packName)
	{
		std::vector<std::vector<std::string>> texturesInPack;
		texturesInPack.resize(r2::draw::tex::TextureType::NUM_TEXTURE_TYPES);

		char* manifestFileData = utils::ReadFile(manifestFilePath);

		if (!manifestFileData)
		{
			R2_CHECK(false, "We couldn't read the manifest file path: %s", manifestFilePath.c_str());
			return texturesInPack;
		}

		auto packNameStringID = r2::asset::Asset::GetAssetNameForFilePath(packName.c_str(), r2::asset::TEXTURE_PACK);

		const flat::TexturePacksManifest* manifest = flat::GetTexturePacksManifest((const void*)manifestFileData);

		R2_CHECK(manifest != nullptr, "We couldn't make the texture pack manifest data!");

		auto numTexturePacks = manifest->texturePacks()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const auto texturePack = manifest->texturePacks()->Get(i);

			if (texturePack->packName() == packNameStringID)
			{
				if (texturePack->albedo()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->albedo()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Diffuse].push_back(texturePack->albedo()->Get(filePathIndex)->str());
					}
				}

				if (texturePack->normal()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->normal()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Normal].push_back(texturePack->normal()->Get(filePathIndex)->str());
					}
				}

				
				if (texturePack->metallic()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->metallic()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Metallic].push_back(texturePack->metallic()->Get(filePathIndex)->str());
					}
				}
				
				if (texturePack->roughness()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->roughness()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Roughness].push_back(texturePack->roughness()->Get(filePathIndex)->str());
					}
				}
				
				if (texturePack->occlusion()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->occlusion()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Occlusion].push_back(texturePack->occlusion()->Get(filePathIndex)->str());
					}
				}

				if (texturePack->emissive()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->emissive()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Emissive].push_back(texturePack->emissive()->Get(filePathIndex)->str());
					}
				}

				if (texturePack->anisotropy()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->anisotropy()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Anisotropy].push_back(texturePack->anisotropy()->Get(filePathIndex)->str());
					}
				}

				
				if (texturePack->height()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->height()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Height].push_back(texturePack->height()->Get(filePathIndex)->str());
					}
				}
				
				if (texturePack->detail()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->detail()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::Detail].push_back(texturePack->detail()->Get(filePathIndex)->str());
					}
				}
				
				if (texturePack->clearCoat()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoat()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::ClearCoat].push_back(texturePack->clearCoat()->Get(filePathIndex)->str());
					}
				}
				
				if (texturePack->clearCoatRoughness()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoatRoughness()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::ClearCoatRoughness].push_back(texturePack->clearCoatRoughness()->Get(filePathIndex)->str());
					}
				}

				if (texturePack->clearCoatNormal()->size() > 0)
				{
					for (flatbuffers::uoffset_t filePathIndex = 0; filePathIndex < texturePack->clearCoatNormal()->size(); ++filePathIndex)
					{
						texturesInPack[r2::draw::tex::TextureType::ClearCoatNormal].push_back(texturePack->clearCoatNormal()->Get(filePathIndex)->str());
					}
				}
			}
		}

		delete[] manifestFileData;
		return texturesInPack;
	}*/


	void MakeTexturePackMetaFileFromFlat(const flat::TexturePackMetaData* texturePackMetaData, TexturePackMetaFile& metaFile)
	{
		metaFile.type = texturePackMetaData->type();
		metaFile.desiredMipLevels = texturePackMetaData->desiredMipLevels();
		metaFile.filter = texturePackMetaData->mipMapFilter();

		for (flatbuffers::uoffset_t i = 0; i < texturePackMetaData->mipLevels()->size(); ++i)
		{
			r2::asset::pln::tex::TexturePackMetaFileMipLevel mipLevel;

			auto flatMipLevel = texturePackMetaData->mipLevels()->Get(i);

			mipLevel.level = flatMipLevel->level();

			for (flatbuffers::uoffset_t j = 0; j < flatMipLevel->sides()->size(); ++j)
			{
				auto flatNextSide = flatMipLevel->sides()->Get(j);

				mipLevel.sides.push_back({ flatNextSide->textureName()->str(), flatNextSide->side() });
			}

			metaFile.mipLevels.push_back(mipLevel); 
		}
	}

	bool GenerateTexturePackMetaJSONFile(const std::filesystem::path& outputDir, const TexturePackMetaFile& metaFile)
	{
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::MipLevel>> mipLevels;

		for (u32 i = 0; i < metaFile.mipLevels.size(); ++i)
		{
			std::vector<flatbuffers::Offset<flat::CubemapSideEntry>> sideEntries;

			for (u32 j = 0; j < metaFile.mipLevels[i].sides.size(); ++j)
			{
				sideEntries.push_back( flat::CreateCubemapSideEntry(builder, builder.CreateString(metaFile.mipLevels[i].sides[j].textureName), metaFile.mipLevels[i].sides[j].cubemapSide) );
			}

			mipLevels.push_back( flat::CreateMipLevel(builder, metaFile.mipLevels[i].level, builder.CreateVector(sideEntries)) );
		}


		//add the texture packs to the manifest
		auto manifest = flat::CreateTexturePackMetaData(builder, metaFile.type, builder.CreateVector(mipLevels), metaFile.desiredMipLevels, metaFile.filter);

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		std::filesystem::path binaryPath = outputDir / "meta.bin";

		utils::WriteFile(binaryPath.string(), (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char texturePacksManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), texturePacksManifestSchemaPath, TEXTURE_PACK_META_DATA_NAME_FBS.c_str());

		std::filesystem::path jsonPath = outputDir / "meta.json";

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(outputDir.string(), texturePacksManifestSchemaPath, binaryPath.string());

		if (std::filesystem::exists(binaryPath))
		{
			std::filesystem::remove(binaryPath);
		}
	
		return generatedJSON;
	}

}



#endif