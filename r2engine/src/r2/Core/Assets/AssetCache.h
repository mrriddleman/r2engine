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
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include <type_traits>

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
    
    class AssetCache
    {
    public:
        
        static const u32 LRU_CAPACITY = 1000;
        static const u32 MAP_CAPACITY = 1000;
        
        AssetCache();

        bool Init(r2::mem::utils::MemBoundary boundary, FileList list, u32 lruCapacity = LRU_CAPACITY, u32 mapCapacity =MAP_CAPACITY);
        
        void Shutdown();
        
        void RegisterAssetLoader(AssetLoader* assetLoader);
        
        AssetBuffer* GetAssetBuffer(const Asset& asset);
        
        bool ReturnAssetBuffer(AssetBuffer* buffer);
        
        void FlushAll();
        
        int Preload(const char* pattern, AssetLoadProgressCallback callback);
        
        //To be used for files and loaders
        template<class T>
        AssetLoader* MakeAssetLoader()
        {
            //bool value = std::is_convertible<AssetLoader*, T*>::value;
           // R2_CHECK(value, "Passed in type is not have AssetLoader as it's base type!");
            return ALLOC(T, mMallocArena);
        }
        
        template<class T>
        AssetFile* MakeAssetFile()
        {
          //  bool value = std::is_base_of<AssetFile, T>::value;
          //  R2_CHECK(value, "Passed in type is not have AssetFile as it's base type!");
            return ALLOC(T, mMallocArena);
        }
        
        //NOTE: don't use T* for the type here, it's taken care of for you
       
        FileList MakeFileList(u64 capacity)
        {
            return MAKE_SARRAY(mMallocArena, AssetFile*, capacity);
        }
        
    private:
        
        struct AssetBufferRef
        {
            AssetBuffer* mAssetBuffer = nullptr;
            u32 mRefCount = 0;
        };
        
        using AssetList = r2::SQueue<AssetHandle>*;
        using AssetMap = r2::SHashMap<AssetBufferRef>*;
        //using AssetFileMap = r2::SHashMap<FileHandle>*;
        using AssetLoaderList = r2::SArray<AssetLoader*>*;
       
        AssetBuffer* Load(const Asset& asset);
        void UpdateLRU(AssetHandle handle);
        FileHandle FindInFiles(const Asset& asset);
        AssetBufferRef& Find(AssetHandle handle, AssetBufferRef& theDefault);
        
        
        void Free(AssetHandle handle, bool decrementRefCount, bool forceFree);
        bool MakeRoom(u64 amount);
        void FreeOneResource(bool decrementRefCount, bool forceFree);
        s64 GetLRUIndex(AssetHandle handle);
        void RemoveFromLRU(AssetHandle handle);
        
        FileList mFiles;
        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetLoaderList mAssetLoaders;
        DefaultAssetLoader* mDefaultLoader;
     //   AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
        
        //@TODO(Serge): figure out the Allocator/memory scheme
        
        //This is for debug only
        r2::mem::MallocArena mMallocArena;
        
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
