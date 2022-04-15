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
    class GLTFAssetFile;
}

//@TODO(Serge): this is bad for multithreading. We should consider a different way of doing this if we're going
//                 to multithread things

namespace r2::asset::lib
{
    bool Init(const r2::mem::utils::MemBoundary& boundary);
    void Update();
    void Shutdown();
  //  void AddFiles(const r2::asset::AssetCache& cache, FileList list);
    
#ifdef R2_ASSET_PIPELINE
    void PushFilesBuilt(std::vector<std::string> paths);
    using AssetFilesBuiltListener = std::function<void(std::vector<std::string> paths)>;
    void AddAssetFilesBuiltListener(AssetFilesBuiltListener func);

    void ResetFilesForCache(const r2::asset::AssetCache& cache, FileList list);
#endif
    
    RawAssetFile* MakeRawAssetFile(const char* path, u32 numParentDirectoriesToInclude = 0);
    ZipAssetFile* MakeZipAssetFile(const char* path);  
    GLTFAssetFile* MakeGLTFAssetFile(const char* path, u32 numParentDirectoriesToInclude = 0);

    FileList MakeFileList(u64 capacity);
    
    //@TODO(Serge): implement helpers to create file lists easier
    //For example with a manifest or a directory 

    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, r2::asset::FileList files);
    void DestroyCache(r2::asset::AssetCache* cache);
    r2::asset::AssetCache* GetAssetCache(s64 slot);


    u64 EstimateMaxMemUsage(u32 maxZipArchiveCentralDirSize, u64 maxFilesInList);
}

#endif /* AssetLib_h */
