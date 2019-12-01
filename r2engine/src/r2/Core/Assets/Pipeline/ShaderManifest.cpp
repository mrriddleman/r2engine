//
//  ShaderManifest.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-30.
//

#include "ShaderManifest.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest_generated.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln
{
    const std::string MANIFEST_EXT = ".sman";
    const std::string JSON_EXT = ".json";
    
    std::vector<ShaderManifest> LoadAllShaderManifests(const std::string& shaderManifestPath)
    {
        std::vector<ShaderManifest> manifests = {};
        
        for(auto& file : std::filesystem::recursive_directory_iterator(shaderManifestPath))
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
                return {};
            }
            
            fs.read(buf, length);
            
            if (!fs.good())
            {
                fs.close();
                delete [] buf;
                R2_LOGE("Failed read file %s\n", file.path().string().c_str());
                return {};
            }
            
            fs.close();
            
            const auto assetManifest = r2::GetShaderManifest(buf);
            
            ShaderManifest manifest;
            
            manifest.vertexShaderPath = assetManifest->vertexPath()->str();
            manifest.fragmentShaderPath = assetManifest->fragmentPath()->str();
            manifest.binaryPath = assetManifest->binaryPath()->str();
            
            manifests.push_back(manifest);
            
            delete [] buf;
        }
        
        return manifests;
    }
    
    bool BuildShaderManifestsFromJson(const std::string& manifestDir)
    {
        const std::string dataPath = R2_ENGINE_DATA_PATH;
        const std::string flatBufferPath = dataPath + "/flatbuffer_schemas/";
        const std::string assetManifestFbsPath = flatBufferPath + "ShaderManifest.fbs";
        
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

