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
#include "r2/Core/Assets/AssetFiles/AssetFile.h"

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/AssetReference.h"
#endif


namespace r2::asset
{
    class AssetCache;
    class RawAssetFile;
    class SoundAssetFile;
    class ZipAssetFile;
    class ManifestAssetFile;
	
#ifdef R2_ASSET_PIPELINE

    struct AssetReference;

    struct ManifestReloadEntry
    {
        pln::HotReloadType hotReloadType;
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

        r2::SArray<ManifestAssetFile*>* mManifestFiles;

     //   FileList mGameFileList;

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
    const u32 MAX_NUM_GAME_ASSET_FILES = 2000;

    bool Init(const r2::mem::utils::MemBoundary& boundary);
    void Update();
    void Shutdown();

    AssetLib* Create(const r2::mem::utils::MemBoundary& boundary, u32 numManifests, u32 cacheSize);
    void Shutdown(AssetLib* assetLib);
    void Update(AssetLib& assetLib);
    

    const byte* GetManifestData(AssetLib& assetLib, u64 manifestAssetHandle);
    void GetManifestDataForType(AssetLib& assetLib, r2::asset::EngineAssetType type, r2::SArray<const byte*>* manifestDataArray);
    u32 GetManifestDataCountForType(AssetLib& assetLib, r2::asset::EngineAssetType type);

    void RegisterAndLoadManifestFile(AssetLib& assetLib, ManifestAssetFile* manifestFile);

    ManifestAssetFile* GetManifest(AssetLib& assetLib, u64 manifestAssetHandle);
    void GetManifestFilesForType(AssetLib& assetLib, r2::asset::EngineAssetType type, r2::SArray<ManifestAssetFile*>* manifests);
    u32 GetManifestCountForType(AssetLib& assetLib, r2::asset::EngineAssetType type);


    bool HasAsset(AssetLib& assetLib, const char* path, r2::asset::AssetType type);
    bool HasAsset(AssetLib& assetLib, const Asset& asset);

    r2::asset::AssetFile* GetAssetFileForAsset(AssetLib& assetLib, const Asset& asset);

#ifdef R2_ASSET_PIPELINE
    bool ImportAsset(AssetLib& assetLib, const AssetReference& assetReference, r2::asset::AssetType type);
    void PathChangedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& changedPaths);
    void PathAddedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector < std::string>& pathsAdded);
    void PathRemovedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector < std::string>& pathsRemoved);

    //void PushFilesBuilt(std::vector<std::string> paths);
    //using AssetFilesBuiltListener = std::function<void(std::vector<std::string> paths)>;
    //void AddAssetFilesBuiltListener(AssetFilesBuiltListener func);

    //void ResetFilesForCache(const r2::asset::AssetCache& cache, FileList list);

    std::vector<r2::asset::AssetFile*> GetAllAssetFilesForType(AssetLib& assetLib, r2::asset::AssetType type);

    std::vector<r2::asset::AssetName> GetAllAssetNamesForType(AssetLib& assetLib, r2::asset::AssetType type);

	//@Temporary
	void ImportAssetFiles(AssetLib& assetLib, const std::vector<r2::asset::AssetReferenceAndType>& assetReferences);
#endif
    

    RawAssetFile* MakeRawAssetFile(const char* path, r2::asset::AssetType assetType);
    void FreeRawAssetFile(RawAssetFile* file);
    //@TODO(Serge): maybe make a MakeRawAssetFile that will alloc an array?

    SoundAssetFile* MakeSoundAssetFile(const char* path);
    void FreeSoundAssetFile(SoundAssetFile* file);


    ZipAssetFile* MakeZipAssetFile(const char* path);  

    ManifestAssetFile* MakeTexturePackManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);
    ManifestAssetFile* MakeMaterialManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);
    ManifestAssetFile* MakeRModelsManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);
    ManifestAssetFile* MakeSoundsManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);
    ManifestAssetFile* MakeLevelPackManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);
    ManifestAssetFile* MakeEngineModelManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath);

    FileList MakeFileList(u64 capacity);
    void DestoryFileList(const FileList fileList);

    //@TODO(Serge): implement helpers to create file lists easier
    //For example with a manifest or a directory 

    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, u32 assetTotalSize, u32 numFiles);
    void DestroyCache(r2::asset::AssetCache* cache);
   // r2::asset::AssetCache* GetAssetCache(s64 slot);

}

#endif /* AssetLib_h */
