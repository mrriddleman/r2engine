//
//  AssetLib.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-16.
//
#include "r2pch.h"
#include "AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/AssetFiles/RawAssetFile.h"
#include "r2/Core/Assets/AssetFiles/ZipAssetFile.h"
#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/ManifestSingleAssetFile.h"
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"
#include "r2/Core/File/PathUtils.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"

#else
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#endif


namespace r2::asset
{
	u64 AssetLib::MemorySize(u32 cacheSize, u32 numGameManifests, u32 numEngineManifests)
	{
		u64 memorySize = 0;

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AssetLib), 16, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), 16, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ManifestAssetFile*>::MemorySize(numGameManifests), 16, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ManifestAssetFile*>::MemorySize(numEngineManifests), 16, stackHeaderSize, boundsChecking);
		memorySize += AssetCache::TotalMemoryNeeded(numGameManifests + numEngineManifests, cacheSize, 16);

		return memorySize;
	}
}

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


    bool ReloadManifestFileInternal(AssetLib& assetLib, ManifestAssetFile& manifestFile, const std::vector<std::string>& paths, HotReloadType type);

#endif
    
#ifdef R2_ASSET_PIPELINE
    r2::mem::MallocArena* s_arenaPtr = nullptr;
#else
    r2::mem::FreeListArena* s_arenaPtr = nullptr;
#endif
    
    const u64 MAX_FILES_TO_ASSET_CACHE_CAPACITY = 2000;
    const u64 MAX_ASSET_CACHES = 1000;
    

    const u32 MAX_NUM_GAME_ASSET_FILES = 2000;
    const u32 ALIGNMENT = 16;

    AssetLib* Create(const r2::mem::utils::MemBoundary& boundary, u32 numManifests, u32 cacheSize)
    {
        r2::mem::StackArena* stackArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(boundary);

        R2_CHECK(stackArena != nullptr, "Failed to emplace the stack arena");

        AssetLib* newAssetLib = ALLOC(AssetLib, *stackArena);

        R2_CHECK(newAssetLib != nullptr, "We couldn't allocate the asset lib");

        newAssetLib->mArena = stackArena;
        newAssetLib->mBoundary = boundary;

        newAssetLib->mManifestFiles = MAKE_SARRAY(*stackArena, ManifestAssetFile*, numManifests);

        newAssetLib->mGameFileList = MakeFileList(MAX_NUM_GAME_ASSET_FILES);

        u64 totalSize = AssetCache::TotalMemoryNeeded(numManifests, cacheSize, ALIGNMENT);

        newAssetLib->mAssetCacheBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*stackArena, totalSize, ALIGNMENT, "Asset lib's asset cache");

        FileList fileList = MakeFileList(numManifests);

        R2_CHECK(fileList != nullptr, "Failed to create the fileList");

        //@TODO(Serge): remove the CreateAssetCache + MakeFileList stuff
        newAssetLib->mAssetCache = CreateAssetCache(newAssetLib->mAssetCacheBoundary, cacheSize, fileList);

        R2_CHECK(newAssetLib->mAssetCache != nullptr, "Failed to create the AssetCache");

        return newAssetLib;
    }

    void Shutdown(AssetLib* assetLib)
    {
        if (assetLib == nullptr)
        {
            R2_CHECK(false, "assetLib is nullptr?");
            return;
        }

        u32 numManifests = r2::sarr::Size(*assetLib->mManifestFiles);
        for (u32 i = 0; i < numManifests; ++i)
        {
            ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib->mManifestFiles, i);
            manifestFile->UnloadManifest();
            manifestFile->Shutdown();

            FREE(manifestFile, *s_arenaPtr);
        }
        r2::sarr::Clear(*assetLib->mManifestFiles);
        

        r2::mem::StackArena* arena = assetLib->mArena;

        DestroyCache(assetLib->mAssetCache);

        FREE(assetLib->mAssetCacheBoundary.location, *arena);

        assetLib->mGameFileList = nullptr;

        FREE(assetLib->mManifestFiles, *arena);

        FREE(assetLib, *arena);

        FREE_EMPLACED_ARENA(arena);
    }

    void Update(AssetLib& assetLib)
    {
#ifdef R2_ASSET_PIPELINE
        ManifestReloadEntry entry;
        bool shouldRegenerateFiles = false;
        while (assetLib.mManifestChangedRequests.TryPop(entry))
        {
            shouldRegenerateFiles = true;
            const u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);
            ManifestAssetFile* foundManifest = nullptr;

            for (u32 i = 0; i < numManifests; ++i)
            {
                ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);
                std::filesystem::path nextManifestFilePath = manifestFile->FilePath();

                if (entry.manifestPath == nextManifestFilePath)
                {
                    foundManifest = manifestFile;
                    break;
                }
            }

            if (!foundManifest)
            {
                continue;
            }

            ReloadManifestFileInternal(assetLib, *foundManifest, entry.changedPaths, entry.hotReloadType);
        }

        if (shouldRegenerateFiles)
        {
            //@TODO(Serge): figure out a better way to do this.
			//hmm maybe dangerous, but, we need to update the files in the GameAssetManager somehow
			//RegenerateAssetFilesFromManifests(assetLib);
        }
