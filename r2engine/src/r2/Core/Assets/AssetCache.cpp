//
//  AssetCache.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-24.
//
#include "r2pch.h"
#include "AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetLoaders/AssetLoader.h"
#include "r2/Core/Assets/AssetWriters/AssetWriter.h"
#include "r2/Core/Assets/AssetLoaders/DefaultAssetLoader.h"
#include "assetlib/AssetFile.h"

#if defined(R2_DEBUG) || defined(R2_RELEASE)
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#endif

#ifdef R2_ASSET_PIPELINE
#include <filesystem>
#include "r2/Core/Assets/AssetLib.h"
#endif

#define NOT_INITIALIZED !mnoptrFiles || !mAssetLRU || !mAssetMap || !mAssetLoaders || !mAssetNameMap

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
        , mAssetWriters(nullptr)
        , mAssetNameMap(nullptr)
        , mAssetFreedCallbackList(nullptr)
        , mDefaultLoader(nullptr)
        , mSlot(slot)
        , mAssetCacheArena(boundary)
        , mAssetBufferPoolPtr(nullptr)
        , mMemoryHighWaterMark(0)
        , mMemoryBoundary(boundary)
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
            mAssetNameMap = MAKE_SHASHMAP(mAssetCacheArena, Asset, mapCapacity);
          //  mAssetFileMap = MAKE_SHASHMAP(mMallocArena, FileHandle, MAP_CAPACITY);
            mDefaultLoader = ALLOC(DefaultAssetLoader, mAssetCacheArena);
            
            mAssetLoaders = MAKE_SARRAY(mAssetCacheArena, AssetLoader*, lruCapacity);
            mAssetWriters = MAKE_SARRAY(mAssetCacheArena, AssetWriter*, lruCapacity);
            mAssetBufferPoolPtr = MAKE_POOL_ARENA(mAssetCacheArena, sizeof(AssetBuffer), lruCapacity);
            mAssetFreedCallbackList = MAKE_SARRAY(mAssetCacheArena, AssetFreedCallback, lruCapacity);
        }
        
#ifdef R2_ASSET_PIPELINE
        
     //   r2::asset::lib::AddFiles(*this, mnoptrFiles);
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
        
#if R2_ASSET_CACHE_DEBUG
        
        PrintHighWaterMark();
        PrintLRU();
        PrintAssetMap();
        
