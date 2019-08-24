//
//  AssetWatcher.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#ifndef AssetWatcher_h
#define AssetWatcher_h

#include <vector>
#include <string>
#include <functional>
#include "r2/Core/Assets/Pipeline/FileWatcher.h"

namespace r2::asset::pln
{
    using AssetChangedFunc = std::function<void (const std::vector<std::string>& assets)>;
    
    void Init( const std::string& assetManifestsPath,
               const std::string& flatbufferCompilerLocation);
    
    void Update();
    void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
    
    void AddAssetChangedListener(AssetChangedFunc func);
    
}

#endif /* AssetWatcher_h */
