//
//  AssetCache.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//
#include "r2pch.h"
#include "AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetLoader.h"
#include "r2/Core/Assets/DefaultAssetLoader.h"
#if defined(R2_DEBUG) || defined(R2_RELEASE)
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#endif

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#include "r2/Core/Assets/AssetLib.h"
#endif

#define NOT_INITIALIZED !mnoptrFiles || !mAssetLRU || !mAssetMap || !mAssetLoaders

namespace
{
    const s64 INVALID_FILE_INDEX = -1;
}

namespace r2::asset
{
    
    const u32 AssetCache::LRU_CAPACITY;
    const u32 AssetCache::MAP_CAPACITY;
    
    AssetCache::AssetCache(u64 slot, const r2::mem::utils::MemBoundary& boundary)
        : mnoptrFiles(nullptr)
        , mAssetLRU(nullptr)
        , mAssetMap(nullptr)
        , mAssetLoaders(nullptr)
        , mSlot(slot)
    //    , mAssetFileMap(nullptr)
        , mAssetCacheArena(boundary)
        , mAssetBufferPoolPtr(nullptr)
    {
        
    }
    
    bool AssetCache::Init(FileList noptrList, u32 lruCapacity, u32 mapCapacity)
    {
        mnoptrFiles = noptrList;

#if ASSET_CACHE_DEBUG
        PrintAllAssetsInFiles();
#endif
        
        //Memory allocations for our lists and maps
        {
            mAssetLRU = MAKE_SQUEUE(mAssetCacheArena, AssetHandle, lruCapacity);
            mAssetMap = MAKE_SHASHMAP(mAssetCacheArena, AssetBufferRef, mapCapacity);
          //  mAssetFileMap = MAKE_SHASHMAP(mMallocArena, FileHandle, MAP_CAPACITY);
            mDefaultLoader = ALLOC(DefaultAssetLoader, mAssetCacheArena);
            
            mAssetLoaders = MAKE_SARRAY(mAssetCacheArena, AssetLoader*, lruCapacity);
            mAssetBufferPoolPtr = MAKE_POOL_ARENA(mAssetCacheArena, sizeof(AssetBuffer), lruCapacity);
        }
        
#ifdef R2_ASSET_PIPELINE
        
        r2::asset::lib::AddFiles(*this, mnoptrFiles);
        //@TODO(Serge): may need an update function that does the following instead
        //The below will just add paths to a vector or something
        r2::asset::lib::AddAssetFilesBuiltListener([this](std::vector<std::string> paths)
        {
            //Reload the asset if needed
            for (const std::string& path : paths)
            {
                u64 numnoptrFiles = r2::sarr::Size(*this->mnoptrFiles);

                for (u64 i = 0; i < numnoptrFiles; ++i)
                {
                    AssetFile* file = r2::sarr::At(*this->mnoptrFiles, i);

                    if (std::filesystem::path(file->FilePath()) == std::filesystem::path(path))
                    {

                        this->InvalidateAssetsForFile(i);
                    }
                }
            }
        });
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
            FREE(loader, mAssetCacheArena);
        }
        
        u64 numFiles = r2::sarr::Size(*mnoptrFiles);
        for (u64 i = 0; i < numFiles; ++i)
        {
            AssetFile* file = r2::sarr::At(*mnoptrFiles, i);
            if (file->IsOpen())
            {
                file->Close();
            }
        }
        
        FREE(mDefaultLoader, mAssetCacheArena);
        FREE(mAssetLRU, mAssetCacheArena);
        FREE(mAssetMap, mAssetCacheArena);
        FREE(mAssetLoaders, mAssetCacheArena);
        FREE(mAssetBufferPoolPtr, mAssetCacheArena);
        
