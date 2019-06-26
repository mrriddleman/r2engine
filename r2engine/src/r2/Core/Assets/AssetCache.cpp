//
//  AssetCache.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//

#include "AssetCache.h"

#define NOT_INITIALIZED !mFiles || !mAssetLRU || !mAssetMap || !mAssetFileMap

namespace r2::asset
{
    AssetCache::AssetCache()
        : mFiles(nullptr)
        , mAssetLRU(nullptr)
        , mAssetMap(nullptr)
        , mAssetFileMap(nullptr)
        , mMallocArena(r2::mem::utils::MemBoundary())
    {
        
    }
    
    bool AssetCache::Init(r2::mem::utils::MemBoundary boundary, FileList list)
    {
        return false;
    }
    
    void AssetCache::Shutdown()
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
    }
    
    AssetBuffer* AssetCache::GetAssetBuffer(const Asset& asset)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return nullptr;
        }
        
        return nullptr;
    }
    
    void AssetCache::FlushAll()
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
    }

    //Private
    AssetBuffer* AssetCache::Load(AssetHandle handle)
    {
        return nullptr;
    }
    
    void AssetCache::UpdateLRU(AssetHandle handle)
    {
        
    }
    
    u64 AssetCache::FindInFiles(const Asset& asset)
    {
        return 0;
    }
    
    u64 AssetCache::FindAsset(u64 fileIndex, const Asset& asset)
    {
        return 0;
    }
    
    byte* AssetCache::Allocate(u64 size, u64 alignment)
    {
        return nullptr;
    }
    
    void AssetCache::Free(AssetHandle handle)
    {
        
    }
    
    bool AssetCache::MakeRoom(u64 amount)
    {
        return false;
    }
}
