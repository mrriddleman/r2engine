//
//  AssetCache.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//

#include "AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetLoader.h"
#include "r2/Core/Assets/DefaultAssetLoader.h"

#define NOT_INITIALIZED !mFiles || !mAssetLRU || !mAssetMap || !mAssetLoaders

namespace
{
    const u64 MAP_CAPACITY = 1000;
    const u64 LRU_CAPACITY = 1000;
    const s64 INVALID_FILE_INDEX = -1;
    const u64 INVALID_ASSET_ID = 0;
}

namespace r2::asset
{
    AssetCache::AssetCache()
        : mFiles(nullptr)
        , mAssetLRU(nullptr)
        , mAssetMap(nullptr)
        , mAssetLoaders(nullptr)
    //    , mAssetFileMap(nullptr)
        , mMallocArena(r2::mem::utils::MemBoundary())
    {
        
    }
    
    bool AssetCache::Init(r2::mem::utils::MemBoundary boundary, FileList list)
    {
        //@TODO(Serge): do something with the memory boundary - for now we don't care, just malloc!
        
        mFiles = list;
        
        //Memory allocations for our lists and maps
        {
            mAssetLRU = MAKE_SQUEUE(mMallocArena, AssetHandle, LRU_CAPACITY);
            mAssetMap = MAKE_SHASHMAP(mMallocArena, AssetBufferRef, MAP_CAPACITY);
          //  mAssetFileMap = MAKE_SHASHMAP(mMallocArena, FileHandle, MAP_CAPACITY);
            mDefaultLoader = ALLOC(DefaultAssetLoader, mMallocArena);
            
            mAssetLoaders = MAKE_SARRAY(mMallocArena, AssetLoader*, LRU_CAPACITY);
        }
        
        
        
        
        return true;
    }
    
    void AssetCache::Shutdown()
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
        
        FlushAll();
        
        u64 numLoaders = r2::sarr::Size(*mAssetLoaders);
        for (u64 i = 0; i < numLoaders; ++i)
        {
            AssetLoader* loader = r2::sarr::At(*mAssetLoaders, i);
            FREE(loader, mMallocArena);
        }
        
        u64 numFiles = r2::sarr::Size(*mFiles);
        for (u64 i = 0; i < numFiles; ++i)
        {
            AssetFile* file = r2::sarr::At(*mFiles, i);
            FREE(file, mMallocArena);
        }
        
        FREE(mDefaultLoader, mMallocArena);
        FREE(mAssetLRU, mMallocArena);
        FREE(mAssetMap, mMallocArena);
        FREE(mAssetLoaders, mMallocArena);
        //FREE(mAssetFileMap, mMallocArena);
        
