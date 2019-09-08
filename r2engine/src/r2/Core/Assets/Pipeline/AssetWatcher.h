//
//  AssetWatcher.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#ifndef AssetWatcher_h
#define AssetWatcher_h

#ifdef R2_DEBUG

#include <vector>
#include <string>
#include <functional>
#include "r2/Core/Assets/Pipeline/FileWatcher.h"

namespace r2::asset::pln
{
    
    using AssetBuiltFunc = std::function<void(std::vector<std::string> paths)>;
    
    
    void Init( const std::string& assetManifestsPath,
               const std::string& assetTempPath,
               const std::string& flatbufferCompilerLocation);
    
    void Update();
    
    void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
    
    
    void PushNewlyBuiltAssets(std::vector<std::string> paths);
    

    void AddAssetBuiltFunction(AssetBuiltFunc func);
}

#endif

#endif /* AssetWatcher_h */
