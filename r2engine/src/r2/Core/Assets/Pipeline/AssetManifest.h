//
//  AssetManifest.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//

#ifndef AssetManifest_h
#define AssetManifest_h

#include <vector>
#include <string>

namespace r2::asset::pln
{
    enum class AssetCompileCommand
    {
        None,
        FlatBufferCompile
    };
    
    enum class AssetType
    {
        Raw,
        Zip
    };
    
    struct AssetBuildCommand
    {
        AssetCompileCommand compile = AssetCompileCommand::None;
        std::string schemaPath = "";
    };
    
    struct AssetFileCommand
    {
        std::string inputFile = "";
        std::string outputFile = "";
        AssetBuildCommand command;
    };
    
    struct AssetManifest
    {
        std::string assetOutputPath = "";
        std::vector<AssetFileCommand> fileCommands;
        AssetType outputType = AssetType::Raw; //output type
    };
    
    bool WriteAssetManifest(const AssetManifest& manifest, const std::string& manifestName, const std::string& manifestDir);
    bool LoadAssetManifests(std::vector<AssetManifest>& manifests, const std::string& manifestDir);
    bool BuildAssetManifestsFromJson(const std::string& manifestDir);
}

#endif /* AssetManifest_h */