        mAssetLRU = nullptr;
        mAssetMap = nullptr;
        mAssetLoaders = nullptr;
        mnoptrFiles = nullptr;
        mDefaultLoader = nullptr;
        mAssetBufferPoolPtr = nullptr;
     //   mAssetFileMap = nullptr;
    }
    
    AssetHandle AssetCache::LoadAsset(const Asset& asset)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            return INVALID_ASSET_HANDLE;
        }
        
        AssetHandle handle = asset.HashID();
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(handle, theDefault);
        
        if (bufferRef.mAssetBuffer == nullptr)
        {
            AssetBuffer* buffer = Load(asset);
            
            if (!buffer)
            {
                R2_CHECK(false, "Failed to Load Asset: %s", asset.Name());
                return INVALID_ASSET_HANDLE;
            }
        }
        
        return handle;
    }
    
    AssetCacheRecord AssetCache::GetAssetBuffer(AssetHandle handle)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        if (NOT_INITIALIZED)
        {
            AssetCacheRecord record;
            return record;
        }
        
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(handle, theDefault);
        
        if (bufferRef.mAssetBuffer != nullptr)
        {
            ++bufferRef.mRefCount;
            UpdateLRU(handle);
        }
        
        AssetCacheRecord record;
        record.handle = handle;
        record.buffer = bufferRef.mAssetBuffer;
        
        return record;
    }
    
    bool AssetCache::ReturnAssetBuffer(const AssetCacheRecord& buffer)
    {
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(buffer.buffer->GetAsset().HashID(), theDefault);
        
        bool found = bufferRef.mAssetBuffer != nullptr;
        
        if (found)
        {
            Free(buffer.buffer->GetAsset().HashID(), false);
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
    AssetBuffer* AssetCache::Load(const Asset& asset, bool startCountAtOne)
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
        
        AssetFile* theAssetFile = r2::sarr::At(*mnoptrFiles, fileIndex);
        R2_CHECK(theAssetFile != nullptr, "We have a null asset file?");
        
        if (!theAssetFile->IsOpen())
        {
            theAssetFile->Open();
        }
        
        
        AssetHandle handle = asset.HashID();
        u64 rawAssetSize = theAssetFile->RawAssetSize(asset);
        
        bool result = MakeRoom(rawAssetSize);
        
        R2_CHECK(result, "We don't have enough room to fit this asset!");
        
        byte* rawAssetBuffer = ALLOC_BYTESN(mAssetCacheArena, rawAssetSize, alignof(size_t));
        
        R2_CHECK(rawAssetBuffer != nullptr, "failed to allocate asset handle: %lli of size: %llu", handle, rawAssetSize);
        
        theAssetFile->GetRawAsset(asset, rawAssetBuffer, (u32)rawAssetSize);
        
        if (rawAssetBuffer == nullptr)
        {
            R2_CHECK(false, "Failed to get the raw data from the asset: %s\n", asset.Name());
            return nullptr;
        }

        AssetBuffer* assetBuffer = nullptr;
        
        assetBuffer = ALLOC(AssetBuffer, *mAssetBufferPoolPtr);
        
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
            byte* buffer = ALLOC_BYTESN(mAssetCacheArena, size, alignof(size_t));
            
            R2_CHECK(buffer != nullptr, "Failed to allocate buffer!");
            
            if (!buffer)
            {
                return nullptr;
            }
            
            assetBuffer->Load(asset, buffer, size);
            
            bool success = loader->LoadAsset(rawAssetBuffer, rawAssetSize, *assetBuffer);
            
            R2_CHECK(success, "Failed to load asset");
            
            FREE(rawAssetBuffer, mAssetCacheArena);
            
            if (!success)
            {
                return nullptr;
            }
        }
        AssetBufferRef bufferRef;
        
        if(startCountAtOne)
        {
            bufferRef.mRefCount = 1;
        }
        else
        {
            bufferRef.mRefCount = 0;
        }
        
        bufferRef.mAssetBuffer = assetBuffer;

        r2::shashmap::Set(*mAssetMap, handle, bufferRef);
        UpdateLRU(handle);
        
        AssetRecord record;
        record.handle = handle;
        record.name = asset.Name();
        
#ifdef R2_ASSET_PIPELINE
        AddAssetToAssetsForFileList(fileIndex, record);
#endif
        
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
            if (r2::squeue::Space(*mAssetLRU) <= 0)
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
        
#if ASSET_CACHE_DEBUG
        PrintLRU();
#endif
    }
    
    FileHandle AssetCache::FindInFiles(const Asset& asset)
    {
        const AssetHandle handle = asset.HashID();
        
        const u64 numnoptrFiles = r2::sarr::Size(*mnoptrFiles);
        
        for (u64 i = 0; i < numnoptrFiles; ++i)
        {
            const AssetFile* file = r2::sarr::At(*mnoptrFiles, i);
            
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
    
    void AssetCache::Free(AssetHandle handle, bool forceFree)
    {
        AssetBufferRef theDefault;
        
        AssetBufferRef& assetBufferRef = Find(handle, theDefault);
        
        if (assetBufferRef.mAssetBuffer != nullptr)
        {
            --assetBufferRef.mRefCount;
            
            if (forceFree && assetBufferRef.mRefCount > 0)
            {
                R2_CHECK(false, "AssetCache::Free() - we're trying to free the asset: %s but we still have %u references to it!", assetBufferRef.mAssetBuffer->GetAsset().Name(),  assetBufferRef.mRefCount);
            }
            
            if (assetBufferRef.mRefCount < 0)
            {
                FREE(assetBufferRef.mAssetBuffer->MutableData(), mAssetCacheArena);
                
                FREE(assetBufferRef.mAssetBuffer, *mAssetBufferPoolPtr);
            
                r2::shashmap::Remove(*mAssetMap, handle);
                
                RemoveFromLRU(handle);
#ifdef R2_ASSET_PIPELINE
                RemoveAssetFromAssetForFileList(handle);
#endif
            }
        }
    }
    
    bool AssetCache::MakeRoom(u64 amount)
    {
        if(amount > mAssetCacheArena.MemorySize())
        {
            R2_CHECK(false, "Can't even fit %llu into the total amount we have: %llu", amount, mAssetCacheArena.MemorySize());
            return false;
        }
        
        while (amount > (mAssetCacheArena.MemorySize() - mAssetCacheArena.TotalBytesAllocated()))
        {
            if (r2::squeue::Size(*mAssetLRU) == 0)
            {
                R2_CHECK(false, "We still don't have enough room to fit: %llu", amount);
                return false;
            }
            
            FreeOneResource(true);
        }
        
        return true;
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
    
    u64 AssetCache::TotalMemoryNeeded(u64 assetCapacity, u32 lruCapacity, u32 mapCapacity)
    {
        u64 elementSize = sizeof(AssetBuffer);
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        elementSize = elementSize + r2::mem::BasicBoundsChecking::SIZE_BACK + r2::mem::BasicBoundsChecking::SIZE_FRONT;
#endif
        u64 poolSizeInBytes = LRU_CAPACITY * elementSize;
        
        return r2::SQueue<AssetHandle>::MemorySize(lruCapacity) + SHashMap<AssetBufferRef>::MemorySize(mapCapacity) + sizeof(DefaultAssetLoader) + r2::SArray<AssetLoader*>::MemorySize(lruCapacity) + sizeof(r2::mem::PoolArena) + poolSizeInBytes + assetCapacity;
    }
    
#ifdef R2_ASSET_PIPELINE
    void AssetCache::AddAssetToAssetsForFileList(FileHandle fileHandle, const AssetRecord& assetRecord)
    {
        bool found = false;
        
        for (u64 i = 0; i < mAssetsForFiles.size(); ++i)
        {
            if (mAssetsForFiles[i].file == fileHandle)
            {
                found = true;
                
                bool foundAssetHandle = false;
                for (const auto& handle : mAssetsForFiles[i].assets)
                {
                    if (assetRecord.handle == handle.handle)
                    {
                        foundAssetHandle = true;
                        break;
                    }
                }
                
                if (!foundAssetHandle)
                {
                    mAssetsForFiles[i].assets.push_back(assetRecord);
                }
                
                break;
            }
        }
        
        if (!found)
        {
            //add new entry
            AssetsToFile newEntry;
            newEntry.file = fileHandle;
            newEntry.assets.push_back(assetRecord);
            mAssetsForFiles.push_back(newEntry);
        }
    }
    
    void AssetCache::RemoveAssetFromAssetForFileList(AssetHandle assetHandle)
    {
        for (size_t i = 0; i < mAssetsForFiles.size(); ++i)
        {
            auto iter = std::remove_if(mAssetsForFiles[i].assets.begin(), mAssetsForFiles[i].assets.end(), [assetHandle](const AssetRecord& record){
                return record.handle == assetHandle;
            });
            
            if (iter != mAssetsForFiles[i].assets.end())
            {
                mAssetsForFiles[i].assets.erase(iter);
                return;
            }
        }
    }
    
    void AssetCache::InvalidateAssetsForFile(FileHandle fileHandle)
    {
        //reload all of the assets for that file and ensure that the asset isn't currently in use - assert if that's the case
        //We may need to keep a record of all the assets associated with the file, we could then just invalidate (free) them
        //Then the next time we request that asset, we reload automatically - alternatively, we can have callbacks here
        //We should also close the files
        
        R2_CHECK(fileHandle >= 0 && fileHandle < r2::sarr::Size(*mnoptrFiles), "File handle: %llu is not within the number of files we have", fileHandle);
        
        AssetFile* file = r2::sarr::At(*mnoptrFiles, fileHandle);
    
        file->Close();
        
        std::vector<AssetRecord> assetsToReload;
        
        for (size_t i = 0; i < mAssetsForFiles.size(); ++i)
        {
            if (mAssetsForFiles[i].file == fileHandle)
            {
                assetsToReload = mAssetsForFiles[i].assets;
                //unload these assets
                for (auto assetHandle : assetsToReload)
                {
                    AssetBufferRef defaultBuffer;
                    AssetBufferRef& bufferRef = Find(assetHandle.handle, defaultBuffer);
                    
                    for (auto func : mReloadFunctions)
                    {
                        func(assetHandle.handle);
                    }
                    
                    if (bufferRef.mAssetBuffer != nullptr)
                    {
                        Free(assetHandle.handle, true);
                    }
                }

                break;
            }
        }
        
        bool opened = file->Open();
        
        R2_CHECK(opened, "Could not re-open the file: %s\n", file->FilePath());
        
        if (assetsToReload.size() > 0)
        {
            for (const auto& assetRecord : assetsToReload)
            {
                AssetBuffer* buffer = Load(Asset(assetRecord.name.c_str()));
                
                R2_CHECK(buffer != nullptr, "buffer could not be loaded for asset: %s", assetRecord.name.c_str());
            }
        }
        
    }
    
    void AssetCache::AddReloadFunction(AssetReloadedFunc func)
    {
        mReloadFunctions.push_back(func);
    }
#endif
    
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
        auto size = r2::sarr::Size(*mnoptrFiles);
        
        for (decltype(size) i = 0; i < size; ++i)
        {
            AssetFile* file = r2::sarr::At(*mnoptrFiles, i);
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
    
//#ifdef R2_DEBUG
//    std::string AssetCache::GetParentDirectory() const
//    {
//        u64 numnoptrFiles = r2::sarr::Size(*mnoptrFiles);
//
//        R2_CHECK(numnoptrFiles > 0, "We don't have any files to compare!");
//        std::filesystem::path parentPath;
//
//        if (numnoptrFiles > 0)
//        {
//            std::vector<std::filesystem::path::iterator> filePathIters;
//
//            for (u64 i = 0; i < numnoptrFiles; ++i)
//            {
//                const AssetFile* assetFile = r2::sarr::At(*mnoptrFiles, i);
//                filePathIters.push_back(std::filesystem::path(assetFile->FilePath()).begin());
//            }
//
//            if (filePathIters.size() == 1)
//            {
//                const AssetFile* assetFile = r2::sarr::At(*mnoptrFiles, 0);
//                parentPath = std::filesystem::path(assetFile->FilePath()).parent_path();
//            }
//            else
//            {
//
//                while (true)
//                {
//                    std::filesystem::path tempPath = std::filesystem::path(*filePathIters[0]);
//                    filePathIters[0]++;
//                    bool failed = false;
//                    for (u32 i = 1; i < filePathIters.size(); ++i)
//                    {
//                        if (tempPath != std::filesystem::path(*filePathIters[i]))
//                        {
//                            failed = true;
//                            break;
//                        }
//
//                        filePathIters[i]++;
//                    }
//
//                    if (!failed)
//                    {
//                        parentPath/=tempPath;
//                    }
//                    else
//                    {
//                        break;
//                    }
//                }
//            }
//        }
//
//        R2_CHECK(parentPath.string().size()> 1, "We have no parent directory?");
//
//        return parentPath.string();
//    }
//#endif
}
