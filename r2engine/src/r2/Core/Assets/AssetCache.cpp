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
#include "r2/Core/Assets/AssetLib.h"

#if defined(R2_DEBUG) || defined(R2_RELEASE)
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#endif

#define NOT_INITIALIZED !mAssetLRU || !mAssetMap || !mAssetLoaders

namespace r2::asset
{
    
    const u32 AssetCache::LRU_CAPACITY;
    const u32 AssetCache::MAP_CAPACITY;
    
    AssetCache::AssetCache(u64 slot, const r2::mem::utils::MemBoundary& boundary)
        : mAssetLRU(nullptr)
        , mAssetMap(nullptr)
        , mAssetLoaders(nullptr)
        , mDefaultLoader(nullptr)
        , mSlot(slot)
        , mAssetCacheArena(boundary)
        , mAssetBufferPoolPtr(nullptr)
        , mMemoryHighWaterMark(0)
        , mMemoryBoundary(boundary)
    {
        
    }
    
    bool AssetCache::Init( u32 lruCapacity, u32 mapCapacity)
    {

        //Memory allocations for our lists and maps
        {
            mAssetLRU = MAKE_SQUEUE(mAssetCacheArena, AssetHandle, lruCapacity);
            mAssetMap = MAKE_SARRAY(mAssetCacheArena, AssetBufferRef, mapCapacity);
            mDefaultLoader = ALLOC(DefaultAssetLoader, mAssetCacheArena);
            mAssetLoaders = MAKE_SARRAY(mAssetCacheArena, AssetLoader*, lruCapacity);
            mAssetBufferPoolPtr = MAKE_POOL_ARENA(mAssetCacheArena, sizeof(AssetBuffer), alignof(AssetBuffer), lruCapacity);
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
        
#if R2_ASSET_CACHE_DEBUG
        
        PrintHighWaterMark();
        PrintLRU();
        PrintAssetMap();
        
#endif
        FlushAll();

        s32 numLoaders = static_cast<s32>(r2::sarr::Size(*mAssetLoaders) );
        for (s32 i = numLoaders-1; i >=0; --i)
        {
            AssetLoader* loader = r2::sarr::At(*mAssetLoaders, i);
            FREE(loader, mAssetCacheArena);
        }
        
        FREE(mDefaultLoader, mAssetCacheArena);
        FREE(mAssetLRU, mAssetCacheArena);
        FREE(mAssetMap, mAssetCacheArena);
        FREE(mAssetLoaders, mAssetCacheArena);
        FREE(mAssetBufferPoolPtr, mAssetCacheArena);


        mAssetLRU = nullptr;
        mAssetMap = nullptr;
        mAssetLoaders = nullptr;
        mDefaultLoader = nullptr;
        mAssetBufferPoolPtr = nullptr;

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
        AssetBufferRef& bufferRef = Find(asset.HashID(), theDefault);
        
        if (bufferRef.mAssetBuffer == nullptr)
        {
            AssetBuffer* buffer = Load(asset);
            
            if (!buffer)
            {
#ifdef R2_ASSET_CACHE_DEBUG
                R2_CHECK(false, "Failed to Load Asset: %s", asset.Name().c_str());
#endif
                return invalidHandle;
            }
        }
        
        return handle;
    }

    bool AssetCache::IsAssetLoaded(const Asset& asset)
    {
        return IsLoaded(asset);
    }
    
    AssetHandle AssetCache::ReloadAsset(const Asset& asset)
    {
        AssetHandle handle = { asset.HashID(), mSlot };

		AssetBufferRef theDefault;
		AssetBufferRef& bufferRef = Find(handle.handle, theDefault);

		bool found = bufferRef.mAssetBuffer != nullptr;

		if (found)
		{
			Free(handle, true);
		}
        
        auto assetHandle = LoadAsset(asset);

        return assetHandle;
    }

    void AssetCache::FreeAsset(const AssetHandle& handle)
    {
        Free(handle, false);
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
        AssetBufferRef& bufferRef = Find(handle.handle, theDefault);

		Asset defaultAsset;
        const Asset& asset = bufferRef.mAsset;

        R2_CHECK(!asset.Empty() && defaultAsset.HashID() != asset.HashID(), "Failed to get the asset! Probably never loaded it!");


        AssetBuffer* assetBufferPtr = nullptr;

        if (bufferRef.mAssetBuffer != nullptr)
        {
            ++bufferRef.mRefCount;
            UpdateLRU(handle);

            assetBufferPtr = bufferRef.mAssetBuffer;
        }
        else
        {
            R2_CHECK(false, "We haven't loaded the asset yet!");
            return {};
        }

        AssetCacheRecord record( asset, assetBufferPtr, this );

        return record;
    }
    
    bool AssetCache::ReturnAssetBuffer(const AssetCacheRecord& buffer)
    {
        AssetBufferRef theDefault;
        AssetBufferRef& bufferRef = Find(buffer.GetAsset().HashID(), theDefault);
        
        bool found = bufferRef.mAssetBuffer != nullptr;
        
        if (found)
        {
            Free({ buffer.GetAsset().HashID(), mSlot }, false);
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
	AssetLoader* AssetCache::GetAssetLoader(r2::asset::AssetType type)
	{
        u64 numAssetLoaders = r2::sarr::Size(*mAssetLoaders);

		AssetLoader* loader = nullptr;
		for (u64 i = 0; i < numAssetLoaders; ++i)
		{
			AssetLoader* nextLoader = r2::sarr::At(*mAssetLoaders, i);
			if (nextLoader->GetType() == type)
			{
				loader = nextLoader;
				break;
			}
		}

        return loader;
	}

    bool AssetCache::IsLoaded(const Asset& asset)
    {
		AssetBufferRef theDefault;

		const AssetBufferRef& assetBufferRef = Find(asset.HashID(), theDefault);

        R2_CHECK(assetBufferRef.mRefCount > 0, "This buffer might be freed soon!");

        return assetBufferRef.mAssetBuffer != nullptr && assetBufferRef.mRefCount > 0;
    }

    AssetBuffer* AssetCache::Load(const Asset& asset)
    {
        AssetLoader* loader = GetAssetLoader(asset.GetType());
        
        if (loader == nullptr)
        {
            loader = mDefaultLoader;
        }
        
        R2_CHECK(loader != nullptr, "couldn't find asset loader");

        AssetLib& assetLib = MENG.GetAssetLib();

        AssetFile* theAssetFile = lib::GetAssetFileForAsset(assetLib, asset);

        if (!theAssetFile)
        {
#ifdef R2_ASSET_CACHE_DEBUG
            printf("Failed AssetCache::Load: %s\n We probably haven't added it to the AssetLib!\n", asset.Name().c_str());
#endif
            R2_CHECK(false, "Failed to load the asset: %llu", asset.HashID());
            return nullptr;
        }

        if (!theAssetFile->IsOpen())
        {
            theAssetFile->Open();
        }
        
        AssetHandle handle = { asset.HashID(), mSlot};
        u64 rawAssetSize = theAssetFile->RawAssetSize(asset);
        
        bool result = MakeRoom(rawAssetSize);
        
        R2_CHECK(result, "We don't have enough room to fit %s!\n", theAssetFile->FilePath());
        
        byte* rawAssetBuffer = ALLOC_BYTESN(mAssetCacheArena, rawAssetSize, alignof(size_t));
		
        if (rawAssetBuffer == nullptr)
		{
#ifdef R2_ASSET_CACHE_DEBUG
			R2_CHECK(false, "Failed to get the raw data from the asset: %s\n", asset.Name().c_str());
#endif
			return nullptr;
		}

        R2_CHECK(rawAssetBuffer != nullptr, "failed to allocate asset handle: %lli of size: %llu", handle, rawAssetSize);
        
        mMemoryHighWaterMark = std::max(mMemoryHighWaterMark, mAssetCacheArena.TotalBytesAllocated());

        theAssetFile->LoadRawAsset(asset, rawAssetBuffer, (u32)rawAssetSize);

        AssetBuffer* assetBuffer = nullptr;
        
        assetBuffer = ALLOC(AssetBuffer, *mAssetBufferPoolPtr);
#ifdef R2_ASSET_CACHE_DEBUG
        R2_CHECK(assetBuffer != nullptr, "Failed to allocate a new asset buffer for asset: %s\n", asset.Name().c_str());
#endif
        if (assetBuffer == nullptr)
        {
            theAssetFile->Close();
            return nullptr;
        }
        
        if (!loader->ShouldProcess())
        {
            assetBuffer->Load(rawAssetBuffer, rawAssetSize);
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

			if (!buffer)
			{
                R2_CHECK(false, "Failed to allocate buffer!");
				theAssetFile->Close();
				return nullptr;
			}

			mMemoryHighWaterMark = std::max(mMemoryHighWaterMark, mAssetCacheArena.TotalBytesAllocated());

			assetBuffer->Load(buffer, size);

			success = loader->LoadAsset(theAssetFile->FilePath(), rawAssetBuffer, rawAssetSize, *assetBuffer);
				
            if (!success)
			{
                R2_CHECK(false, "Failed to load asset for path: %s\n", theAssetFile->FilePath());
				theAssetFile->Close();
				return nullptr;
			}

			FREE(rawAssetBuffer, mAssetCacheArena);
        }

        AssetBufferRef bufferRef;
        bufferRef.mAssetBuffer = assetBuffer;
        bufferRef.mAsset = asset;

        r2::sarr::Push(*mAssetMap, bufferRef);

        UpdateLRU(handle);
        
#if ASSET_CACHE_DEBUG
        PrintAssetMap();
#endif
        theAssetFile->Close();

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
    }

    AssetCache::AssetBufferRef& AssetCache::Find(u64 handle, AssetCache::AssetBufferRef& theDefault)
    {
        const u32 numAssetsInMap = r2::sarr::Size(*mAssetMap);

        for (u32 i = 0; i < numAssetsInMap; ++i)
        {
            auto& assetBufferRef = r2::sarr::At(*mAssetMap, i);
            if (assetBufferRef.mAsset.HashID() == handle)
            {
                return assetBufferRef;
            }
        }

        return theDefault;
    }

    void AssetCache::RemoveAssetBuffer(u64 handle)
    {
		const u32 numAssetsInMap = r2::sarr::Size(*mAssetMap);

        for (u32 i = 0; i < numAssetsInMap; ++i)
        {
            auto& assetBufferRef = r2::sarr::At(*mAssetMap, i);
            if (assetBufferRef.mAsset.HashID() == handle)
            {
                r2::sarr::RemoveAndSwapWithLastElement(*mAssetMap, i);
                break;
            }
        }
    }
    
    void AssetCache::Free(AssetHandle handle, bool forceFree)
    {
        AssetBufferRef theDefault;

        AssetBufferRef& assetBufferRef = Find(handle.handle, theDefault);
        
        if (assetBufferRef.mAssetBuffer != nullptr)
        {
            --assetBufferRef.mRefCount;
            
            if (forceFree && assetBufferRef.mRefCount > 0)
            {
                const Asset& theAsset = assetBufferRef.mAsset;
#ifdef R2_ASSET_CACHE_DEBUG
                R2_CHECK(false, "AssetCache::Free() - we're trying to free the asset: %s but we still have %u references to it!", theAsset.Name().c_str(),  assetBufferRef.mRefCount);
#endif
            }
            
            if (assetBufferRef.mRefCount < 0)
            {
                const Asset& theAsset = assetBufferRef.mAsset;

                //Do this before we actually do the freeing
                AssetLoader* assetLoader = GetAssetLoader(theAsset.GetType());
                if (assetLoader)
                {
                    assetLoader->FreeAsset(*assetBufferRef.mAssetBuffer);
                }

                FREE(assetBufferRef.mAssetBuffer->MutableData(), mAssetCacheArena);
                
                FREE(assetBufferRef.mAssetBuffer, *mAssetBufferPoolPtr);

                RemoveAssetBuffer(handle.handle);

                RemoveFromLRU(handle);
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

    u64 AssetCache::TotalMemoryNeeded(u64 numAssets, u64 assetCapacity, u64 alignment, u32 lruCapacity, u32 mapCapacity)
    {
        u64 elementSize = sizeof(AssetBuffer);
        u32 boundsChecking = 0;
        u32 headerSize = r2::mem::FreeListAllocator::HeaderSize();
#if defined(R2_DEBUG) || defined(R2_RELEASE)

        boundsChecking = r2::mem::BasicBoundsChecking::SIZE_BACK + r2::mem::BasicBoundsChecking::SIZE_FRONT;
        elementSize = elementSize + boundsChecking;
        headerSize = r2::mem::MallocAllocator::HeaderSize();
#endif
        u64 poolSizeInBytes = lruCapacity * elementSize;

        return 
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<AssetHandle>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking)  +
            r2::mem::utils::GetMaxMemoryForAllocation(SArray<AssetBufferRef>::MemorySize(mapCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(DefaultAssetLoader), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AssetLoader*>::MemorySize(lruCapacity), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Asset>::MemorySize(mapCapacity), alignment, headerSize, boundsChecking) +
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
            AssetBufferRef bufRef = Find((*mAssetLRU)[i].handle, defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
                Asset theDefault;
                const Asset& asset = bufRef.mAsset;
                

                AssetHandle handle = { asset.HashID(), mSlot };
                PrintAsset(asset.Name().c_str(), handle, bufRef.mRefCount);
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
            AssetBufferRef bufRef = Find((*mAssetLRU)[i].handle, defaultRef);
            
            if (bufRef.mAssetBuffer != nullptr)
            {
				Asset theDefault;
                const Asset& asset = bufRef.mAsset;

                AssetHandle handle = { asset.HashID(), mSlot };
                PrintAsset(asset.Name().c_str(), handle, bufRef.mRefCount);
            }
        }
        
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
}
