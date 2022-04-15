//
//  AssetLib.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-16.
//
#include "r2pch.h"
#include "AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetFile.h"

#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/GLTFAssetFile.h"
#include "r2/Core/Assets/RawAssetFile.h"
#include "r2/Core/Assets/ZipAssetFile.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#else
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#endif

namespace r2::asset::lib
{
    r2::mem::utils::MemBoundary s_boundary;
    AssetCache** s_assetCaches = nullptr;
    FileList* s_filesForCaches = nullptr;
    
    u64 s_numCaches = 0;
//    r2::SHashMap<u64>* s_fileToAssetCacheMap = nullptr;
#ifdef R2_ASSET_PIPELINE
    r2::asset::pln::AssetThreadSafeQueue<std::vector<std::string>> s_assetsBuiltQueue;
    std::vector<AssetFilesBuiltListener> s_listeners;
#endif
    
#ifdef R2_ASSET_PIPELINE
    r2::mem::MallocArena* s_arenaPtr = nullptr;
#else
    r2::mem::FreeListArena* s_arenaPtr = nullptr;
#endif
    
    const u64 MAX_FILES_TO_ASSET_CACHE_CAPACITY = 2000;
    const u64 MAX_ASSET_CACHES = 1000;
    
    bool Init(const r2::mem::utils::MemBoundary& boundary)
    {
#ifdef R2_ASSET_PIPELINE
        s_arenaPtr = ALLOC_PARAMS(r2::mem::MallocArena, *MEM_ENG_PERMANENT_PTR, boundary);
#else
        s_arenaPtr = ALLOC_PARAMS(r2::mem::FreeListArena, *MEM_ENG_PERMANENT_PTR, boundary);
#endif
        
        
        s_assetCaches = ALLOC_ARRAY(r2::asset::AssetCache*[MAX_ASSET_CACHES], *s_arenaPtr);
        s_filesForCaches = ALLOC_ARRAY(FileList[MAX_ASSET_CACHES], *s_arenaPtr);
        
        for (u64 i = 0; i < MAX_ASSET_CACHES; ++i)
        {
            s_assetCaches[i] = nullptr;
            s_filesForCaches[i] = nullptr;
        }
        
       // s_fileToAssetCacheMap = MAKE_SHASHMAP(*s_arenaPtr, u64, MAX_FILES_TO_ASSET_CACHE_CAPACITY);
        
        return s_assetCaches != nullptr;//&& s_fileToAssetCacheMap != nullptr;
    }
    
    void Update()
    {
#ifdef R2_ASSET_PIPELINE
        if (s_assetCaches)// && s_fileToAssetCacheMap)
        {
            std::vector<std::string> paths;
            
            if(s_assetsBuiltQueue.TryPop(paths))
            {
                for (r2::asset::lib::AssetFilesBuiltListener listener : s_listeners)
                {
                    listener(paths);
                }
            }

        }
#endif
    }
    
    void Shutdown()
    {
        if (s_assetCaches)// && s_fileToAssetCacheMap)
        {

            for (u64 i = 0; i < MAX_ASSET_CACHES; ++i)
            {
                r2::asset::AssetCache* cache = s_assetCaches[i];
                if (cache != nullptr)
                {
                    FREE(cache, *s_arenaPtr);
                }
                
                FileList fileList = s_filesForCaches[i];
                
                if (fileList != nullptr)
                {
                    u64 numFiles = r2::sarr::Size(*fileList);
                    for (u64 i = 0; i < numFiles; ++i)
                    {
                        AssetFile* file = r2::sarr::At(*fileList, i);
                        
                        if (file->IsOpen())
                        {
                            file->Close();
                        }
                        
                        FREE(file, *s_arenaPtr);
                    }
                    
                    FREE(fileList, *s_arenaPtr);
                }
            }
            
       //     FREE(s_fileToAssetCacheMap, *s_arenaPtr);
            FREE(s_assetCaches, *s_arenaPtr);
            FREE(s_filesForCaches, *s_arenaPtr);
        }
        
        FREE(s_arenaPtr, *MEM_ENG_PERMANENT_PTR);
        s_arenaPtr = nullptr;
    }
    
