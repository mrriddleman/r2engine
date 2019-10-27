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
    class RawAssetFile;
    class ZipAssetFile;
}

namespace r2::asset::lib
{
    bool Init(const r2::mem::utils::MemBoundary& boundary);
    void Update();
    void Shutdown();
    void AddFiles(const r2::asset::AssetCache& cache, FileList list);
    
#ifdef R2_ASSET_PIPELINE
    void PushFilesBuilt(std::vector<std::string> paths);
    using AssetFilesBuiltListener = std::function<void(std::vector<std::string> paths)>;
    void AddAssetFilesBuiltListener(AssetFilesBuiltListener func);
#endif
    
    RawAssetFile* MakeRawAssetFile(const char* path);
    ZipAssetFile* MakeZipAssetFile(const char* path);  
    FileList MakeFileList(u64 capacity);
    
    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, r2::asset::FileList files);
    void DestroyCache(r2::asset::AssetCache* cache);
    
    u64 EstimateMaxMemUsage(u32 maxZipArchiveCentralDirSize, u32 maxFilesInList);
}

#endif /* AssetLib_h */
