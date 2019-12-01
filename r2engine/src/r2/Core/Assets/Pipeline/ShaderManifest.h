//
//  ShaderManifest.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-30.
//

#ifndef ShaderManifest_h
#define ShaderManifest_h

namespace r2::asset::pln
{
    struct ShaderManifest
    {
        std::string vertexShaderPath = "";
        std::string fragmentShaderPath = "";
        std::string binaryPath = "";
    };
    
    std::vector<ShaderManifest> LoadAllShaderManifests(const std::string& shaderManifestPath);
    bool BuildShaderManifestsFromJson(const std::string& manifestDir);
}

#endif /* ShaderManifest_h */
