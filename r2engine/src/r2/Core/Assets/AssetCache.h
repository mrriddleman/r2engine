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
#include "r2/Core/Assets/AssetCacheRecord.h"
#include "r2/Core/File/FileTypes.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#else
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#endif



namespace r2::asset
{
    class AssetFile;
    class AssetLoader;
    class AssetWriter;
    class AssetBuffer;
    class Asset;
    class DefaultAssetLoader;
    


    class AssetCache
    {
    public:
        
        static const u32 LRU_CAPACITY = 1024;
        static const u32 MAP_CAPACITY = 1024;

        explicit AssetCache(u64 slot, const r2::mem::utils::MemBoundary& boundary);

        bool Init(u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        
        void Shutdown();
        
        void RegisterAssetLoader(AssetLoader* assetLoader);


        //The handle should remain valid across the run of the game
        AssetHandle LoadAsset(const Asset& asset);
        bool IsAssetLoaded(const Asset& asset);

        AssetHandle ReloadAsset(const Asset& asset);
        void FreeAsset(const AssetHandle& handle);

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

        s64 GetSlot() const {return mSlot;}

        void RegisterAssetFreedCallback(AssetFreedCallback func);

        static u64 TotalMemoryNeeded(u64 numAssets, u64 assetCapacity, u64 alignment, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        static u64 CalculateCacheSizeNeeded(u64 initialAssetCapcity, u64 numAssets, u64 alignment);


    private:
        
        friend struct AssetCacheRecord;

        struct AssetBufferRef
        {
            Asset mAsset;
            AssetBuffer* mAssetBuffer = nullptr;
            s32 mRefCount = 0;
        };
        
        using AssetList = r2::SQueue<AssetHandle>*;
        using AssetMap = r2::SArray<AssetBufferRef>*;
        using AssetLoaderList = r2::SArray<AssetLoader*>*;
        using AssetFreedCallbackList = r2::SArray<AssetFreedCallback>*;

        bool IsLoaded(const Asset& asset);

        AssetBuffer* Load(const Asset& asset);

        void UpdateLRU(AssetHandle handle);

        AssetBufferRef& Find(u64 handle, AssetBufferRef& theDefault);
        void RemoveAssetBuffer(u64 handle);

        void Free(AssetHandle handle, bool forceFree);
        bool MakeRoom(u64 amount);
        void FreeOneResource(bool forceFree);
        s64 GetLRUIndex(AssetHandle handle);
        void RemoveFromLRU(AssetHandle handle);

        u64 MemoryHighWaterMark();
	

        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetLoaderList mAssetLoaders;

        AssetFreedCallbackList mAssetFreedCallbackList;

        DefaultAssetLoader* mDefaultLoader;
        s64 mSlot;
        u64 mMemoryHighWaterMark;
        r2::mem::utils::MemBoundary mMemoryBoundary;

        r2::mem::PoolArena* mAssetBufferPoolPtr;

#ifdef R2_ASSET_PIPELINE
        //This is for debug only
        r2::mem::MallocArena mAssetCacheArena;
#else
        
        r2::mem::FreeListArena mAssetCacheArena;
#endif
        
        //Debug stuff
#if R2_ASSET_CACHE_DEBUG 
        void PrintLRU();
        void PrintAssetMap();
        void PrintHighWaterMark();
        void PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount);
#endif
    };
}

#endif /* AssetCache_h  */
