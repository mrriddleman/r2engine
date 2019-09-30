//
//  AssetManifest.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetManifest_generated.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln
{
    const std::string MANIFEST_EXT = ".aman";
    const std::string JSON_EXT = ".json";
    
    bool WriteAssetManifest(const AssetManifest& manifest, const std::string& manifestName, const std::string& manifestDir)
    {
        flatbuffers::FlatBufferBuilder builder;
        
        std::vector<flatbuffers::Offset<r2::AssetFileCommand>> fileCommands;
        
        for (const auto& assetFileCommand: manifest.fileCommands)
        {
            fileCommands.push_back(r2::CreateAssetFileCommand(builder, builder.CreateString(assetFileCommand.inputPath), builder.CreateString(assetFileCommand.outputFileName), r2::CreateAssetBuildCommand(builder, static_cast<r2::AssetCompileCommand>(assetFileCommand.command.compile), builder.CreateString(assetFileCommand.command.schemaPath))));
        }
        
        auto inputFiles = builder.CreateVector(fileCommands);
        
        auto assetManifest = r2::CreateAssetManifest(builder, builder.CreateString(manifest.assetOutputPath), inputFiles, static_cast<r2::AssetType>(manifest.outputType));
        
        builder.Finish(assetManifest);
        
        char* buf = (char*)builder.GetBufferPointer();
        u32 size = builder.GetSize();
        
        std::fstream fs;
        
        std::string manifestPath = "";
        
        if (manifestDir[manifestDir.size()-1] != r2::fs::utils::PATH_SEPARATOR)
        {
            manifestPath = manifestDir + r2::fs::utils::PATH_SEPARATOR;
            manifestPath += manifestName;
        }
        else
        {
            manifestPath = manifestDir + manifestName;
        }
        
        fs.open(manifestPath.c_str(), std::fstream::out | std::fstream::binary);
        fs.write(buf, size);
        fs.close();
        
        if(fs.fail())
            return false;
        
        std::string dataPath = R2_ENGINE_DATA_PATH;
        std::string flatBufferPath = dataPath + "/flatbuffer_schemas/";
        std::string assetManifestFbsPath = flatBufferPath + "AssetManifest.fbs";
        
        //generate json files
        if(!flat::GenerateFlatbufferJSONFile(manifestDir, assetManifestFbsPath, manifestPath))
        {
            return false;
        }
        
        return true;
    }
    
    bool LoadAssetManifests(std::vector<AssetManifest>& manifests, const std::string& manifestDir)
    {
        for(auto& file : std::filesystem::recursive_directory_iterator(manifestDir))
        {
            //UGH MAC - ignore .DS_Store
            if (std::filesystem::file_size(file.path()) <= 0 || std::filesystem::path(file.path()).extension().string() != MANIFEST_EXT)
            {
                continue;
            }
            
            std::fstream fs;
            fs.open(file.path().string().c_str(), std::fstream::in | std::fstream::binary);
            
            fs.seekg(0, fs.end);
            size_t length = fs.tellg();
            fs.seekg(0, fs.beg);
            
            char* buf = new(std::nothrow) char[length];
            if (!buf)
            {
                fs.close();
                R2_LOGE("Failed to allocate %zu bytes for %s\n", length, file.path().string().c_str());
                return false;
            }
            
            fs.read(buf, length);
            
            if (!fs.good())
            {
                fs.close();
                delete [] buf;
                R2_LOGE("Failed read file %s\n", file.path().string().c_str());
                return false;
            }
            
            fs.close();
            
            const auto assetManifest = r2::GetAssetManifest(buf);
            
            AssetManifest manifest;
            
            
            manifest.assetOutputPath = assetManifest->outputPath()->str();
            manifest.outputType = static_cast<AssetType>( assetManifest->outputType() );
            for (u32 i = 0; i < assetManifest->inputFiles()->Length(); ++i)
            {
                AssetFileCommand fileCommand;
                fileCommand.inputPath = assetManifest->inputFiles()->Get(i)->inputFile()->str();
                fileCommand.outputFileName = assetManifest->inputFiles()->Get(i)->outputFile()->str();
                fileCommand.command.compile = static_cast<AssetCompileCommand>( assetManifest->inputFiles()->Get(i)->buildCommand()->compileCommand() );
                fileCommand.command.schemaPath = assetManifest->inputFiles()->Get(i)->buildCommand()->schemaPath()->str();
                
                manifest.fileCommands.push_back(fileCommand);
            }
            
            manifests.push_back(manifest);
            
            delete [] buf;
        }
        
        return true;
    }
    
    bool BuildAssetManifestsFromJson(const std::string& manifestDir)
    {
        const std::string dataPath = R2_ENGINE_DATA_PATH;
        const std::string flatBufferPath = dataPath + "/flatbuffer_schemas/";
        const std::string assetManifestFbsPath = flatBufferPath + "AssetManifest.fbs";
        
        for(auto& file : std::filesystem::recursive_directory_iterator(manifestDir))
        {
            
            if (std::filesystem::file_size(file.path()) <= 0 || std::filesystem::path(file.path()).extension().string() != JSON_EXT)
            {
                continue;
            }
            
            //generate json files
            if(!flat::GenerateFlatbufferBinaryFile(manifestDir, assetManifestFbsPath, file.path().string()))
            {
                R2_LOGE("Failed to generate asset manifest for json file: %s\n", file.path().string().c_str());
                return false;
            }
        }
        
        return true;
    }
}
#endif
