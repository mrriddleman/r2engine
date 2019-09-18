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
    
    
    void Init( const std::string& assetManifestsPath,
               const std::string& assetTempPath,
               const std::string& flatbufferCompilerLocation,
               Milliseconds delay,
               const std::vector<std::string>& paths,
               AssetsBuiltFunc assetsBuiltFunc);
    
    
    
    void Shutdown();
    //@NOTE: if we want this functionality again, we could add another queue that would push more watch paths
    //void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
}

#endif

#endif /* AssetWatcher_h */
