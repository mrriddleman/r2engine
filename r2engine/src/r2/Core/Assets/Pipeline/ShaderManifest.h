//
//  ShaderManifest.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-30.
//

#ifndef ShaderManifest_h
#define ShaderManifest_h

#ifdef R2_ASSET_PIPELINE
namespace r2::asset::pln
{
    struct ShaderManifest
    {
        u64 hashName = 0;
        std::string vertexShaderPath = "";
        std::string fragmentShaderPath = "";
        std::string geometryShaderPath = "";
        std::string computeShaderPath = "";
        std::string binaryPath = "";
        std::string partPath = "";
        std::string basePath = "";
    };
    
    bool BuildShaderManifestsIfNeeded(std::vector<ShaderManifest>& currentManifests, const std::string& manifestPath, const std::string& rawPath);
    std::vector<ShaderManifest> LoadAllShaderManifests(const std::string& shaderManifestPath);
    bool BuildShaderManifestsFromJson(const std::string& manifestDir);
    bool GenerateShaderManifests(const std::vector<ShaderManifest>& manifests, const std::string& manifestsPath, const std::string& rawPath);
    bool BuildShaderManifestsFromJsonIO(const std::string& inputJsonPath, const std::string& outputPath);
}
#endif
#endif /* ShaderManifest_h */
