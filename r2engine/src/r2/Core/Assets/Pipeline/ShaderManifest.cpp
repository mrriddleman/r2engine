//
//  ShaderManifest.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-30.
//
#include "r2pch.h"
#include "ShaderManifest.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest_generated.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
#include <fstream>

#ifdef R2_ASSET_PIPELINE

namespace r2::asset::pln
{
    const std::string SHADER_MANIFEST_NAME_FBS = "ShaderManifest.fbs";
    const std::string MANIFEST_EXT = ".sman";
    const std::string JSON_EXT = ".json";
    const std::string VERTEX_EXT = ".vs";
    const std::string FRAGMENT_EXT = ".fs";
    const std::string GEOMETRY_EXT = ".gs";
    const std::string COMPUTE_EXT = ".cs";
    const std::string PART_EXT = ".glsl";
    
    
    bool BuildShaderManifestsIfNeeded(std::vector<ShaderManifest>& currentManifests, const std::string& manifestFilePath, const std::string& rawPath)
    {
        size_t oldSize = currentManifests.size();
        //@TODO(Serge): add in support for changes to names of files as well/adding geometry shaders names directly to the manifests
        
        std::vector<ShaderManifest> newManifests;
        
        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
            if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != VERTEX_EXT))
            {
                continue;
            }
            
            char cStringPath[fs::FILE_PATH_LENGTH];

            fs::utils::SanitizeSubPath(file.path().string().c_str(), cStringPath);

            ShaderManifest newManifest;
            newManifest.basePath = rawPath;
            newManifest.hashName = STRING_ID( file.path().stem().string().c_str() );
            newManifest.vertexShaderPath = std::string(cStringPath);
            newManifests.push_back(newManifest);
        }

        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
            if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != COMPUTE_EXT))
            {
                continue;
            }

			char cStringPath[fs::FILE_PATH_LENGTH];

			fs::utils::SanitizeSubPath(file.path().string().c_str(), cStringPath);

            ShaderManifest newManifest;
            newManifest.basePath = rawPath;

            newManifest.hashName = STRING_ID(file.path().stem().string().c_str());
            newManifest.computeShaderPath = std::string(cStringPath);
            newManifests.push_back(newManifest);
        }

        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
			if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != PART_EXT))
			{
				continue;
			}

			char cStringPath[fs::FILE_PATH_LENGTH];

			fs::utils::SanitizeSubPath(file.path().string().c_str(), cStringPath);



			auto r = file.path().lexically_relative(rawPath);

            char sanitizedCStringName[fs::FILE_PATH_LENGTH];

            fs::utils::SanitizeSubPath(r.string().c_str(), sanitizedCStringName);

            //printf("part name: %s\n", sanitizedCStringName);
		//	auto pos = r.string().find('.');

		//	std::string resultString = r.string().substr(0, pos);

		//	printf("%s\n", resultString.c_str());


			ShaderManifest newManifest;
            newManifest.basePath = rawPath;
			std::string stringName = sanitizedCStringName;
			newManifest.hashName = STRING_ID(stringName.c_str());
			newManifest.partPath = std::string(cStringPath);
			newManifests.push_back(newManifest);
        }
        
        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
            if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != FRAGMENT_EXT))
            {
                continue;
            }
            
            //find the matching vertex shader
            for (auto& shaderManifest : newManifests)
            {
                std::filesystem::path vertexPath(shaderManifest.vertexShaderPath);
                if (vertexPath.stem() == file.path().stem())
                {
					char cStringPath[fs::FILE_PATH_LENGTH];

					fs::utils::SanitizeSubPath(file.path().string().c_str(), cStringPath);

                    shaderManifest.fragmentShaderPath = std::string(cStringPath);
                    break;
                }
            }
        }
        
        for (const auto& file : std::filesystem::recursive_directory_iterator(rawPath))
        {
            if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != GEOMETRY_EXT))
            {
                continue;
            }
            
            //find the matching vertex shader
            for (auto& shaderManifest : newManifests)
            {
                std::filesystem::path vertexPath(shaderManifest.vertexShaderPath);
                if (vertexPath.stem() == file.path().stem())
                {
					char cStringPath[fs::FILE_PATH_LENGTH];

					fs::utils::SanitizeSubPath(file.path().string().c_str(), cStringPath);

                    shaderManifest.geometryShaderPath = std::string(cStringPath);
                    break;
                }
            }
        }
        
        //remove all degenerate cases
        auto iter = std::remove_if(newManifests.begin(), newManifests.end(), [](const ShaderManifest& manifest){
            return
                manifest.computeShaderPath == "" && manifest.partPath == "" && (manifest.vertexShaderPath == "" || manifest.fragmentShaderPath == "");
        });
        newManifests.erase(iter, newManifests.end());
        
        if (oldSize != newManifests.size())
        {
            currentManifests = newManifests;
            return GenerateShaderManifests(currentManifests, manifestFilePath, rawPath);
        }
        
        return true;
    }
    
    bool GenerateShaderManifests(const std::vector<ShaderManifest>& manifests, const std::string& manifestFilePath, const std::string& rawPath)
    {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<r2::ShaderManifest>> flatShaderManifests;
        
        for (const auto& manifest : manifests)
        {
            flatShaderManifests.push_back(r2::CreateShaderManifest
                                    (builder,manifest.hashName, builder.CreateString(manifest.vertexShaderPath),
                                        builder.CreateString(manifest.fragmentShaderPath),
                                        builder.CreateString(manifest.geometryShaderPath),
                                        builder.CreateString(manifest.computeShaderPath),
                                        builder.CreateString(manifest.binaryPath),
                                        builder.CreateString(manifest.partPath)));
        }
        
        auto shaders = r2::CreateShaderManifests(builder, builder.CreateVector(flatShaderManifests), builder.CreateString(rawPath));
        
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
        bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(p.parent_path().string(), shaderManifestSchemaPath, manifestFilePath);
        
        bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), shaderManifestSchemaPath, jsonPath.string());
        
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
            
            flatbuffers::uoffset_t numManifests = shaderManifestsBuf->manifests()->size();
            
            for (flatbuffers::uoffset_t i = 0; i < numManifests; ++i)
            {
                ShaderManifest newManifest;
                newManifest.hashName = shaderManifestsBuf->manifests()->Get(i)->shaderName();
                newManifest.vertexShaderPath = shaderManifestsBuf->manifests()->Get(i)->vertexPath()->str();
                newManifest.fragmentShaderPath = shaderManifestsBuf->manifests()->Get(i)->fragmentPath()->str();
                newManifest.geometryShaderPath = shaderManifestsBuf->manifests()->Get(i)->geometryPath()->str();
                newManifest.computeShaderPath = shaderManifestsBuf->manifests()->Get(i)->computePath()->str();
                newManifest.binaryPath = shaderManifestsBuf->manifests()->Get(i)->binaryPath()->str();
                newManifest.partPath = shaderManifestsBuf->manifests()->Get(i)->partPath()->str();
                newManifest.basePath = shaderManifestsBuf->basePath()->str();
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
            if(!flathelp::GenerateFlatbufferBinaryFile(manifestDir, assetManifestFbsPath, file.path().string()))
            {
                R2_LOGE("Failed to generate asset manifest for json file: %s\n", file.path().string().c_str());
                return false;
            }
        }
        
        return true;
    }

    bool BuildShaderManifestsFromJsonIO(const std::string& inputJsonPath, const std::string& outputPath)
    {
		const std::string dataPath = R2_ENGINE_DATA_PATH;
		const std::string flatBufferPath = dataPath + "/flatbuffer_schemas/";
		const std::string assetManifestFbsPath = flatBufferPath + "ShaderManifest.fbs";

        

     //   if(std::filesystem::exists())

		if (std::filesystem::file_size(inputJsonPath) <= 0 || std::filesystem::path(inputJsonPath).extension().string() != JSON_EXT)
		{
            R2_CHECK(false, "%s isn't a proper json file!", inputJsonPath.c_str());
			return false;
		}

        if (!flathelp::GenerateFlatbufferBinaryFile(outputPath, assetManifestFbsPath, inputJsonPath))
        {
			R2_LOGE("Failed to generate asset manifest for json file: %s\n", inputJsonPath.c_str());
			return false;
        }

        return true;
    }
}

#endif