#endif
        
        FlushAll();

        s32 numWriters = r2::sarr::Size(*mAssetWriters);
        for (s32 i = numWriters - 1; i >= 0; --i)
        {
            AssetWriter* writer = r2::sarr::At(*mAssetWriters, i);
            FREE(writer, mAssetCacheArena);
        }
        
        s32 numLoaders = r2::sarr::Size(*mAssetLoaders);
        for (s32 i = numLoaders-1; i >=0; --i)
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
        FREE(mAssetNameMap, mAssetCacheArena);
        FREE(mAssetWriters, mAssetCacheArena);
        FREE(mAssetLoaders, mAssetCacheArena);
        FREE(mAssetBufferPoolPtr, mAssetCacheArena);
        FREE(mAssetFreedCallbackList, mAssetCacheArena);

        mAssetLRU = nullptr;
        mAssetMap = nullptr;
        mAssetNameMap = nullptr;
        mAssetLoaders = nullptr;
        mAssetWriters = nullptr;
        mnoptrFiles = nullptr;
        mDefaultLoader = nullptr;
        mAssetBufferPoolPtr = nullptr;
        mAssetFreedCallbackList = nullptr;
     //   mAssetFileMap = nullptr;
    }
    
    AssetHandle AssetCache::LoadAsset(const Asset& asset)
    {
        R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
        AssetHandle invalidHandle;

        if (NOT_INITIALIZED)
        {
            return invalidHandle;
        }
        
        AssetHandle handle = { asset.HashID(), mSlot };
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(handle, theDefault);
        
        if (bufferRef.mAssetBuffer == nullptr)
        {
            AssetBuffer* buffer = Load(asset);
            
            if (!buffer)
            {
#ifdef R2_ASSET_CACHE_DEBUG
                R2_CHECK(false, "Failed to Load Asset: %s", asset.Name());
#endif
                return invalidHandle;
            }
        }
        
        return handle;
    }

    bool AssetCache::HasAsset(const Asset& asset)
    {

        if (!mnoptrFiles)
            return false;

        u64 numFiles = r2::sarr::Size(*mnoptrFiles);

        for (u64 i = 0; i < numFiles; ++i)
        {
            auto file = r2::sarr::At(*mnoptrFiles, i);

            auto numAssets = file->NumAssets();
            for (u64 a = 0; a < numAssets; ++a)
            {
                if (file->GetAssetHandle(a) == asset.HashID())
                {
                    return true;
                }
            }
        }

        return false;


		//AssetHandle handle = { asset.HashID(), mSlot };
		//AssetBufferRef theDefault;
		//AssetBufferRef& bufferRef = Find(handle, theDefault);

  //      return bufferRef.mAssetBuffer != nullptr;
    }
    
    AssetHandle AssetCache::ReloadAsset(const Asset& asset)
    {
        AssetHandle handle = { asset.HashID(), mSlot };

		AssetBufferRef theDefault;
		AssetBufferRef& bufferRef = Find(handle, theDefault);

		bool found = bufferRef.mAssetBuffer != nullptr;

		if (found)
		{
			Free(handle, true);
		}

        return LoadAsset(asset);
    }

    void AssetCache::FreeAsset(const AssetHandle& handle)
    {
        Free(handle, false);
    }

    void AssetCache::WriteAssetFile(const Asset& asset, const void* data, u32 size, u32 offset, r2::fs::FileMode mode)
    {
		if (!mnoptrFiles)
			return;

		u64 numFiles = r2::sarr::Size(*mnoptrFiles);

        for (u64 i = 0; i < numFiles; ++i)
        {
            auto file = r2::sarr::At(*mnoptrFiles, i);

            auto numAssets = file->NumAssets();
            for (u64 a = 0; a < numAssets; ++a)
            {
                if (file->GetAssetHandle(a) == asset.HashID())
                {

                    Write(asset, data, size, offset, mode);

                    break;
                }
            }
        }
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
        
        AssetCacheRecord record;


		Asset defaultAsset;
		const Asset& asset = r2::shashmap::Get(*mAssetNameMap, handle.handle, defaultAsset);

        R2_CHECK(!asset.Empty() && defaultAsset.HashID() != asset.HashID(), "Failed to get the asset!");

        record.type = asset.GetType();

        if (bufferRef.mAssetBuffer != nullptr)
        {
            ++bufferRef.mRefCount;
            UpdateLRU(handle);

			record.handle = handle;
			record.buffer = bufferRef.mAssetBuffer;
        }
        else
        {
            
            AssetBuffer* buffer = Load(asset);
            
            if (!buffer)
            {
#ifdef R2_ASSET_CACHE_DEBUG
                R2_CHECK(false, "We couldn't reload the asset: %s", asset.Name()); 
#endif
            }

            record.handle = handle;
            record.buffer = buffer;
        }
        
        return record;
    }
    
    bool AssetCache::ReturnAssetBuffer(const AssetCacheRecord& buffer)
    {
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(buffer.handle, theDefault);
        
        bool found = bufferRef.mAssetBuffer != nullptr;
        
        if (found)
        {
            Free(buffer.handle, false);
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

    void AssetCache::RegisterAssetWriter(AssetWriter* optrAssetWriter)
    {
		R2_CHECK(!NOT_INITIALIZED, "We haven't initialized the asset cache");
		if (NOT_INITIALIZED)
		{
			return;
		}

        r2::sarr::Push(*mAssetWriters, optrAssetWriter);
    }

    //Private
    AssetBuffer* AssetCache::Load(const Asset& asset, bool startCountAtOne)
    {

        if (!mnoptrFiles || r2::sarr::Size(*mnoptrFiles) == 0)
        {
            return nullptr;
        }

        u64 numAssetLoaders = r2::sarr::Size(*mAssetLoaders);
        AssetLoader* loader = nullptr;
		for (u64 i = 0; i < numAssetLoaders; ++i)
		{
			AssetLoader* nextLoader = r2::sarr::At(*mAssetLoaders, i);
			if (nextLoader->GetType() == asset.GetType())
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
        
        
        AssetHandle handle = { asset.HashID(), mSlot};
        u64 rawAssetSize = theAssetFile->RawAssetSize(asset);
        
        bool result = MakeRoom(rawAssetSize);
        
        R2_CHECK(result, "We don't have enough room to fit %s!\n", theAssetFile->FilePath());
        
        byte* rawAssetBuffer = ALLOC_BYTESN(mAssetCacheArena, rawAssetSize, alignof(size_t));
        
        R2_CHECK(rawAssetBuffer != nullptr, "failed to allocate asset handle: %lli of size: %llu", handle, rawAssetSize);
        
        mMemoryHighWaterMark = std::max(mMemoryHighWaterMark, mAssetCacheArena.TotalBytesAllocated());

        theAssetFile->LoadRawAsset(asset, rawAssetBuffer, (u32)rawAssetSize);
        
        if (rawAssetBuffer == nullptr)
        {
#ifdef R2_ASSET_CACHE_DEBUG
            R2_CHECK(false, "Failed to get the raw data from the asset: %s\n", asset.Name());
#endif
            return nullptr;
        }

        AssetBuffer* assetBuffer = nullptr;
        
        assetBuffer = ALLOC(AssetBuffer, *mAssetBufferPoolPtr);
#ifdef R2_ASSET_CACHE_DEBUG
        R2_CHECK(assetBuffer != nullptr, "Failed to allocate a new asset buffer for asset: %s\n", asset.Name());
#endif
        if (assetBuffer == nullptr)
        {
            theAssetFile->Close();
            return nullptr;
        }
        
        if (!loader->ShouldProcess())
        {
            assetBuffer->Load(asset, mSlot, rawAssetBuffer, rawAssetSize);
        }
        else
        {
			u32 headerSize = 0;
			u32 boundsChecking = 0;
#ifdef R2_ASSET_PIPELINE
			headerSize = r2::mem::MallocAllocator::HeaderSize();
#else
			headerSize = r2::mem::FreeListAllocator::HeaderSize();
#endif
#ifdef R2_DEBUG
			boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

            const u64 ALIGNMENT = 16;

            u64 size = loader->GetLoadedAssetSize(theAssetFile->FilePath(), rawAssetBuffer, rawAssetSize, ALIGNMENT, headerSize, boundsChecking);
            bool success = false;

			bool madeRoom = MakeRoom(size);

			R2_CHECK(madeRoom, "We don't have enough room to fit %s!\n", theAssetFile->FilePath());

			byte* buffer = ALLOC_BYTESN(mAssetCacheArena, size, alignof(size_t));

			R2_CHECK(buffer != nullptr, "Failed to allocate buffer!");

			if (!buffer)
			{
				theAssetFile->Close();
				return nullptr;
			}

			mMemoryHighWaterMark = std::max(mMemoryHighWaterMark, mAssetCacheArena.TotalBytesAllocated());

			assetBuffer->Load(asset, mSlot, buffer, size);

			success = loader->LoadAsset(theAssetFile->FilePath(), rawAssetBuffer, rawAssetSize, *assetBuffer);
				
            if (!success)
			{
				theAssetFile->Close();
				return nullptr;
			}

			R2_CHECK(success, "Failed to load asset");

			FREE(rawAssetBuffer, mAssetCacheArena);
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

        r2::shashmap::Set(*mAssetMap, handle.handle, bufferRef);
        r2::shashmap::Set(*mAssetNameMap, handle.handle, asset);
        UpdateLRU(handle);
        
#ifdef R2_ASSET_PIPELINE
		AssetRecord record;
		record.handle = handle;
        record.type = asset.GetType();
#ifdef R2_ASSET_CACHE_DEBUG
		record.name = asset.Name();
#endif
        AddAssetToAssetsForFileList(fileIndex, record);
#endif
        
#if ASSET_CACHE_DEBUG
        PrintAssetMap();
#endif
        theAssetFile->Close();

        return assetBuffer;
    }

    void AssetCache::Write(const Asset& asset, const void* data, u32 size, u32 offset, r2::fs::FileMode mode)
    {
        if (!mnoptrFiles)
        {
            return;
        }

		u32 numAssetWriters = r2::sarr::Size(*mAssetWriters);
		AssetWriter* writer = nullptr;
		for (u64 i = 0; i < numAssetWriters; ++i)
		{
			AssetWriter* nextWriter = r2::sarr::At(*mAssetWriters, i);
			if (nextWriter->GetType() == asset.GetType())
			{
				writer = nextWriter;
				break;
			}
		}

		R2_CHECK(writer != nullptr, "Couldn't find an asset Writer for asset: %s\n", asset.Name());
        if (!writer)
        {
            return;
        }

		FileHandle fileIndex = FindInFiles(asset);
		if (fileIndex == INVALID_FILE_INDEX)
		{
			R2_CHECK(false, "We failed to find the asset in any of our asset files");
			return;
		}

		AssetFile* theAssetFile = r2::sarr::At(*mnoptrFiles, fileIndex);
		R2_CHECK(theAssetFile != nullptr, "We have a null asset file?");

        if (theAssetFile->IsOpen())
        {
            theAssetFile->Close();
        }

        R2_CHECK(mode.IsSet(fs::Mode::Write), "Write should be enabled for the file: %s\n", theAssetFile->FilePath());

        bool isOpened = theAssetFile->Open(mode);
        
        if (!isOpened)
        {
            R2_CHECK(false, "We couldn't open the file: %s\n", theAssetFile->FilePath());
            return;
        }

        bool writeSuccess = writer->WriteAsset(*theAssetFile, asset, data, size, offset);

        R2_CHECK(writeSuccess, "Failed to write to file path: %s\n", theAssetFile->FilePath());
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
        
#if R2_ASSET_CACHE_DEBUG
   //     PrintLRU();
#endif
    }
    
    FileHandle AssetCache::FindInFiles(const Asset& asset)
    {
        if (!mnoptrFiles)
        {
            return INVALID_FILE_INDEX;
        }

        const AssetHandle handle = { asset.HashID(), mSlot };
        
        const u64 numnoptrFiles = r2::sarr::Size(*mnoptrFiles);
        
        for (u64 i = 0; i < numnoptrFiles; ++i)
        {
            AssetFile* file = r2::sarr::At(*mnoptrFiles, i);
            
            //if (!file->IsOpen())
            //{
            //    file->Open();
            //}

            const u64 numAssets = file->NumAssets();
            
            for (u64 j = 0; j < numAssets; ++j)
            {
                if (file->GetAssetHandle(j) == handle.handle)
                {
                    return i;
                }
            }
        }
        
        return INVALID_FILE_INDEX;
    }
    
    AssetCache::AssetBufferRef& AssetCache::Find(AssetHandle handle, AssetCache::AssetBufferRef& theDefault)
    {
        return r2::shashmap::Get(*mAssetMap, handle.handle, theDefault);
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
                Asset theDefault;
                const Asset& theAsset = r2::shashmap::Get(*mAssetNameMap, handle.handle, theDefault);
#ifdef R2_ASSET_CACHE_DEBUG
                R2_CHECK(false, "AssetCache::Free() - we're trying to free the asset: %s but we still have %u references to it!", theAsset.Name(),  assetBufferRef.mRefCount);
#endif
            }
            
            if (assetBufferRef.mRefCount < 0)
            {
                FREE(assetBufferRef.mAssetBuffer->MutableData(), mAssetCacheArena);
                
                FREE(assetBufferRef.mAssetBuffer, *mAssetBufferPoolPtr);
            
                r2::shashmap::Remove(*mAssetMap, handle.handle);
                
                RemoveFromLRU(handle);
#ifdef R2_ASSET_PIPELINE
                RemoveAssetFromAssetForFileList(handle);
#endif

                const auto numFreedCallbacks = r2::sarr::Size(*mAssetFreedCallbackList);

                for (u32 i = 0; i < numFreedCallbacks; ++i)
                {
                    auto callback = r2::sarr::At(*mAssetFreedCallbackList, i);
                    callback(handle);
                }
            }
        }
    }
    
    bool AssetCache::MakeRoom(u64 amount)
    {
        const auto cacheMemorySize = mAssetCacheArena.MemorySize();
        if(amount > cacheMemorySize)
        {
            R2_CHECK(false, "Can't even fit %llu into the total amount we have: %llu", amount, mAssetCacheArena.MemorySize());
            return false;
        }
        
        while (amount > (cacheMemorySize - mAssetCacheArena.TotalBytesAllocated()))
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
            
          //  RemoveFromLRU(handle);
        }
    }
    
    s64 AssetCache::GetLRUIndex(AssetHandle handle)
    {
        u64 queueSize = r2::squeue::Size(*mAssetLRU);
        
        for (u64 i = 0; i < queueSize; ++i)
        {
            if ((*mAssetLRU)[i].handle == handle.handle)
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
    
    u64 AssetCache::MemoryHighWaterMark()
    {
        return mMemoryHighWaterMark;
    }

    u64 AssetCache::TotalMemoryNeeded(u32 headerSize, u32 boundsChecking, u64 numAssets, u64 assetCapacity, u64 alignment, u32 lruCapacity, u32 mapCapacity)
    {
        u64 elementSize = sizeof(AssetBuffer);
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        elementSize = elementSize + r2::mem::BasicBoundsChecking::SIZE_BACK + r2::mem::BasicBoundsChecking::SIZE_FRONT;
#endif
        u64 poolSizeInBytes = lruCapacity * elementSize;
        
        /*alignment = std::max({
            alignment,
            alignof(r2::SQueue<AssetHandle>),
            alignof(AssetHandle),
            alignof(DefaultAssetLoader),
            alignof(r2::SArray<AssetLoader*>),
            alignof(r2::SArray<AssetWriter*>),
            alignof(AssetLoader*),
            alignof(Asset),
            alignof(r2::SHashMap<Asset>),
            alignof(r2::mem::PoolArena),
            alignof(r2::SArray<AssetFreedCallback>),
            });*/

        return 
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<AssetHandle>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking)  +
            r2::mem::utils::GetMaxMemoryForAllocation(SHashMap<AssetBufferRef>::MemorySize(mapCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(DefaultAssetLoader), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AssetLoader*>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AssetWriter*>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Asset>::MemorySize(mapCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AssetFreedCallback>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(poolSizeInBytes, alignment, headerSize, boundsChecking) +
            CalculateCacheSizeNeeded(assetCapacity, numAssets, alignment);
    }

    u64 AssetCache::CalculateCacheSizeNeeded(u64 initialAssetCapcity, u64 numAssets, u64 alignment)
    {
        u32 headerSize = 0;
        u32 boundsChecking = 0;
#ifdef R2_ASSET_PIPELINE
        headerSize = r2::mem::MallocAllocator::HeaderSize();
#else
        headerSize = r2::mem::FreeListAllocator::HeaderSize();
#endif
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

        return initialAssetCapcity + (numAssets * (static_cast<u64>(headerSize) + static_cast<u64>(boundsChecking) + alignment));
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
                    if (assetRecord.handle.handle == handle.handle.handle)
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
                return record.handle.handle == assetHandle.handle;
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
        
        R2_CHECK(fileHandle >= 0 && fileHandle < static_cast<s64>(r2::sarr::Size(*mnoptrFiles)), "File handle: %llu is not within the number of files we have", fileHandle);
        
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
                AssetBuffer* buffer = Load(Asset(assetRecord.name.c_str(), assetRecord.type));
                
                R2_CHECK(buffer != nullptr, "buffer could not be loaded for asset: %s", assetRecord.name.c_str());
            }
        }
        
    }
    
    void AssetCache::AddReloadFunction(AssetReloadedFunc func)
    {
        mReloadFunctions.push_back(func);
    }

    void AssetCache::ResetFileList(FileList fileList)
    {
        asset::lib::ResetFilesForCache(*this, fileList);
        mnoptrFiles = fileList;
    }
#endif

    void AssetCache::RegisterAssetFreedCallback(AssetFreedCallback func)
    {
        r2::sarr::Push(*mAssetFreedCallbackList, func);
    }
    
#if R2_ASSET_CACHE_DEBUG
    //Debug stuff
    void AssetCache::PrintLRU()
    {
        printf("==========================PrintLRU==========================\n");
        printf("Asset Cache: %llu\n", mSlot);
        u64 size = r2::squeue::Size(*mAssetLRU);
        
        AssetBufferRef defaultRef;
        
        for (u64 i = 0; i < size; ++i)
        {
            AssetBufferRef bufRef = r2::shashmap::Get(*mAssetMap, (*mAssetLRU)[i].handle, defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
                Asset theDefault;
                const Asset& asset = r2::shashmap::Get(*mAssetNameMap, bufRef.mAssetBuffer->GetHandle().handle, theDefault);
                

                AssetHandle handle = { asset.HashID(), mSlot };
                PrintAsset(asset.Name(), handle, bufRef.mRefCount);
            }
        }
        
        printf("============================================================\n");
    }
    
    void AssetCache::PrintAssetMap()
    {
        printf("==========================PrintAssetMap==========================\n");
        printf("Asset Cache: %llu\n", mSlot);
        
        u64 size = r2::squeue::Size(*mAssetLRU);
        
        AssetBufferRef defaultRef;
        
        for (u64 i = 0; i < size; ++i)
        {
            AssetBufferRef bufRef = r2::shashmap::Get(*mAssetMap, (*mAssetLRU)[i].handle, defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
				Asset theDefault;
				const Asset& asset = r2::shashmap::Get(*mAssetNameMap, bufRef.mAssetBuffer->GetHandle().handle, theDefault);

                AssetHandle handle = { asset.HashID(), mSlot };
                PrintAsset(asset.Name(), handle, bufRef.mRefCount);
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

    void AssetCache::PrintHighWaterMark()
    {
        const auto totalMemorySize = mMemoryBoundary.size;

		printf("===============================================PrintHighWaterMark====================================================\n");
		printf("Asset Cache: %llu - memory high water mark: %llu bytes, %f Kilobytes, %f Megabytes\n", mSlot, mMemoryHighWaterMark, BytesToKilobytes(mMemoryHighWaterMark), BytesToMegabytes(mMemoryHighWaterMark));
        printf("Asset Cache: %llu - utilization: %llu / %llu, %f percent\n", mSlot, mMemoryHighWaterMark, totalMemorySize, (double(mMemoryHighWaterMark) / double(totalMemorySize)) * 100.0);
		printf("=====================================================================================================================\n");
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
            
            AssetHandle handle = { file->GetAssetHandle(i), mSlot };
            
            PrintAsset(nameBuf, handle, 0);
        }
        
        printf("==============================================================\n");
    }
    
    void AssetCache::PrintAsset(const char* asset, AssetHandle assetHandle, u32 refcount)
    {
        if (refcount == 0)
        {
            printf("Asset name: %s handle: %llu\n", asset, assetHandle.handle);
        }
        else
        {
            printf("Asset name: %s handle: %llu #references: %u\n", asset, assetHandle.handle, refcount);
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
