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

        bool Init(FileList noptrList, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        
        void Shutdown();
        
        void RegisterAssetLoader(AssetLoader* assetLoader);
        void RegisterAssetWriter(AssetWriter* assetWriter);

        //The handle should remain valid across the run of the game
        AssetHandle LoadAsset(const Asset& asset);
        bool HasAsset(const Asset& asset);

        AssetHandle ReloadAsset(const Asset& asset);
        void FreeAsset(const AssetHandle& handle);

        void WriteAssetFile(const Asset& asset, const void* data, u32 size, u32 offset, r2::fs::FileMode mode);

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
        
        template<class T>
        AssetWriter* MakeAssetWriter()
        {
            return ALLOC(T, mAssetCacheArena);
        }

        s64 GetSlot() const {return mSlot;}
#ifdef R2_ASSET_PIPELINE
        //@TODO(Serge): get rid of all of this
        void AddReloadFunction(AssetReloadedFunc func);

        void ResetFileList(FileList fileList);

        std::vector<AssetFile*> GetAllAssetFilesForAssetType(r2::asset::AssetType type);

        const AssetFile* GetAssetFileForAsset(const r2::asset::Asset& asset);

        const AssetFile* GetAssetFileForAssetHandle(const r2::asset::AssetHandle& assetHandle);

#endif
        void ClearFileList();

        void RegisterAssetFreedCallback(AssetFreedCallback func);
        
        //@TODO(Serge): get rid of these
        const FileList GetFileList() const { return mnoptrFiles; }
        const AssetFile* GetAssetFile(const Asset& asset) const;
        const AssetFile* GetAssetFile(const r2::asset::AssetHandle& assetHandle) const;
        void AddAssetFile(AssetFile* assetFile);
        void RemoveFile(const Asset& asset);



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
        using AssetWriterList = r2::SArray<AssetWriter*>*;
        using AssetFreedCallbackList = r2::SArray<AssetFreedCallback>*;

        bool IsLoaded(const Asset& asset);

        AssetBuffer* Load(const Asset& asset);
        void Write(const Asset& asset, const void* data, u32 size, u32 offset, r2::fs::FileMode mode);

        void UpdateLRU(AssetHandle handle);
        FileHandle FindInFiles(u64 assetID) const;
        AssetBufferRef& Find(u64 handle, AssetBufferRef& theDefault);
        void RemoveAssetBuffer(u64 handle);

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
        AssetWriterList mAssetWriters;
        AssetFreedCallbackList mAssetFreedCallbackList;

        DefaultAssetLoader* mDefaultLoader;
        s64 mSlot;
        u64 mMemoryHighWaterMark;
        r2::mem::utils::MemBoundary mMemoryBoundary;

        r2::mem::PoolArena* mAssetBufferPoolPtr;
     //   AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
#ifdef R2_ASSET_PIPELINE
        //This is for debug only
        r2::mem::MallocArena mAssetCacheArena;
#else
        
        r2::mem::FreeListArena mAssetCacheArena;
#endif
        
#ifdef R2_ASSET_PIPELINE
        //@TODO(Serge): get rid of all of this
        struct AssetRecord
        {
            AssetHandle handle;
            std::string name;
            AssetType type;
        };
        struct AssetsToFile
        {
            AssetFile* assetFile = nullptr;
            std::vector<AssetRecord> assets;
        };
        
        std::vector<AssetReloadedFunc> mReloadFunctions;
        
        void AddAssetToAssetsForFileList(FileHandle fileHandle, const AssetRecord& assetRecord);
        void RemoveAssetFromAssetForFileList(AssetHandle assetHandle);
        void InvalidateAssetsForFile(AssetFile* file);

        std::vector<AssetsToFile> mAssetsForFiles;

#endif
        //Debug stuff
#if R2_ASSET_CACHE_DEBUG 
        void PrintLRU();
        void PrintAssetMap();
        void PrintHighWaterMark();
        void PrintAllAssetsInFiles();
        void PrintAssetsInFile(AssetFile* file);
        void PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount);
#endif
    };
}

#endif /* AssetCache_h  */
