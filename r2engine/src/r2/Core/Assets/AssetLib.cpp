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
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#endif

namespace r2::asset::lib
{
    r2::mem::utils::MemBoundary s_boundary;
    AssetCache** s_assetCaches = nullptr;
    u64 s_numCaches = 0;
    r2::SHashMap<u64>* s_fileToAssetCacheMap = nullptr;
#ifdef R2_ASSET_PIPELINE
    r2::asset::pln::AssetThreadSafeQueue<std::vector<std::string>> s_assetsBuiltQueue;
    //std::queue<std::vector<std::string>> s_assetsBuiltQueue;
    std::vector<AssetFilesBuiltListener> s_listeners;
#endif
    //Change to something real
    r2::mem::MallocArena s_arena{r2::mem::utils::MemBoundary()};

    const u32 MAX_FILES_TO_ASSET_CACHE_CAPACITY = 2000;
    const u32 MAX_ASSET_CACHES = 1000;
    
    bool Init()
    {
        s_assetCaches = ALLOC_ARRAY(r2::asset::AssetCache*[MAX_ASSET_CACHES], s_arena);
        
        for (u64 i = 0; i < MAX_ASSET_CACHES; ++i)
        {
            s_assetCaches[i] = nullptr;
        }
        
        s_fileToAssetCacheMap = MAKE_SHASHMAP(s_arena, u64, MAX_FILES_TO_ASSET_CACHE_CAPACITY);
        
        return s_assetCaches != nullptr && s_fileToAssetCacheMap != nullptr;
    }
    
    void Update()
    {
#ifdef R2_ASSET_PIPELINE
        if (s_assetCaches && s_fileToAssetCacheMap)
        {
            std::vector<std::string> paths;
            
            if(s_assetsBuiltQueue.TryPop(paths))
            {
                for (r2::asset::lib::AssetFilesBuiltListener listener : s_listeners)
                {
                    listener(paths);
                }
            }
#endif
        }
    }
    
    void Shutdown()
    {
        if (s_assetCaches && s_fileToAssetCacheMap)
        {

            for (u64 i = 0; i < MAX_ASSET_CACHES; ++i)
            {
                r2::asset::AssetCache* cache = s_assetCaches[i];
                if (cache != nullptr)
                {
                    FREE(cache, s_arena);
                }
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
        s_assetsBuiltQueue.Push(paths);
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
            s64 slot = -1;
            for (u64 i = 0; i < MAX_ASSET_CACHES; ++i)
            {
                if (s_assetCaches[i] == nullptr)
                {
                    slot = i;
                    break;
                }
            }
            
            if (slot == -1)
            {
                return nullptr;
            }
            
            r2::asset::AssetCache* cache = ALLOC_PARAMS(r2::asset::AssetCache, s_arena, slot);
            
            if (cache)
            {
                s_assetCaches[slot] = cache;
            }
            
            return cache;
        }
        
        return nullptr;
    }
    
    void DestroyCache(r2::asset::AssetCache* cache)
    {
        if (s_assetCaches)
        {
            u64 slot = cache->GetSlot();
            
            R2_CHECK(s_assetCaches[slot] == cache, "We don't have the proper cache for this slot!");
            
            cache->Shutdown();
            
            FREE(cache, s_arena);
            
            s_assetCaches[slot] = nullptr;
        }
    }
}
