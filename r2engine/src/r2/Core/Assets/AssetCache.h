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
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#else
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#endif

#define ASSET_CACHE_DEBUG 1

namespace r2::asset
{
    class AssetFile;
    class AssetLoader;
    class AssetBuffer;
    class Asset;
    class DefaultAssetLoader;
    
    using FileList = r2::SArray<AssetFile*>*;
    using AssetHandle = u64;
    using FileHandle = s64;
    using AssetLoadProgressCallback = std::function<void (int, bool&)>;
    
    const u64 INVALID_ASSET_HANDLE = 0;
    
    struct AssetCacheRecord
    {
        r2::asset::AssetHandle handle = INVALID_ASSET_HANDLE;
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
        
        static u64 TotalMemoryNeeded(u64 assetCapacity, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        
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
       
        AssetBuffer* Load(const Asset& asset, bool startCountAtOne = false);
        void UpdateLRU(AssetHandle handle);
        FileHandle FindInFiles(const Asset& asset);
        AssetBufferRef& Find(AssetHandle handle, AssetBufferRef& theDefault);
        
        void Free(AssetHandle handle, bool forceFree);
        bool MakeRoom(u64 amount);
        void FreeOneResource(bool forceFree);
        s64 GetLRUIndex(AssetHandle handle);
        void RemoveFromLRU(AssetHandle handle);

        FileList mnoptrFiles;
        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetLoaderList mAssetLoaders;
        DefaultAssetLoader* mDefaultLoader;
        u64 mSlot;
        
        r2::mem::PoolArena* mAssetBufferPoolPtr;
     //   AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
#ifdef R2_ASSET_PIPELINE
        r2::mem::MallocArena mAssetCacheArena;
#else
        //This is for debug only
        r2::mem::FreeListArena mAssetCacheArena;
#endif
        
        //@TODO(Serge): put in R2_DEBUG
#ifdef R2_ASSET_PIPELINE
        struct AssetRecord
        {
            AssetHandle handle;
            std::string name;
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
#if ASSET_CACHE_DEBUG 
        void PrintLRU();
        void PrintAssetMap();
        void PrintAllAssetsInFiles();
        void PrintAssetsInFile(AssetFile* file);
        void PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount);
#endif
    };
}

#endif /* AssetCache_h  */