#endif
    }

    const byte* GetManifestData(AssetLib& assetLib, ManifestAssetFile& manifestFile)
    {
        return manifestFile.GetManifestData();
    }

    const byte* GetManifestData(AssetLib& assetLib, u64 manifestAssetHandle)
    {
        ManifestAssetFile* foundManifestFile = nullptr;

        const u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numManifests; ++i)
        {
            ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            if (manifestFile->GetManifestFileHandle() == manifestAssetHandle)
            {
                foundManifestFile = manifestFile;
                break;
            }
        }
        
        if (!foundManifestFile)
        {
            R2_CHECK(false, "Failed to find the manifest for: %llu", manifestAssetHandle);
            return nullptr;
        }

        return foundManifestFile->GetManifestData();
    }

    void GetManifestDataForType(AssetLib& assetLib, r2::asset::EngineAssetType type, r2::SArray<const byte*>* manifestDataArray)
    {
        u32 numManifestFiles = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numManifestFiles; ++i)
        {
            r2::asset::ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            if (manifestFile->GetAssetType() == type)
            {
                r2::sarr::Push(*manifestDataArray, GetManifestData(assetLib, manifestFile->GetManifestFileHandle()));
            }
        }
    }

    u32 GetManifestDataCountForType(AssetLib& assetLib, r2::asset::EngineAssetType type)
    {
        u32 count = 0;
		u32 numManifestFiles = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numManifestFiles; ++i)
        {
            r2::asset::ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            if (manifestFile->GetAssetType() == type)
            {
                count++;
            }
        }

        return count;
    }

    void RegisterAndLoadManifestFile(AssetLib& assetLib, ManifestAssetFile* manifestFile)
    {
        r2::sarr::Push(*assetLib.mManifestFiles, manifestFile);
        manifestFile->LoadManifest();
    }

    void CloseAndFreeAllFilesInFileList(FileList fileList)
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

        r2::sarr::Clear(*fileList);
    }

    bool RegenerateAssetFilesFromManifests(const AssetLib& assetLib)
    {
        FileList fileList = assetLib.mGameFileList;

        CloseAndFreeAllFilesInFileList(fileList);

        const u32 numGameManifests = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numGameManifests; ++i)
        {
            ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            bool result = manifestFile->AddAllFilePaths(fileList);

            R2_CHECK(result, "Failed to add the files");
        }

        return true;
    }

    FileList GetFileList(const AssetLib& assetLib)
    {
        return assetLib.mGameFileList;
    }

    ManifestAssetFile* GetManifest(AssetLib& assetLib, u64 manifestAssetHandle)
    {
        u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numManifests; ++i)
        {
            ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            if (manifestFile->GetManifestFileHandle() == manifestAssetHandle)
            {
                return manifestFile;
            }
        }

        return nullptr;
    }

    void GetManifestFilesForType(AssetLib& assetLib, r2::asset::EngineAssetType type, r2::SArray<ManifestAssetFile*>* manifests)
    {
		u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);

        for (u32 i = 0; i < numManifests; ++i)
        {
            ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

            if (manifestFile->GetAssetType() == type)
            {
                r2::sarr::Push(*manifests, manifestFile);
            }
        }
    }

    u32 GetManifestCountForType(AssetLib& assetLib, r2::asset::EngineAssetType type)
    {
        u32 numManifestsForType = 0;

		u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);

		for (u32 i = 0; i < numManifests; ++i)
		{
			ManifestAssetFile* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);

			if (manifestFile->GetAssetType() == type)
			{
                ++numManifestsForType;
			}
		}

        return numManifestsForType;
    }

