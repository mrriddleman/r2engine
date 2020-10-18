//
//  AssetWatcher.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#ifndef AssetWatcher_h
#define AssetWatcher_h

#ifdef R2_ASSET_PIPELINE

#include <vector>
#include <string>
#include <functional>
#include "r2/Core/Assets/Pipeline/FileWatcher.h"

namespace r2::asset::pln
{
    
    using AssetsBuiltFunc = std::function<void(std::vector<std::string> paths)>;
    
    //@TODO(Serge): we need a way here to specifically build sound definitions based on a vector of sound directories - might be separate from these
    
    struct SoundDefinitionCommand
    {
        std::vector<std::string> soundDirectories;
        std::string soundDefinitionFilePath;
        AssetsBuiltFunc buildFunc;
    };
    
    struct ShaderManifestCommand
    {
        std::vector<std::string> manifestDirectories;
        std::vector<std::string> shaderWatchPaths;
        std::vector<std::string> manifestFilePaths;
        AssetsBuiltFunc buildFunc;
    };
    
    struct TexturePackManifestCommand
    {
        std::vector<std::string> manifestRawFilePaths;
        std::vector<std::string> manifestBinaryFilePaths;
        std::vector<std::string> texturePacksWatchDirectories;

        AssetsBuiltFunc buildFunc;
    };

    struct MaterialPackManifestCommand
    {
		std::vector<std::string> manifestRawFilePaths;
        std::vector<std::string> manifestBinaryFilePaths;
		std::vector<std::string> materialPacksWatchDirectoriesRaw;
        std::vector<std::string> materialPacksWatchDirectoriesBin;

		AssetsBuiltFunc buildFunc;
    };

    struct AssetCommand
    {
        std::string assetManifestsPath;
        std::string assetTempPath;
        std::vector<std::string> pathsToWatch;
        AssetsBuiltFunc assetsBuldFunc;
    };
    
    void Init(  const std::string& flatbufferCompilerLocation,
                Milliseconds delay,
                const AssetCommand& assetCommand,
                const SoundDefinitionCommand& soundDefinitionCommand,
                const ShaderManifestCommand& shaderCommand,
                const TexturePackManifestCommand& texturePackCommand,
                const MaterialPackManifestCommand& materialPackCommand);
    
    void Update();
    
    void Shutdown();
    //@NOTE: if we want this functionality again, we could add another queue that would push more watch paths
    //void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
}

#endif

#endif /* AssetWatcher_h */
