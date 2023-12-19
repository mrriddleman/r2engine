//
//  SoundDefinitionUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-25.
//

#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "SoundDefinitionUtils.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"

#include "r2/Core/File/PathUtils.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include <filesystem>


namespace r2::asset::pln::audio
{
    const std::string SOUND_DEFINITION_NAME_FBS = "SoundDefinition.fbs";
    const std::string JSON_EXT = ".json";
    const std::string SDEF_EXT = ".sdef";
    
    bool GenerateSoundDefinitionsFromJson(const std::string& soundDefinitionFilePath)
    {
        std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
        
        char soundDefinitionSchemaPath[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), soundDefinitionSchemaPath, SOUND_DEFINITION_NAME_FBS.c_str());
        
        std::filesystem::path p = soundDefinitionFilePath;
        
        return r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), soundDefinitionSchemaPath, soundDefinitionFilePath);
    }
    
	bool GenerateSoundDefinitionsFromDirectories(u32 version, const std::string& binFilePath, const std::string& jsonFilePath, const std::vector<std::string>& directories)
    {
        std::vector<r2::asset::AssetReference> bankFiles;
        r2::asset::AssetReference masterBankFile;
        r2::asset::AssetReference masterBankStringsFile;

		char directoryPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SOUNDS, "", directoryPath); //@NOTE(Serge): maybe should be SOUND_DEFINITIONS?

        for (const auto& directory : directories)
        {
            for (auto& file : std::filesystem::recursive_directory_iterator(directory))
            {
                if (std::filesystem::is_directory(file) || std::filesystem::file_size(file.path()) <= 0 || file.path().filename() == ".DS_Store")
                {
                    continue;
                }

                std::filesystem::path filePathURI = file.path().lexically_relative(directoryPath);

                char sanitizedPath[r2::fs::FILE_PATH_LENGTH];

                r2::fs::utils::SanitizeSubPath(filePathURI.string().c_str(), sanitizedPath);

                std::string sanitizedPathStr = sanitizedPath;
                if (sanitizedPathStr.find("Master.bank") != std::string::npos)
                {
                    masterBankFile = r2::asset::CreateNewAssetReference(sanitizedPathStr, sanitizedPathStr, r2::asset::SOUND);
                }
                else if (sanitizedPathStr.find("Master.strings.bank") != std::string::npos)
                {
                    masterBankStringsFile = r2::asset::CreateNewAssetReference(sanitizedPathStr, sanitizedPathStr, r2::asset::SOUND);
                }
                else
                {
                    bankFiles.push_back(r2::asset::CreateNewAssetReference(sanitizedPathStr, sanitizedPathStr, r2::asset::SOUND));
                }
            }
        }

        return GenerateSoundDefinitionsFile(version, binFilePath, jsonFilePath, masterBankFile, masterBankStringsFile, bankFiles);
    }

    bool SaveSoundDefinitionsManifestAssetReferencesToManifestFile(u32 version, const std::vector<r2::asset::AssetReference>& assetReferences, const std::string& binFilePath, const std::string& rawFilePath)
    {
        r2::asset::AssetReference masterBankAssetReference;
        r2::asset::AssetReference masterBankStringsAssetReference;
        std::vector<r2::asset::AssetReference> banks;

        for (const r2::asset::AssetReference& assetReference : assetReferences)
        {
			if (assetReference.binPath ==  "Master.bank")
			{
                masterBankAssetReference = assetReference;
			}
			else if (assetReference.binPath == "Master.strings.bank")
			{
                masterBankStringsAssetReference = assetReference;
			}
			else
			{
                banks.push_back(assetReference);
			}
        }

        return GenerateSoundDefinitionsFile(version, binFilePath, rawFilePath, masterBankAssetReference, masterBankStringsAssetReference, banks);
    }
    
    bool FindSoundDefinitionFile(const std::string& directory, std::string& outPath, bool isBinary)
    {
        for(auto& file : std::filesystem::recursive_directory_iterator(directory))
        {
            //UGH MAC - ignore .DS_Store
            if (std::filesystem::file_size(file.path()) <= 0 || (std::filesystem::path(file.path()).extension().string() != JSON_EXT && std::filesystem::path(file.path()).extension().string() != SDEF_EXT))
            {
                continue;
            }
            
            if ((isBinary && std::filesystem::path(file.path()).extension().string() == SDEF_EXT) || (!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT))
            {
                outPath = file.path().string();
                return true;
            }
        }
        
        return false;
    }
    
	bool GenerateSoundDefinitionsFile(u32 version, const std::string& binFilePath, const std::string& jsonFilePath, const r2::asset::AssetReference& masterBank, const r2::asset::AssetReference& masterBankStrings, const std::vector<r2::asset::AssetReference>& bankFiles)
    {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<flat::AssetRef>> flatBankFiles;

        for (const auto& bankFile : bankFiles)
        {

            //@TODO(Serge): UUID
            auto assetName = flat::CreateAssetName(builder, 0, bankFile.assetName.hashID, builder.CreateString(bankFile.assetName.assetNameString));
            auto assetRef = flat::CreateAssetRef(builder, assetName, builder.CreateString(bankFile.binPath.string()), builder.CreateString(bankFile.rawPath.string()));

            flatBankFiles.push_back(assetRef);
        }

        //@TODO(Serge): UUID
        auto masterBankFileAssetName = flat::CreateAssetName(builder, 0, masterBank.assetName.hashID, builder.CreateString(masterBank.assetName.assetNameString));

        auto masterBankFile = flat::CreateAssetRef(builder, masterBankFileAssetName, builder.CreateString(masterBank.binPath.string()), builder.CreateString(masterBank.rawPath.string()));

        //@TODO(Serge): UUID
        auto masterBankStringsAssetName = flat::CreateAssetName(builder, 0, masterBankStrings.assetName.hashID, builder.CreateString(masterBankStrings.assetName.assetNameString));

        auto masterBankStringsFile = flat::CreateAssetRef(builder, masterBankStringsAssetName, builder.CreateString(masterBankStrings.binPath.string()), builder.CreateString(masterBankStrings.rawPath.string()));

        auto soundDef = flat::CreateSoundDefinitions(builder, version, masterBankFile, masterBankStringsFile, builder.CreateVector(flatBankFiles));

        builder.Finish(soundDef);
        
        byte* buf = builder.GetBufferPointer();
        u32 size = builder.GetSize();
        
        bool result = r2::asset::pln::utils::WriteFile(binFilePath,(char*)buf, size);
        R2_CHECK(result, "Should never happen");
        
        std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
        
        char soundDefinitionSchemaPath[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), soundDefinitionSchemaPath, SOUND_DEFINITION_NAME_FBS.c_str());
        
        std::filesystem::path p = binFilePath;
        
        std::filesystem::path jsonPath = jsonFilePath;
        
        bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), soundDefinitionSchemaPath, binFilePath);
        
        bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), soundDefinitionSchemaPath, jsonPath.string());

        return generatedJSON && generatedBinary;
    }
}
#endif