    void AddFiles(const r2::asset::AssetCache& cache, FileList list)
    {
        //if (s_assetCaches)// && s_fileToAssetCacheMap)
        //{
        //    u64 numFiles = r2::sarr::Size(*list);
        //    
        //    for (u64 i = 0; i < numFiles; ++i)
        //    {
        //        r2::asset::AssetFile* file = r2::sarr::At(*list, i);
        //        u64 hash = r2::utils::Hash<const char*>{}(file->FilePath());
        //        
        //        r2::shashmap::Set(*s_fileToAssetCacheMap, hash, cache.GetSlot());
        //    }
        //}
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

    void ResetFilesForCache(const r2::asset::AssetCache& cache, FileList list)
    {
        R2_CHECK(list != nullptr, "passed in a null list!");
        const FileList fileList = cache.GetFileList();

        R2_CHECK(fileList != nullptr, "The cache had an empty list?");
        const auto numFiles = r2::sarr::Size(*cache.GetFileList());

		for (u64 i = 0; i < numFiles; ++i)
		{
			AssetFile* file = r2::sarr::At(*fileList, i);

			if (file->IsOpen())
			{
				file->Close();
			}

			FREE(file, *s_arenaPtr);
		}

        FREE(fileList, *s_arenaPtr);

        s_filesForCaches[cache.GetSlot()] = list;
    }
#endif
    
    FileList MakeFileList(u64 capacity)
    {
        return MAKE_SARRAY(*s_arenaPtr, AssetFile*, capacity);
    }
    
    RawAssetFile* MakeRawAssetFile(const char* path, u32 numParentDirectoriesToInclude)
    {
        RawAssetFile* rawAssetFile = ALLOC(RawAssetFile, *s_arenaPtr);
        
        bool result = rawAssetFile->Init(path, numParentDirectoriesToInclude);
        R2_CHECK(result, "Failed to initialize raw asset file");
        return rawAssetFile;
    }

    GLTFAssetFile* MakeGLTFAssetFile(const char* path, u32 numParentDirectoriesToInclude)
    {
        GLTFAssetFile* gltfAssetFile = ALLOC(GLTFAssetFile, *s_arenaPtr);

		bool result = gltfAssetFile->Init(path, numParentDirectoriesToInclude);
		R2_CHECK(result, "Failed to initialize raw asset file");
		return gltfAssetFile;
    }
    
    ZipAssetFile* MakeZipAssetFile(const char* path)
    {
        ZipAssetFile* zipAssetFile = ALLOC(ZipAssetFile, *s_arenaPtr);
        
        bool result = zipAssetFile->Init(path,
                                     [](u64 size, u64 alignment)
                                     {
                                         return ALLOC_BYTESN(*s_arenaPtr, size, alignment);
                                     },
                                     [](byte* ptr)
                                     {
                                         FREE(ptr, *s_arenaPtr);
                                     });
        
        R2_CHECK(result, "Failed to initialize zip asset file");
        return zipAssetFile;
    }
    
    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, r2::asset::FileList files)
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
            
            r2::asset::AssetCache* cache = ALLOC_PARAMS(r2::asset::AssetCache, *s_arenaPtr, slot, boundary);
            
            if (cache)
            {
                bool created = cache->Init(files);
                R2_CHECK(created, "Failed to initialize asset cache for slot: %llu", slot);
                if (!created)
                {
                    FREE(cache, *s_arenaPtr);
                    return nullptr;
                }
                
                s_assetCaches[slot] = cache;
                s_filesForCaches[slot] = files;

                s_numCaches++;
            }
            
            return cache;
        }
        
        return nullptr;
    }
    
    r2::asset::AssetCache* GetAssetCache(s64 slot)
    {
        if (s_assetCaches && (slot >= 0 && slot < static_cast<s64>(s_numCaches)))
        {
            //R2_CHECK(s_assetCaches[slot] != nullptr, "We don't have the proper cache for this slot!");
            return s_assetCaches[slot];
        }

        R2_CHECK(s_assetCaches != nullptr, "We haven't initialized the asset caches yet!");
        R2_CHECK(slot >= 0 && slot < static_cast<s64>(s_numCaches), "Passed in an invalid asset cache slot: %lli", slot);
        return nullptr;
    }


    void DestroyCache(r2::asset::AssetCache* cache)
    {
        if (s_assetCaches)
        {
            s64 slot = cache->GetSlot();
            R2_CHECK(slot >= 0, "We don't have a proper asset cache slot!");
            R2_CHECK(s_assetCaches[slot] == cache, "We don't have the proper cache for this slot!");
            
            cache->Shutdown();
            
            FREE(cache, *s_arenaPtr);
            
            FileList fileList = s_filesForCaches[slot];
            R2_CHECK(fileList != nullptr, "FileList should not be nullptr");
            u64 numFiles = r2::sarr::Size(*fileList);
            for (u64 i = 0; i < numFiles; ++i)
            {
                AssetFile* file = r2::sarr::At(*fileList, i);
                
                if (file->IsOpen())
                {
                    file->Close();
                }

                FREE(file, *s_arenaPtr);
            }
            
            FREE(fileList, *s_arenaPtr);
            
            s_assetCaches[slot] = nullptr;
            s_filesForCaches[slot] = nullptr;
        }
    }
    
    u64 EstimateMaxMemUsage(u32 maxZipArchiveCentralDirSize, u64 maxFilesInList)
    {
        u64 assetMemUsage = r2::SHashMap<u64>::MemorySize(MAX_FILES_TO_ASSET_CACHE_CAPACITY) + r2::SArray<AssetCache*>::MemorySize(MAX_ASSET_CACHES) +
            sizeof(AssetCache) * MAX_ASSET_CACHES;
        
        u64 filesMemUsage = r2::SArray<FileList>::MemorySize(MAX_ASSET_CACHES) + MAX_ASSET_CACHES * maxFilesInList * (sizeof(ZipAssetFile) + sizeof(r2::fs::ZipFile) + maxZipArchiveCentralDirSize);
        
        return filesMemUsage + assetMemUsage;
    }
}
