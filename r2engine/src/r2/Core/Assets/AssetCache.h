//
//  AssetCache.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//

#ifndef AssetCache_h
#define AssetCache_h

#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Assets/AssetTypes.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#else
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#endif

namespace r2::asset
{
    class AssetFile;
    class AssetLoader;
    class AssetBuffer;
    class Asset;
    class DefaultAssetLoader;
    
    using AssetLoadProgressCallback = std::function<void(int, bool&)>;

    
    struct AssetCacheRecord
    {
        r2::asset::AssetHandle handle;
        r2::asset::AssetBuffer* buffer = nullptr;
    };
    
    class AssetCache
    {
    public:
        
        static const u32 LRU_CAPACITY = 1024;
        static const u32 MAP_CAPACITY = 1024;
        
        using AssetReloadedFunc = std::function<void (AssetHandle asset)>;
        
        explicit AssetCache(u64 slot, const r2::mem::utils::MemBoundary& boundary);

        bool Init(FileList noptrList, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        
        void Shutdown();
        
        void RegisterAssetLoader(AssetLoader* assetLoader);
        
        //The handle should remain valid across the run of the game
        AssetHandle LoadAsset(const Asset& asset);
        bool HasAsset(const Asset& asset);

        AssetHandle ReloadAsset(const Asset& asset);

        //Should not keep this pointer around for longer than necessary to use it as it can change in debug
        AssetCacheRecord GetAssetBuffer(AssetHandle handle);

        bool ReturnAssetBuffer(const AssetCacheRecord& buffer);
        
        void FlushAll();
        
        int Preload(const char* pattern, AssetLoadProgressCallback callback);

        //To be used for files and loaders
        template<class T>
        AssetLoader* MakeAssetLoader()
        {
            //bool value = std::is_convertible<AssetLoader*, T*>::value;
           // R2_CHECK(value, "Passed in type is not have AssetLoader as it's base type!");
            return ALLOC(T, mAssetCacheArena);
        }
        
        u64 GetSlot() const {return mSlot;}
#ifdef R2_ASSET_PIPELINE
        void AddReloadFunction(AssetReloadedFunc func);
#endif
        
        static u64 TotalMemoryNeeded(u32 headerSize, u32 boundsChecking, u64 numAssets, u64 assetCapacity, u64 alignment, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        static u64 CalculateCacheSizeNeeded(u64 initialAssetCapcity, u64 numAssets, u64 alignment);
    private:
        
        struct AssetBufferRef
        {
            AssetBuffer* mAssetBuffer = nullptr;
            s32 mRefCount = 0;
        };
        
        using AssetList = r2::SQueue<AssetHandle>*;
        using AssetMap = r2::SHashMap<AssetBufferRef>*;
        //using AssetFileMap = r2::SHashMap<FileHandle>*;
        using AssetLoaderList = r2::SArray<AssetLoader*>*;
        using AssetNameMap = r2::SHashMap<Asset>*;
       
        AssetBuffer* Load(const Asset& asset, bool startCountAtOne = false);
        void UpdateLRU(AssetHandle handle);
        FileHandle FindInFiles(const Asset& asset);
        AssetBufferRef& Find(AssetHandle handle, AssetBufferRef& theDefault);
        
        void Free(AssetHandle handle, bool forceFree);
        bool MakeRoom(u64 amount);
        void FreeOneResource(bool forceFree);
        s64 GetLRUIndex(AssetHandle handle);
        void RemoveFromLRU(AssetHandle handle);

        u64 MemoryHighWaterMark();
	

        FileList mnoptrFiles;
        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetLoaderList mAssetLoaders;
        AssetNameMap mAssetNameMap;

        DefaultAssetLoader* mDefaultLoader;
        s64 mSlot;
        u64 mMemoryHighWaterMark;
        
        r2::mem::PoolArena* mAssetBufferPoolPtr;
     //   AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
#ifdef R2_ASSET_PIPELINE
        //This is for debug only
        r2::mem::MallocArena mAssetCacheArena;
#else
        
        r2::mem::FreeListArena mAssetCacheArena;
#endif
        
#ifdef R2_ASSET_PIPELINE
        struct AssetRecord
        {
            AssetHandle handle;
            std::string name;
            AssetType type;
        };
        struct AssetsToFile
        {
            FileHandle file;
            std::vector<AssetRecord> assets;
        };
        
        std::vector<AssetReloadedFunc> mReloadFunctions;
        
        void AddAssetToAssetsForFileList(FileHandle fileHandle, const AssetRecord& assetRecord);
        void RemoveAssetFromAssetForFileList(AssetHandle assetHandle);
        void InvalidateAssetsForFile(FileHandle fileHandle);

        std::vector<AssetsToFile> mAssetsForFiles;
#endif
        //Debug stuff
#if R2_ASSET_CACHE_DEBUG 
        void PrintLRU();
        void PrintAssetMap();
        void PrintAllAssetsInFiles();
        void PrintAssetsInFile(AssetFile* file);
        void PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount);
#endif
    };
}

#endif /* AssetCache_h  */
