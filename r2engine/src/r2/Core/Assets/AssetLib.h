//
//  AssetLib.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-16.
//

#ifndef AssetLib_h
#define AssetLib_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetCacheRecord.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#endif

namespace r2::asset
{
    class AssetCache;
    class RawAssetFile;
    class ZipAssetFile;
    class ManifestAssetFile;
	
#ifdef R2_ASSET_PIPELINE

	

    struct ManifestReloadEntry
    {
        HotReloadType hotReloadType;
        std::filesystem::path manifestPath;
        std::vector<std::string> changedPaths;
    };
#endif

    struct ManifestAsset
    {
        r2::asset::AssetFile* mAssetFile;
        const byte* manifestData;
    };

    struct AssetLib
    {
        r2::mem::StackArena* mArena;
        AssetCache* mAssetCache;
        
        r2::mem::utils::MemBoundary mBoundary;
        r2::mem::utils::MemBoundary mAssetCacheBoundary;

        r2::SHashMap<AssetCacheRecord>* mAssetCacheRecords;

        r2::SArray<ManifestAssetFile*>* mManifestFiles;

        FileList mGameFileList;

#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::AssetThreadSafeQueue<ManifestReloadEntry> mManifestChangedRequests;
#endif

        static u64 MemorySize(u32 cacheSize, u32 numGameManifests, u32 numEngineManifests);

    };

}

//@TODO(Serge): this is bad for multithreading. We should consider a different way of doing this if we're going
//                 to multithread things

namespace r2::asset::lib
{
    bool Init(const r2::mem::utils::MemBoundary& boundary);
    void Update();
    void Shutdown();

    

    AssetLib* Create(const r2::mem::utils::MemBoundary& boundary, u32 numManifests, u32 cacheSize);
    void Shutdown(AssetLib* assetLib);
    void Update(AssetLib& assetLib);
    
    const byte* GetManifestData(AssetLib& assetLib, u64 manifestAssetHandle);
    void GetManifestDataForType(AssetLib& assetLib, r2::asset::EngineAssetType type, r2::SArray<const byte*>* manifestDataArray);

    void RegisterManifestFile(AssetLib& assetLib, ManifestAssetFile* manifestFile);
    bool RegenerateAssetFilesFromManifests(const AssetLib& assetLib);
    FileList GetFileList(const AssetLib& assetLib);

#ifdef R2_ASSET_PIPELINE
    void PathChangedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& changedPaths);
    void PathAddedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector < std::string>& pathsAdded);
    void PathRemovedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector < std::string>& pathsRemoved);
#endif

#ifdef R2_ASSET_PIPELINE
    void PushFilesBuilt(std::vector<std::string> paths);
    using AssetFilesBuiltListener = std::function<void(std::vector<std::string> paths)>;
    void AddAssetFilesBuiltListener(AssetFilesBuiltListener func);

    void ResetFilesForCache(const r2::asset::AssetCache& cache, FileList list);
#endif
    
    RawAssetFile* MakeRawAssetFile(const char* path, u32 numParentDirectoriesToInclude = 0);
    ZipAssetFile* MakeZipAssetFile(const char* path);  
    ManifestAssetFile* MakeManifestSingleAssetFile(const char* path, r2::asset::AssetType assetType);
    ManifestAssetFile* MakeTexturePackManifestAssetFile(const char* path);

    FileList MakeFileList(u64 capacity);
    
    //@TODO(Serge): implement helpers to create file lists easier
    //For example with a manifest or a directory 

    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, u32 assetTotalSize, r2::asset::FileList files);
    void DestroyCache(r2::asset::AssetCache* cache);
    r2::asset::AssetCache* GetAssetCache(s64 slot);

}

#endif /* AssetLib_h */
