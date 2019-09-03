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
#include "r2/Core/Assets/RawAssetFile.h"
#include "r2/Core/Assets/ZipAssetFile.h"

//@TODO(Serge): add debug define
#include "r2/Core/Assets/Pipeline/AssetWatcher.h"

#define NOT_INITIALIZED !mFiles || !mAssetLRU || !mAssetMap || !mAssetLoaders

namespace
{
    const s64 INVALID_FILE_INDEX = -1;
}

namespace r2::asset
{
    
    const u32 AssetCache::LRU_CAPACITY;
    const u32 AssetCache::MAP_CAPACITY;
    
    AssetCache::AssetCache()
        : mFiles(nullptr)
        , mAssetLRU(nullptr)
        , mAssetMap(nullptr)
        , mAssetLoaders(nullptr)
    //    , mAssetFileMap(nullptr)
        , mMallocArena(r2::mem::utils::MemBoundary())
    {
        
    }
    
    bool AssetCache::Init(r2::mem::utils::MemBoundary boundary, FileList list, u32 lruCapacity, u32 mapCapacity)
    {
        //@TODO(Serge): do something with the memory boundary - for now we don't care, just malloc!
        
        mFiles = list;
        
#if ASSET_CACHE_DEBUG
        PrintAllAssetsInFiles();
#endif
        
        //Memory allocations for our lists and maps
        {
            mAssetLRU = MAKE_SQUEUE(mMallocArena, AssetHandle, lruCapacity);
            mAssetMap = MAKE_SHASHMAP(mMallocArena, AssetBufferRef, mapCapacity);
          //  mAssetFileMap = MAKE_SHASHMAP(mMallocArena, FileHandle, MAP_CAPACITY);
            mDefaultLoader = ALLOC(DefaultAssetLoader, mMallocArena);
            
            mAssetLoaders = MAKE_SARRAY(mMallocArena, AssetLoader*, lruCapacity);
        }
        
        
#ifdef R2_DEBUG
        //@TODO(Serge): add in watch paths for file watcher
#endif
        

        return true;
    }
    
    void AssetCache::Shutdown()
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return;
        }
        
#if ASSET_CACHE_DEBUG
        
        PrintLRU();
        PrintAssetMap();
        
#endif
        
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
            
            file->Close();
            
            FREE(file, mMallocArena);
        }
        
        FREE(mFiles, mMallocArena);
        FREE(mDefaultLoader, mMallocArena);
        FREE(mAssetLRU, mMallocArena);
        FREE(mAssetMap, mMallocArena);
        FREE(mAssetLoaders, mMallocArena);
        
        //FREE(mAssetFileMap, mMallocArena);
        
        mAssetLRU = nullptr;
        mAssetMap = nullptr;
        mAssetLoaders = nullptr;
        mFiles = nullptr;
        mDefaultLoader = nullptr;
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
            ++bufferRef.mRefCount;
            UpdateLRU(handle);
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
            Free(buffer->GetAsset().HashID(), true, false);
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
            FreeOneResource(false, true);
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
        
        if (loader == nullptr)
        {
            loader = mDefaultLoader;
        }
        
        R2_CHECK(loader != nullptr, "couldn't find asset loader");
        
        FileHandle fileIndex = FindInFiles(asset);
        if(fileIndex == INVALID_FILE_INDEX)
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
        u64 rawAssetSize = theAssetFile->RawAssetSize(asset);
        
        byte* rawAssetBuffer = ALLOC_BYTESN(mMallocArena, rawAssetSize, 4);
        
        R2_CHECK(rawAssetBuffer != nullptr, "failed to allocate asset handle: %lli of size: %llu", handle, rawAssetSize);
        
        theAssetFile->GetRawAsset(asset, rawAssetBuffer, (u32)rawAssetSize);
        
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
            byte* buffer = ALLOC_BYTESN(mMallocArena, size, 4);
            
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
        
#if ASSET_CACHE_DEBUG
        PrintAssetMap();