        mAssetLRU = nullptr;
        mAssetMap = nullptr;
        mAssetLoaders = nullptr;
     //   mAssetFileMap = nullptr;
    }
    
    AssetBuffer* AssetCache::GetAssetBuffer(const Asset& asset)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return nullptr;
        }
        
        AssetHandle handle = asset.HashID();
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(handle, theDefault);
        
        if (bufferRef.mAssetBuffer != nullptr)
        {
            UpdateLRU(handle);
            
            ++bufferRef.mRefCount;
            
            return bufferRef.mAssetBuffer;
        }
        
        return Load(asset);
    }
    
    bool AssetCache::ReturnAssetBuffer(AssetBuffer* buffer)
    {
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(buffer->GetAsset().HashID(), theDefault);
        
        bool found = bufferRef.mAssetBuffer != nullptr;
        
        if (found)
        {
            Free(buffer->GetAsset().HashID(), false);
        }
        
        return found;
    }
    
    void AssetCache::FlushAll()
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
        
        const u64 size = r2::squeue::Size(*mAssetLRU);
        
        for (u64 i = 0; i < size; ++i)
        {
            FreeOneResource(true);
        }
    }
    
    int AssetCache::Preload(const char* pattern, AssetLoadProgressCallback callback)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return 0;
        }
        
        return 0;
    }
    
    void AssetCache::RegisterAssetLoader(AssetLoader* optrAssetLoader)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
        
        r2::sarr::Push(*mAssetLoaders, optrAssetLoader);
    }

    //Private
    AssetBuffer* AssetCache::Load(const Asset& asset)
    {
        u64 numAssetLoaders = r2::sarr::Size(*mAssetLoaders);
        AssetLoader* loader = nullptr;
        for (u64 i = 0; i < numAssetLoaders; ++i)
        {
            AssetLoader* nextLoader = r2::sarr::At(*mAssetLoaders, i);
            if (r2::util::WildcardMatch(nextLoader->GetPattern(), asset.Name()))
            {
                loader = nextLoader;
                break;
            }
        }
        
        if (loader != nullptr)
        {
            loader = mDefaultLoader;
        }
        
        R2_CHECK(loader != nullptr, "couldn't find asset loader");
        
        FileHandle fileIndex = FindInFiles(asset);
        if(fileIndex == INVALID_ASSET_ID)
        {
            R2_CHECK(false, "We failed to find the asset in any of our asset files");
            return nullptr;
        }
        
        AssetFile* theAssetFile = r2::sarr::At(*mFiles, fileIndex);
        R2_CHECK(theAssetFile != nullptr, "We have a null asset file?");
        
        if (!theAssetFile->IsOpen())
        {
            theAssetFile->Open();
        }
        
        
        AssetHandle handle = asset.HashID();
        u64 rawAssetSize = theAssetFile->RawAssetSizeFromHandle(handle);
        
        byte* rawAssetBuffer = Allocate(rawAssetSize, 4);
        
        R2_CHECK(rawAssetBuffer != nullptr, "failed to allocate asset handle: %lli of size: %llu", handle, rawAssetSize);
        
        theAssetFile->GetRawAssetFromHandle(handle, rawAssetBuffer);
        
        if (rawAssetBuffer == nullptr)
        {
            R2_CHECK(false, "Failed to get the raw data from the asset: %s\n", asset.Name());
            return nullptr;
        }

        AssetBuffer* assetBuffer = nullptr;
        
        assetBuffer = ALLOC(AssetBuffer, mMallocArena);
        
        R2_CHECK(assetBuffer != nullptr, "Failed to allocate a new asset buffer for asset: %s\n", asset.Name());
        
        if (assetBuffer == nullptr)
        {
            return nullptr;
        }
        
        if (!loader->ShouldProcess())
        {
            assetBuffer->Load(asset, rawAssetBuffer, rawAssetSize);
        }
        else
        {
            u64 size = loader->GetLoadedAssetSize(rawAssetBuffer, rawAssetSize);
            byte* buffer = Allocate(size, 4);
            
            R2_CHECK(buffer != nullptr, "Failed to allocate buffer!");
            
            if (!buffer)
            {
                return nullptr;
            }
            
            assetBuffer->Load(asset, buffer, size);
            
            bool success = loader->LoadAsset(rawAssetBuffer, rawAssetSize, *assetBuffer);
            
            R2_CHECK(success, "Failed to load asset");
            
            FREE(rawAssetBuffer, mMallocArena);
            
            if (!success)
            {
                return nullptr;
            }
        }
        AssetBufferRef bufferRef;
        bufferRef.mRefCount = 1;
        bufferRef.mAssetBuffer = assetBuffer;

        r2::shashmap::Set(*mAssetMap, handle, bufferRef);
        UpdateLRU(handle);
        
        return assetBuffer;
    }
    
    void AssetCache::UpdateLRU(AssetHandle handle)
    {
        s64 lruIndex = GetLRUIndex(handle);
        
        if (lruIndex == -1)
        {
            if (r2::squeue::Size(*mAssetLRU) + 1 > LRU_CAPACITY)
            {
                FreeOneResource(true);
                //R2_CHECK(false, "AssetCache::UpdateLRU() - We have too many assets in our LRU");
                return;
            }
            
            r2::squeue::PushFront(*mAssetLRU, handle);
        }
        else
        {
            //otherwise move it to the front
            r2::squeue::MoveToFront(*mAssetLRU, lruIndex);
        }
    }
    
    FileHandle AssetCache::FindInFiles(const Asset& asset)
    {
        const AssetHandle handle = asset.HashID();
        
        const u64 numFiles = r2::sarr::Size(*mFiles);
        
        for (u64 i = 0; i < numFiles; ++i)
        {
            const AssetFile* file = r2::sarr::At(*mFiles, i);
            
            const u64 numAssets = file->NumAssets();
            
            for (u64 j = 0; j < numAssets; ++j)
            {
                if (file->GetAssetHandle(j) == handle)
                {
                    return i;
                }
            }
        }
        
        return INVALID_FILE_INDEX;
    }
    
    AssetCache::AssetBufferRef& AssetCache::Find(AssetHandle handle, AssetCache::AssetBufferRef& theDefault)
    {
        return r2::shashmap::Get(*mAssetMap, handle, theDefault);
    }
    
    byte* AssetCache::Allocate(u64 size, u64 alignment)
    {
        return ALLOC_BYTESN(mMallocArena, size, alignment);
    }
    
    void AssetCache::Free(AssetHandle handle, bool forceFree)
    {
        AssetBufferRef theDefault;
        
        AssetBufferRef& assetBufferRef = Find(handle, theDefault);
        
        if (assetBufferRef.mAssetBuffer != nullptr)
        {
            --assetBufferRef.mRefCount;
            
            if (forceFree && assetBufferRef.mRefCount != 0)
            {
                R2_CHECK(false, "AssetCache::Free() - we're trying to free the asset: %s but we still have %u references to it!", assetBufferRef.mAssetBuffer->GetAsset().Name(),  assetBufferRef.mRefCount);
            }
            
            if (assetBufferRef.mRefCount == 0)
            {
                FREE( assetBufferRef.mAssetBuffer->MutableData(), mMallocArena);
                
                FREE(assetBufferRef.mAssetBuffer, mMallocArena);
            
                r2::shashmap::Remove(*mAssetMap, handle);
                
                RemoveFromLRU(handle);
            }
        }
    }
    
    bool AssetCache::MakeRoom(u64 amount)
    {
        return false;
    }
    
    void AssetCache::FreeOneResource(bool forceFree)
    {
        const u64 size = r2::squeue::Size(*mAssetLRU);
        
        if (size > 0)
        {
            AssetHandle handle = r2::squeue::Last(*mAssetLRU);
            Free(handle, forceFree);
            
            RemoveFromLRU(handle);
        }
    }
    
    s64 AssetCache::GetLRUIndex(AssetHandle handle)
    {
        u64 queueSize = r2::squeue::Size(*mAssetLRU);
        
        for (u64 i = 0; i < queueSize; ++i)
        {
            if ((*mAssetLRU)[i] == handle)
            {
                return i;
            }
        }
        
        return -1;
    }
    
    void AssetCache::RemoveFromLRU(AssetHandle handle)
    {
        s64 lruIndex = GetLRUIndex(handle);
        
        if (lruIndex != -1)
        {
            r2::squeue::MoveToFront(*mAssetLRU, lruIndex);
            r2::squeue::PopFront(*mAssetLRU);
        }
    }
}
