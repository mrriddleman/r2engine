//
//  AssetLib.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-16.
//

#ifndef AssetLib_h
#define AssetLib_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::asset
{
    class AssetCache;
    class AssetFile;
    using FileList = r2::SArray<AssetFile*>*;
}

namespace r2::asset::lib
{
    const u32 MAX_ASSET_CACHES = 1000;
    
    bool Init(u32 numAssetCaches = MAX_ASSET_CACHES);
    void Update();
    void Shutdown();
    void AddFiles(const r2::asset::AssetCache& cache, FileList list);
    
#ifdef R2_ASSET_PIPELINE
    void PushFilesBuilt(std::vector<std::string> paths);
    using AssetFilesBuiltListener = std::function<void(std::vector<std::string> paths)>;
    void AddAssetFilesBuiltListener(AssetFilesBuiltListener func);
#endif
    
    //returns uninitialized non owning ptr to an AssetCache - @TODO(Serge): clean this up so it will actually be initialized
    r2::asset::AssetCache* CreateAssetCache(r2::mem::utils::MemBoundary boundary);
}

#endif /* AssetLib_h */