#ifdef R2_ASSET_PIPELINE

    bool ReloadManifestFileInternal(AssetLib& assetLib, ManifestAssetFile& manifestFile, const std::vector<std::string>& changedPaths, HotReloadType type)
    {
		bool hasReloaded = false;

        manifestFile.Reload();

        std::vector<std::string> pathsToUse;
        pathsToUse.reserve(changedPaths.size());

        for (const std::string& changedPath : changedPaths)
        {
			char sanitizedChangedPath[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(changedPath.c_str(), sanitizedChangedPath);

            pathsToUse.push_back(sanitizedChangedPath);
        }

        hasReloaded = manifestFile.ReloadFilePath(pathsToUse, type);

        return hasReloaded;
    }

    void PathChangedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& filePathChanged)
    {
        ManifestReloadEntry newEntry;
        newEntry.hotReloadType = CHANGED;
        newEntry.manifestPath = manifestFilePath;
        newEntry.changedPaths = filePathChanged;

        assetLib.mManifestChangedRequests.Push(newEntry);        
    }

    void PathAddedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& filePathAdded)
    {
		ManifestReloadEntry newEntry;
		newEntry.hotReloadType = ADDED;
		newEntry.manifestPath = manifestFilePath;
		newEntry.changedPaths = filePathAdded;

		assetLib.mManifestChangedRequests.Push(newEntry);
    }

    void PathRemovedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& filePathRemoved)
    {
		ManifestReloadEntry newEntry;
		newEntry.hotReloadType = DELETED;
		newEntry.manifestPath = manifestFilePath;
		newEntry.changedPaths = filePathRemoved;

		assetLib.mManifestChangedRequests.Push(newEntry);
    }

#endif

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
                    CloseAndFreeAllFilesInFileList(fileList);
                    
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
    
    ManifestAssetFile* MakeManifestSingleAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType)
    {
        ManifestSingleAssetFile* manifestFile = ALLOC(ManifestSingleAssetFile, *s_arenaPtr);

        bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, assetType);
        R2_CHECK(result, "Failed to initialize the ManifestSingleAssetFile");
        return manifestFile;
    }

    ManifestAssetFile* MakeTexturePackManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        TexturePackManifestAssetFile* manifestFile = ALLOC(TexturePackManifestAssetFile, *s_arenaPtr);

		bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::TEXTURE_PACK_MANIFEST);
		R2_CHECK(result, "Failed to initialize the TexturePackManifestAssetFile");
		return manifestFile;
    }

    ManifestAssetFile* MakeMaterialManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        MaterialManifestAssetFile* manifestFile = ALLOC(MaterialManifestAssetFile, *s_arenaPtr);
        bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::MATERIAL_PACK_MANIFEST);
        R2_CHECK(result, "Failed to initialize the MaterialManifestAssetFile");
        return manifestFile;
    }

    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, u32 assetTotalSize, r2::asset::FileList files)
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

            const auto memoryNeeded = r2::asset::AssetCache::TotalMemoryNeeded(r2::sarr::Size(*files), assetTotalSize, boundary.alignment);
            
            R2_CHECK(boundary.size >= memoryNeeded, "Our boundary for this asset cache is not big enough. We need: %llu, but have: %llu, difference: %llu", memoryNeeded, boundary.size, static_cast<s64>(memoryNeeded) - static_cast<s64>(boundary.size));

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
}
