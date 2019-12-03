//
//  ShaderManifest.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-30.
//

#include "ShaderManifest.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest_generated.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>
#include <fstream>

namespace r2::asset::pln
{
    const std::string SHADER_MANIFEST_NAME_FBS = "ShaderManifest.fbs";
    const std::string MANIFEST_EXT = ".sman";
    const std::string JSON_EXT = ".json";
    const std::string VERTEX_EXT = ".vs";
    const std::string FRAGMENT_EXT = ".fs";
    
    bool BuildShaderManifestsIfNeeded(std::vector<ShaderManifest>& currentManifests, const std::string& manifestFilePath, const std::string& rawPath)
    {
        size_t oldSize = currentManifests.size();
        
        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
            if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != VERTEX_EXT && file.path().extension().string() != FRAGMENT_EXT))
            {
                continue;
            }
            
            //find the manifest for this
            bool found = false;
            for(u32 i = 0; i < currentManifests.size() && !found; ++i)
            {
                if (currentManifests[i].vertexShaderPath == file.path().string() ||
                    currentManifests[i].fragmentShaderPath == file.path().string())
                {
                    found = true;
                }
            }
            
            if (!found)
            {
                //generate one if we find the matching shader
                std::string otherExt = file.path().extension().string() == VERTEX_EXT ? FRAGMENT_EXT : VERTEX_EXT;
                
                for(const auto& otherShaderFile : std::filesystem::recursive_directory_iterator(rawPath))
                {
                    if (std::filesystem::file_size(file.path()) <= 0 || (otherShaderFile.path().extension().string() != otherExt))
                    {
                        continue;
                    }
                    if(file.path().stem() == otherShaderFile.path().stem())
                    {
                        //add an entry
                        ShaderManifest newManifest;
                        if(otherShaderFile.path().extension() == VERTEX_EXT)
                        {
                            newManifest.vertexShaderPath = otherShaderFile.path().string();
                            newManifest.fragmentShaderPath = file.path().string();
                        }
                        else
                        {
                            newManifest.vertexShaderPath = file.path().string();
                            newManifest.fragmentShaderPath = otherShaderFile.path().string();
                        }
                        
                        currentManifests.push_back(newManifest);
                        
                        break;
                    }
                }
            }
        }
        
        if (oldSize != currentManifests.size())
        {
            return GenerateShaderManifests(currentManifests, manifestFilePath);
        }
        
        return true;
    }
    
    bool GenerateShaderManifests(const std::vector<ShaderManifest>& manifests, const std::string& manifestFilePath)
    {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<r2::ShaderManifest>> flatShaderManifests;
        
        for (const auto& manifest : manifests)
        {
            flatShaderManifests.push_back(r2::CreateShaderManifest
                                    (builder, builder.CreateString(manifest.vertexShaderPath),
                                        builder.CreateString(manifest.fragmentShaderPath),
                                        builder.CreateString(manifest.binaryPath)));
        }
        
        auto shaders = r2::CreateShaderManifests(builder, builder.CreateVector(flatShaderManifests));
        
        builder.Finish(shaders);
        
        byte* buf = builder.GetBufferPointer();
        u32 size = builder.GetSize();
        
        std::filesystem::remove(manifestFilePath);
        
        std::fstream fs;
        fs.open(manifestFilePath, std::fstream::out | std::fstream::binary);
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
        
        char shaderManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), shaderManifestSchemaPath, SHADER_MANIFEST_NAME_FBS.c_str());
        
        std::filesystem::path p = manifestFilePath;
        ;
        std::filesystem::path jsonPath = p.parent_path() / std::filesystem::path(p.stem().string() + JSON_EXT);
        std::filesystem::remove(jsonPath);
        bool generatedJSON = r2::asset::pln::flat::GenerateFlatbufferJSONFile(p.parent_path().string(), shaderManifestSchemaPath, manifestFilePath);
        
        bool generatedBinary = r2::asset::pln::flat::GenerateFlatbufferBinaryFile(p.parent_path().string(), shaderManifestSchemaPath, jsonPath);
        
        return generatedJSON && generatedBinary;
    }
    
    std::vector<ShaderManifest> LoadAllShaderManifests(const std::string& shaderManifestPath)
    {
        std::vector<ShaderManifest> shaderManifests;
        
        std::fstream fs;
        fs.open(shaderManifestPath, std::fstream::in | std::fstream::binary);
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
                R2_LOGE("Failed read file %s\n", shaderManifestPath.c_str());
                delete [] buf;
                fs.close();
                return {};
            }
            
            auto shaderManifestsBuf = r2::GetShaderManifests(buf);
            
            size_t numManifests = shaderManifestsBuf->manifests()->Length();
            
            for (size_t i = 0; i < numManifests; ++i)
            {
                ShaderManifest newManifest;
                newManifest.vertexShaderPath = shaderManifestsBuf->manifests()->Get(i)->vertexPath()->str();
                newManifest.fragmentShaderPath = shaderManifestsBuf->manifests()->Get(i)->fragmentPath()->str();
                newManifest.binaryPath = shaderManifestsBuf->manifests()->Get(i)->binaryPath()->str();
                
                shaderManifests.push_back(newManifest);
            }
            
            delete [] buf;
        }
        
        fs.close();
        
        return shaderManifests;
    }
    
    bool BuildShaderManifestsFromJson(const std::string& manifestDir)
    {
        const std::string dataPath = R2_ENGINE_DATA_PATH;
        const std::string flatBufferPath = dataPath + "/flatbuffer_schemas/";
        const std::string assetManifestFbsPath = flatBufferPath + "ShaderManifest.fbs";
        
        for(const auto& file : std::filesystem::recursive_directory_iterator(manifestDir))
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

