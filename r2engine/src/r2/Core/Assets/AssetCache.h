//
//  AssetCache.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//

#ifndef AssetCache_h
#define AssetCache_h


#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"

namespace r2::asset
{
    using FileList = r2::SArray<AssetFile*>*;
    
    using AssetHandle = u64;
    
    class AssetCache
    {
    public:
        AssetCache();
        bool Init(r2::mem::utils::MemBoundary boundary, FileList list);
        void Shutdown();
        
        AssetBuffer* GetAssetBuffer(const Asset& asset);
        void FlushAll();
        
        
    private:
        
        using AssetList = r2::SArray<u64>*;
        using AssetMap = r2::SHashMap<AssetBuffer*>*;
        using AssetFileMap = r2::SHashMap<u64>*;

        AssetBuffer* Load(AssetHandle handle);
        void UpdateLRU(AssetHandle handle);
        u64 FindInFiles(const Asset& asset);
        u64 FindAsset(u64 fileIndex, const Asset& asset);
        
        byte* Allocate(u64 size, u64 alignment);
        void Free(AssetHandle handle);
        bool MakeRoom(u64 amount);
        
        
        FileList mFiles;
        AssetList mAssetLRU;
        AssetMap mAssetMap;
        AssetFileMap mAssetFileMap; //this maps from an asset id to a file index in mFiles
        
        //@TODO(Serge): figure out the Allocator/memory scheme
        
        //@TODO(Serge): this is for debug only
        r2::mem::MallocArena mMallocArena;
        
    };
}

#endif /* AssetCache_h  */
