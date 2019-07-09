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
        AssetCache();
        bool Init(r2::mem::utils::MemBoundary boundary, FileList list);
        void Shutdown();
        
        void RegisterAssetLoader(AssetLoader* assetLoader);
        
        AssetBuffer* GetAssetBuffer(const Asset& asset);
        void FlushAll();
        
        int Preload(const char* pattern, AssetLoadProgressCallback callback);
        
        //To be used for files and loaders
        byte* Allocate(u64 size, u64 alignment);
        
    private:
        
        using AssetList = r2::SQueue<AssetHandle>*;
        using AssetMap = r2::SHashMap<AssetBuffer*>*;
        //using AssetFileMap = r2::SHashMap<FileHandle>*;
        using AssetLoaderList = r2::SArray<AssetLoader*>*;
       
        AssetBuffer* Load(const Asset& asset);
        void UpdateLRU(AssetHandle handle);
        FileHandle FindInFiles(const Asset& asset);
        AssetBuffer* Find(AssetHandle handle);
        
        
        void Free(AssetHandle handle);
        bool MakeRoom(u64 amount);
        void FreeOneResource();
        s64 GetLRUIndex(AssetHandle handle);
        
        
        FileList mFiles;
        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetLoaderList mAssetLoaders;
        DefaultAssetLoader* mDefaultLoader;
     //   AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
        
        //@TODO(Serge): figure out the Allocator/memory scheme
        
        //This is for debug only
        r2::mem::MallocArena mMallocArena;
        
    };
}

#endif /* AssetCache_h  */
