//
//  AssetLib.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-16.
//

#include "AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Utils/Hash.h"
#include <queue>

namespace r2::asset::lib
{
    r2::mem::utils::MemBoundary s_boundary;
    r2::SArray<AssetCache*>* s_assetCaches = nullptr;
    r2::SHashMap<u64>* s_fileToAssetCacheMap = nullptr;
#ifdef R2_ASSET_PIPELINE
    std::queue<std::vector<std::string>> s_assetsBuiltQueue;
    std::vector<AssetFilesBuiltListener> s_listeners;
#endif
    //Change to something real
    r2::mem::MallocArena s_arena{r2::mem::utils::MemBoundary()};

    const u32 MAX_FILES_TO_ASSET_CACHE_CAPACITY = 2000;
    
    bool Init(u32 numAssetCaches)
    {
        s_assetCaches = MAKE_SARRAY(s_arena, AssetCache*, numAssetCaches);
        s_fileToAssetCacheMap = MAKE_SHASHMAP(s_arena, u64, MAX_FILES_TO_ASSET_CACHE_CAPACITY);
        
        return s_assetCaches != nullptr && s_fileToAssetCacheMap != nullptr;
    }
    
    void Update()
    {
        if (s_assetCaches && s_fileToAssetCacheMap)
        {
            while (!s_assetsBuiltQueue.empty())
            {
                for (r2::asset::lib::AssetFilesBuiltListener listener : s_listeners)
                {
                    listener(s_assetsBuiltQueue.front());
                }
                
                s_assetsBuiltQueue.pop();
            }
        }
    }
    
    void Shutdown()
    {
        if (s_assetCaches && s_fileToAssetCacheMap)
        {
            u64 size = r2::sarr::Size(*s_assetCaches);
            
            for (u64 i = 0; i < size; ++i)
            {
                r2::asset::AssetCache* cache = r2::sarr::At(*s_assetCaches, i);
                FREE(cache, s_arena);
            }
            
            FREE(s_fileToAssetCacheMap, s_arena);
            FREE(s_assetCaches, s_arena);
        }
    }
    
    void AddFiles(const r2::asset::AssetCache& cache, FileList list)
    {
        if (s_assetCaches && s_fileToAssetCacheMap)
        {
            u64 numFiles = r2::sarr::Size(*list);
            
            for (u64 i = 0; i < numFiles; ++i)
            {
                r2::asset::AssetFile* file = r2::sarr::At(*list, i);
                u64 hash = r2::utils::Hash<const char*>{}(file->FilePath());
                
                r2::shashmap::Set(*s_fileToAssetCacheMap, hash, cache.GetSlot());
            }
        }
    }
    
#ifdef R2_ASSET_PIPELINE
    void PushFilesBuilt(std::vector<std::string> paths)
    {
        s_assetsBuiltQueue.push(paths);
    }
    
    void AddAssetFilesBuiltListener(AssetFilesBuiltListener func)
    {
        s_listeners.push_back(func);
    }
#endif
    
    r2::asset::AssetCache* CreateAssetCache(r2::mem::utils::MemBoundary boundary)
    {
        if (s_assetCaches)
        {
            u64 size = r2::sarr::Size(*s_assetCaches);
            
            r2::asset::AssetCache* cache = ALLOC_PARAMS(r2::asset::AssetCache, s_arena, size);
            
            if (cache)
            {
                r2::sarr::Push(*s_assetCaches, cache);
            }
            
            return cache;
        }
        
        return nullptr;
    }
}
