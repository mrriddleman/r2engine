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
#include <filesystem>
#include <fstream>

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
    
	bool GenerateSoundDefinitionsFromDirectories(const std::string& binFilePath, const std::string& jsonFilePath, const std::vector<std::string>& directories)
    {
        std::vector<std::string> bankFiles;
        std::string masterBankFile;
        std::string masterBankStringsFile;

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
                    masterBankFile = sanitizedPathStr;
                }
                else if (sanitizedPathStr.find("Master.strings.bank") != std::string::npos)
                {
                    masterBankStringsFile = sanitizedPathStr;
                }
                else
                {
                    bankFiles.push_back(sanitizedPathStr);
                }
            }
        }

        return GenerateSoundDefinitionsFile(binFilePath, jsonFilePath, masterBankFile, masterBankStringsFile, bankFiles);
    }
    
   /* bool AddNewSoundDefinition(std::vector<r2::audio::AudioEngine::SoundDefinition>& soundDefinitions, const r2::audio::AudioEngine::SoundDefinition& def)
    {
        r2::audio::AudioEngine::SoundDefinition matchedDef;
        u64 soundKey = def.soundKey;
        
        auto iter = std::find_if(soundDefinitions.begin(), soundDefinitions.end(), [soundKey, &matchedDef](const r2::audio::AudioEngine::SoundDefinition & otherDef){
            if (otherDef.soundKey == soundKey)
            {
                matchedDef = otherDef;
                return true;
            }
            return false;
        });
        
        if (iter != soundDefinitions.end())
        {
            R2_LOGE("Matched sound key for filepath: %s with existing filepath: %s\n", def.soundName, matchedDef.soundName);
            return false;
        }
        
        soundDefinitions.push_back(def);
        return true;
    }*/
    
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
    
    /*std::vector<r2::audio::AudioEngine::SoundDefinition> LoadSoundDefinitions(const std::string& soundDefinitionFilePath)
    {
        std::vector<r2::audio::AudioEngine::SoundDefinition> soundDefinitions;
        
        std::fstream fs;
        fs.open(soundDefinitionFilePath, std::fstream::in | std::fstream::binary);
        if (fs.good())
        {
            fs.seekg(0, fs.end);
            size_t length = fs.tellg();
            fs.seekg(0, fs.beg);
            
            char* buf = new(std::nothrow) char[length];
            
            R2_CHECK(buf != nullptr, "Failed to allocate buffer for sound definitions");
          
            fs.read(buf, length);
            
            if (!fs.good())
            {
                R2_LOGE("Failed read file %s\n", soundDefinitionFilePath.c_str());
                delete [] buf;
                fs.close();
                return soundDefinitions;
            }
            
            auto soundDefinitionsBuf = r2::GetSoundDefinitions(buf);
            
            flatbuffers::uoffset_t numDefinitions = soundDefinitionsBuf->definitions()->size();
            
            for (flatbuffers::uoffset_t i = 0; i < numDefinitions; ++i)
            {
                r2::audio::AudioEngine::SoundDefinition definition;
                const auto& soundDef = soundDefinitionsBuf->definitions()->Get(i);
                r2::util::PathCpy( definition.soundName, soundDef->soundName()->c_str() );


                definition.defaultVolume = soundDef->defaultVolume();
                definition.minDistance = soundDef->minDistance();
                definition.maxDistance = soundDef->maxDistance();
                definition.loadOnRegister = soundDef->loadOnRegister();
                definition.defaultPitch = soundDef->pitch();
                
                if (soundDef->loop())
                {
                    definition.flags |= r2::audio::AudioEngine::LOOP;
                }
                
                if (soundDef->is3D())
                {
                    definition.flags |= r2::audio::AudioEngine::IS_3D;
                }
                
                if (soundDef->stream())
                {
                    definition.flags |= r2::audio::AudioEngine::STREAM;
                }
                
                char fileName[r2::fs::FILE_PATH_LENGTH];
                r2::fs::utils::CopyFileNameWithExtension(definition.soundName, fileName);
                definition.soundKey = STRING_ID(fileName);
                
                AddNewSoundDefinition(soundDefinitions, definition);
            }
            
            delete [] buf;
        }
        
        fs.close();
        
        return soundDefinitions;
    }*/
    
	bool GenerateSoundDefinitionsFile(const std::string& binFilePath, const std::string& jsonFilePath, const std::string& masterBank, const std::string& masterBankStrings, const std::vector<std::string>& bankFiles)
    {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<flat::BankFile>> flatBankFiles;

        for (const auto& bankFile : bankFiles)
        {
            flatBankFiles.push_back(flat::CreateBankFile(builder, builder.CreateString(bankFile)));
        }

        auto masterBankFile = flat::CreateBankFile(builder, builder.CreateString(masterBank));
        auto masterBankStringsFile = flat::CreateBankFile(builder, builder.CreateString(masterBankStrings));

        auto soundDef = flat::CreateSoundDefinitions(builder, masterBankFile, masterBankStringsFile, builder.CreateVector(flatBankFiles));

        builder.Finish(soundDef);
        
        byte* buf = builder.GetBufferPointer();
        u32 size = builder.GetSize();
        
        std::fstream fs;
        fs.open(binFilePath, std::fstream::out | std::fstream::binary);
        if (fs.good())
        {
            fs.write((const char*)buf, size);
            
            R2_CHECK(fs.good(), "Failed to write out the buffer!");
            if (!fs.good())
            {
                return false;
            }
        }
        
        fs.close();
        
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