#endif
        
        return assetBuffer;
    }
    
    void AssetCache::UpdateLRU(AssetHandle handle)
    {
        s64 lruIndex = GetLRUIndex(handle);
        
        if (lruIndex == -1)
        {
            if (r2::squeue::Space(*mAssetLRU) - 1 <= 0)
            {
                FreeOneResource(false, true);
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
        
#if ASSET_CACHE_DEBUG
        PrintLRU();
#endif
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
    
    void AssetCache::Free(AssetHandle handle, bool decrementRefCount, bool forceFree)
    {
        AssetBufferRef theDefault;
        
        AssetBufferRef& assetBufferRef = Find(handle, theDefault);
        
        if (assetBufferRef.mAssetBuffer != nullptr)
        {
            if (decrementRefCount)
            {
                --assetBufferRef.mRefCount;
            }
            
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
    
    void AssetCache::FreeOneResource(bool decrementRefCount, bool forceFree)
    {
        const u64 size = r2::squeue::Size(*mAssetLRU);
        
        if (size > 0)
        {
            AssetHandle handle = r2::squeue::Last(*mAssetLRU);
            Free(handle, decrementRefCount, forceFree);
            
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
    
    RawAssetFile* AssetCache::MakeRawAssetFile(const char* path)
    {
        RawAssetFile* rawAssetFile = ALLOC(RawAssetFile, mMallocArena);
        
        bool result = rawAssetFile->Init(path);
        R2_CHECK(result, "Failed to initialize raw asset file");
        return rawAssetFile;
    }
    
    ZipAssetFile* AssetCache::MakeZipAssetFile(const char* path)
    {
        ZipAssetFile* zipAssetFile = ALLOC(ZipAssetFile, mMallocArena);
        
        bool result = zipAssetFile->Init(path,
        [this](u64 size, u64 alignment)
        {
            return ALLOC_BYTESN(mMallocArena, size, alignment);
        },
        [this](byte* ptr)
        {
            FREE(ptr, mMallocArena);
        });
        
        R2_CHECK(result, "Failed to initialize zip asset file");
        return zipAssetFile;
    }
    
    
    
#if ASSET_CACHE_DEBUG
    //Debug stuff
    void AssetCache::PrintLRU()
    {
        printf("==========================PrintLRU==========================\n");
        
        u64 size = r2::squeue::Size(*mAssetLRU);
        
        AssetBufferRef defaultRef;
        
        for (u64 i = 0; i < size; ++i)
        {
            AssetBufferRef bufRef = r2::shashmap::Get(*mAssetMap, (*mAssetLRU)[i], defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
                const Asset& asset = bufRef.mAssetBuffer->GetAsset();
                
                PrintAsset(asset.Name(), asset.HashID(), bufRef.mRefCount);
            }
        }
        
        printf("============================================================\n");
    }
    
    void AssetCache::PrintAssetMap()
    {
        printf("==========================PrintAssetMap==========================\n");

        
        u64 size = r2::squeue::Size(*mAssetLRU);
        
        AssetBufferRef defaultRef;
        
        for (u64 i = 0; i < size; ++i)
        {
            AssetBufferRef bufRef = r2::shashmap::Get(*mAssetMap, (*mAssetLRU)[i], defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
                const Asset& asset = bufRef.mAssetBuffer->GetAsset();
                
                PrintAsset(asset.Name(), asset.HashID(), bufRef.mRefCount);
            }
        }
        
//        u64 capacity = r2::sarr::Capacity( *mAssetMap->mHash);
//
//        for (u64 i = 0; i < capacity; ++i)
//        {
//            if((*mAssetMap->mHash)[i] != r2::hashmap_internal::END_OF_LIST)
//            {
//                u64 hash = (*mAssetMap->mHash)[i];
//
//                const AssetBufferRef& ref = (*mAssetMap->mData)[hash].value;
//
//                if (ref.mAssetBuffer != nullptr)
//                {
//                    PrintAsset(ref.mAssetBuffer->GetAsset().Name(), ref.mAssetBuffer->GetAsset().HashID(), ref.mRefCount);
//                }
//
//
//            }
//        }
        
        printf("=================================================================\n");
    }
    
    void AssetCache::PrintAllAssetsInFiles()
    {
        auto size = r2::sarr::Size(*mFiles);
        
        for (decltype(size) i = 0; i < size; ++i)
        {
            AssetFile* file = r2::sarr::At(*mFiles, i);
            if (!file->IsOpen())
            {
                bool result = file->Open();
                
                R2_CHECK(result, "Couldn't open the file!");
            }
            
            PrintAssetsInFile(file);
        }
    }
    
    void AssetCache::PrintAssetsInFile(AssetFile* file)
    {
        printf("======================PrintAssetsInFile=======================\n");
        
        auto numAssets = file->NumAssets();
        
        char nameBuf[r2::fs::FILE_PATH_LENGTH];
        
        for (decltype(numAssets) i = 0; i < numAssets; ++i)
        {
            file->GetAssetName(i, nameBuf, r2::fs::FILE_PATH_LENGTH);
            
            AssetHandle handle = file->GetAssetHandle(i);
            
            PrintAsset(nameBuf, handle, 0);
        }
        
        printf("==============================================================\n");
    }
    
    void AssetCache::PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount)
    {
        if (refcount == 0)
        {
            printf("Asset name: %s handle: %llu\n", asset, assetHandle);
        }
        else
        {
            printf("Asset name: %s handle: %llu #references: %u\n", asset, assetHandle, refcount);
        }
    }
#endif
}
