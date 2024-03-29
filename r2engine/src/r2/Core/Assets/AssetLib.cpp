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
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/RModelsManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/SoundManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/LevelPackManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/EngineModelManifestAssetFile.h"
#include "r2/Core/Assets/AssetFiles/SoundAssetFile.h"

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
    s64 s_numCaches = -1;

#ifdef R2_ASSET_PIPELINE

    bool ReloadManifestFileInternal(AssetLib& assetLib, ManifestAssetFile& manifestFile, const std::vector<std::string>& paths, pln::HotReloadType type);

#endif
    
#ifdef R2_ASSET_PIPELINE
    r2::mem::MallocArena* s_arenaPtr = nullptr;
#else
    r2::mem::FreeListArena* s_arenaPtr = nullptr;
#endif

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

        u64 totalSize = AssetCache::TotalMemoryNeeded(numManifests, cacheSize, ALIGNMENT);

        newAssetLib->mAssetCacheBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*stackArena, totalSize, ALIGNMENT, "Asset lib's asset cache");

        newAssetLib->mAssetCache = CreateAssetCache(newAssetLib->mAssetCacheBoundary, cacheSize, numManifests);

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

            if (manifestFile->GetManifestAssetType() == type)
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

            if (manifestFile->GetManifestAssetType() == type)
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

            if (manifestFile->GetManifestAssetType() == type)
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

			if (manifestFile->GetManifestAssetType() == type)
			{
                ++numManifestsForType;
			}
		}

        return numManifestsForType;
    }

    EngineAssetType GetManifestTypeForAssetType(r2::asset::AssetType type)
    {
        if (type == RMODEL)
        {
            return RMODEL_MANIFEST;
        }
        else if (type == MATERIAL)
        {
            return MATERIAL_PACK_MANIFEST;
        }
        else if (type == TEXTURE || type == CUBEMAP_TEXTURE)
        {
            return TEXTURE_PACK_MANIFEST;
        }
        else if (type == SOUND)
        {
            return SOUND_DEFINTION;
        }
        else if (type == MODEL || type == MESH)
        {
            return MODEL_MANIFEST;
        }
        else if (type == LEVEL || type == LEVEL_GROUP)
        {
            return LEVEL_PACK_MANIFEST;
        }
        else
        {
            R2_CHECK(false, "Unsupported asset type");
            return DEFAULT;
        }
    }




    bool HasAsset(AssetLib& assetLib, const char* path, r2::asset::AssetType type)
    {
        r2::asset::Asset asset = r2::asset::Asset::MakeAssetFromFilePath(path, type);

        return HasAsset(assetLib, asset);
    }

    bool HasAsset(AssetLib& assetLib, const Asset& asset)
    {
        if (IsManifestFile(asset.GetType()))
        {
            //look through our manifests and see which one it is
            u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);
            for (u32 i = 0; i < numManifests; ++i)
            {
                const auto* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);
                if (manifestFile->GetManifestFileHandle() == asset.HashID() && manifestFile->GetManifestAssetType() == asset.GetType())
                {
                    return true;
                }
            }
            return false;
        }

        auto manifestType = GetManifestTypeForAssetType(asset.GetType());

		u32 manifestsCount = GetManifestCountForType(assetLib, manifestType);

		if (manifestsCount == 0)
		{
			R2_CHECK(false, "Probably a bug here");
			return false;
		}

		r2::SArray<ManifestAssetFile*>* manifestAssetFilesForType = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ManifestAssetFile*, manifestsCount);

		R2_CHECK(manifestAssetFilesForType != nullptr, "Should never happen");

		GetManifestFilesForType(assetLib, manifestType, manifestAssetFilesForType);

		u32 numManifestsForType = r2::sarr::Size(*manifestAssetFilesForType);

        bool found = false;
        
		for (u32 i = 0; i < numManifestsForType; ++i)
		{
			auto* manifestAssetFile = r2::sarr::At(*manifestAssetFilesForType, i);

			if (manifestAssetFile->HasAsset(asset))
			{
                found = true;
                break;
			}
		}

        FREE(manifestAssetFilesForType, *MEM_ENG_SCRATCH_PTR);
        return found;
    }

    r2::asset::AssetFile* GetAssetFileForAsset(AssetLib& assetLib, const Asset& asset)
    {

		if (IsManifestFile(asset.GetType()))
		{
			//look through our manifests and see which one it is
			u32 numManifests = r2::sarr::Size(*assetLib.mManifestFiles);
			for (u32 i = 0; i < numManifests; ++i)
			{
				const auto* manifestFile = r2::sarr::At(*assetLib.mManifestFiles, i);
				if (manifestFile->GetManifestFileHandle() == asset.HashID() && manifestFile->GetManifestAssetType() == asset.GetType())
				{
					return manifestFile->GetManifestAssetFile();
				}
			}
			return nullptr;
		}



		auto manifestType = GetManifestTypeForAssetType(asset.GetType());

		u32 manifestsCount = GetManifestCountForType(assetLib, manifestType);

		if (manifestsCount == 0)
		{
			R2_CHECK(false, "Probably a bug here");
			return false;
		}

		r2::SArray<ManifestAssetFile*>* manifestAssetFilesForType = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ManifestAssetFile*, manifestsCount);

		R2_CHECK(manifestAssetFilesForType != nullptr, "Should never happen");

		GetManifestFilesForType(assetLib, manifestType, manifestAssetFilesForType);

		u32 numManifestsForType = r2::sarr::Size(*manifestAssetFilesForType);

        r2::asset::AssetFile* assetFile = nullptr;

        for (u32 i = 0; i < numManifestsForType; ++i)
        {
            auto* manifestAssetFile = r2::sarr::At(*manifestAssetFilesForType, i);

            if (manifestAssetFile->HasAsset(asset))
            {
                assetFile = manifestAssetFile->GetAssetFile(asset);
                break;
            }
        }

        FREE(manifestAssetFilesForType, *MEM_ENG_SCRATCH_PTR);
        return assetFile;
    }

#ifdef R2_ASSET_PIPELINE
	void ImportAssetFiles(AssetLib& assetLib, const std::vector<r2::asset::AssetReferenceAndType>& assetReferences)
	{
		for (const auto& assetReferenceWithType : assetReferences)
		{
            ImportAsset(assetLib, assetReferenceWithType.assetReference, assetReferenceWithType.type);
		}
	}

	bool ImportAsset(AssetLib& assetLib, const AssetReference& assetReference, r2::asset::AssetType type)
	{
		auto manifestType = GetManifestTypeForAssetType(type);

		u32 manifestsCount = GetManifestCountForType(assetLib, manifestType);

		if (manifestsCount == 0)
		{
			R2_CHECK(false, "Probably a bug here");
			return false;
		}

        R2_CHECK(manifestsCount == 1, "We need to only import into a specific manifest file - need a way of specifying in a parameter");

		r2::SArray<ManifestAssetFile*>* manifestAssetFilesForType = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ManifestAssetFile*, manifestsCount);
		R2_CHECK(manifestAssetFilesForType != nullptr, "Should never happen");

		GetManifestFilesForType(assetLib, manifestType, manifestAssetFilesForType);

		u32 numManifestsForType = r2::sarr::Size(*manifestAssetFilesForType);

		R2_CHECK(numManifestsForType == 1, "We need to only import into a specific manifest file - need a way of specifying in a parameter");

        auto* manifestAssetFile = r2::sarr::At(*manifestAssetFilesForType, 0);

        bool result = manifestAssetFile->AddAssetReference(assetReference);

        FREE(manifestAssetFilesForType, *MEM_ENG_SCRATCH_PTR);

        return result;
	}

    bool ReloadManifestFileInternal(AssetLib& assetLib, ManifestAssetFile& manifestFile, const std::vector<std::string>& changedPaths, pln::HotReloadType type)
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
        newEntry.hotReloadType = pln::CHANGED;
        newEntry.manifestPath = manifestFilePath;
        newEntry.changedPaths = filePathChanged;

        assetLib.mManifestChangedRequests.Push(newEntry);        
    }

    void PathAddedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& filePathAdded)
    {
		ManifestReloadEntry newEntry;
		newEntry.hotReloadType = pln::ADDED;
		newEntry.manifestPath = manifestFilePath;
		newEntry.changedPaths = filePathAdded;

		assetLib.mManifestChangedRequests.Push(newEntry);
    }

    void PathRemovedInManifest(AssetLib& assetLib, const std::string& manifestFilePath, const std::vector<std::string>& filePathRemoved)
    {
		ManifestReloadEntry newEntry;
		newEntry.hotReloadType = pln::DELETED;
		newEntry.manifestPath = manifestFilePath;
		newEntry.changedPaths = filePathRemoved;

		assetLib.mManifestChangedRequests.Push(newEntry);
    }

    std::vector<r2::asset::AssetFile*> GetAllAssetFilesForType(AssetLib& assetLib, r2::asset::AssetType type)
    {
		auto manifestType = GetManifestTypeForAssetType(type);

		u32 manifestsCount = GetManifestCountForType(assetLib, manifestType);

        if (manifestsCount == 0)
        {
            R2_CHECK(false, "Probably a bug here");
            return {};
		}

		r2::SArray<ManifestAssetFile*>* manifestAssetFilesForType = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ManifestAssetFile*, manifestsCount);

		R2_CHECK(manifestAssetFilesForType != nullptr, "Should never happen");

		GetManifestFilesForType(assetLib, manifestType, manifestAssetFilesForType);

		u32 numManifestsForType = r2::sarr::Size(*manifestAssetFilesForType);
       
        std::vector<r2::asset::AssetFile*> assetFiles;

        for (u32 i = 0; i < numManifestsForType; ++i)
        {
            const ManifestAssetFile* manifestAssetFile = r2::sarr::At(*manifestAssetFilesForType, i);
            FileList assetFilesInManifest = manifestAssetFile->GetAssetFiles();

            if (!assetFilesInManifest)
            {
                continue;
            }

            for (u32 j = 0; j < r2::sarr::Size(*assetFilesInManifest); ++j)
            {
                assetFiles.push_back(r2::sarr::At(*assetFilesInManifest, j));
            }
        }

        FREE(manifestAssetFilesForType, *MEM_ENG_SCRATCH_PTR);
        return assetFiles;
    }

    std::vector<r2::asset::AssetName> GetAllAssetNamesForType(AssetLib& assetLib, r2::asset::AssetType type)
    {
		auto manifestType = GetManifestTypeForAssetType(type);

		u32 manifestsCount = GetManifestCountForType(assetLib, manifestType);

		if (manifestsCount == 0)
		{
			R2_CHECK(false, "Probably a bug here");
			return {};
		}

		r2::SArray<ManifestAssetFile*>* manifestAssetFilesForType = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ManifestAssetFile*, manifestsCount);

		R2_CHECK(manifestAssetFilesForType != nullptr, "Should never happen");

		GetManifestFilesForType(assetLib, manifestType, manifestAssetFilesForType);

		u32 numManifestsForType = r2::sarr::Size(*manifestAssetFilesForType);

        std::vector<r2::asset::AssetName> assetNames;

        for (u32 i = 0; i < numManifestsForType; ++i)
        {
            const ManifestAssetFile* manifestAssetFile = r2::sarr::At(*manifestAssetFilesForType, i);

            auto nextAssetNames = manifestAssetFile->GetAssetNames();

            assetNames.insert(assetNames.end(), nextAssetNames.begin(), nextAssetNames.end());
        }

        FREE(manifestAssetFilesForType, *MEM_ENG_SCRATCH_PTR);
        return assetNames;
    }

#endif

    bool Init(const r2::mem::utils::MemBoundary& boundary)
    {
#ifdef R2_ASSET_PIPELINE
        s_arenaPtr = ALLOC_PARAMS(r2::mem::MallocArena, *MEM_ENG_PERMANENT_PTR, boundary);
#else
        s_arenaPtr = ALLOC_PARAMS(r2::mem::FreeListArena, *MEM_ENG_PERMANENT_PTR, boundary);
#endif
        return s_arenaPtr != nullptr;
    }
    
    void Update()
    {
    }
    
    void Shutdown()
    {
        FREE(s_arenaPtr, *MEM_ENG_PERMANENT_PTR);
        s_arenaPtr = nullptr;
    }
    
    FileList MakeFileList(u64 capacity)
    {
        return MAKE_SARRAY(*s_arenaPtr, AssetFile*, capacity);
    }
    
    void DestoryFileList(const FileList fileList)
    {
        FREE(fileList, *s_arenaPtr);
    }

    void FreeRawAssetFile(RawAssetFile* file)
    {
        if (file->IsOpen())
        {
            file->Close();
        }
       
        FREE(file, *s_arenaPtr);
    }

    SoundAssetFile* MakeSoundAssetFile(const char* path)
    {
        SoundAssetFile* soundAssetFile = ALLOC(SoundAssetFile, *s_arenaPtr);
        bool result = soundAssetFile->Init(path);
        R2_CHECK(result, "Failed to initialize the sound asset file");
        return soundAssetFile;
    }

    void FreeSoundAssetFile(SoundAssetFile* file)
    {
		if (file->IsOpen())
		{
			file->Close();
		}

		FREE(file, *s_arenaPtr);
    }

    RawAssetFile* MakeRawAssetFile(const char* path, r2::asset::AssetType assetType)
    {
        RawAssetFile* rawAssetFile = ALLOC(RawAssetFile, *s_arenaPtr);
        bool result = rawAssetFile->Init(path, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
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

    ManifestAssetFile* MakeRModelsManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        RModelsManifestAssetFile* manifestFile = ALLOC(RModelsManifestAssetFile, *s_arenaPtr);
        bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::RMODEL_MANIFEST);
		R2_CHECK(result, "Failed to initialize the RModelManifestAssetFile");
		return manifestFile;
    }

    ManifestAssetFile* MakeSoundsManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        SoundManifestAssetFile* manifestFile = ALLOC(SoundManifestAssetFile, *s_arenaPtr);
		bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::SOUND_DEFINTION);
		R2_CHECK(result, "Failed to initialize the SoundManifestAssetFile");
        return manifestFile;
    }

    ManifestAssetFile* MakeLevelPackManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        LevelPackManifestAssetFile* manifestFile = ALLOC(LevelPackManifestAssetFile, *s_arenaPtr);
        bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::LEVEL_PACK_MANIFEST);
        R2_CHECK(result, "Failed to initialize the LevelPackManifestAssetFile");
        return manifestFile;
    }

    ManifestAssetFile* MakeEngineModelManifestAssetFile(AssetLib& assetLib, const char* binPath, const char* rawPath, const char* watchPath)
    {
        EngineModelManifestAssetFile* manifestFile = ALLOC(EngineModelManifestAssetFile, *s_arenaPtr);
        bool result = manifestFile->Init(assetLib.mAssetCache, binPath, rawPath, watchPath, r2::asset::MODEL_MANIFEST);
		R2_CHECK(result, "Failed to initialize the EngineModelManifestAssetFile");
		return manifestFile;
    }

    r2::asset::AssetCache* CreateAssetCache(const r2::mem::utils::MemBoundary& boundary, u32 assetTotalSize, u32 numFiles)
    {
        if (s_arenaPtr)
        {
			s64 slot = ++s_numCaches;

            const auto memoryNeeded = r2::asset::AssetCache::TotalMemoryNeeded(numFiles, assetTotalSize, boundary.alignment);
            
            R2_CHECK(boundary.size >= memoryNeeded, "Our boundary for this asset cache is not big enough. We need: %llu, but have: %llu, difference: %llu", memoryNeeded, boundary.size, static_cast<s64>(memoryNeeded) - static_cast<s64>(boundary.size));

            r2::asset::AssetCache* cache = ALLOC_PARAMS(r2::asset::AssetCache, *s_arenaPtr, slot , boundary);
            
            if (cache)
            {
                bool created = cache->Init();
                R2_CHECK(created, "Failed to initialize asset cache for slot: %llu", slot);
                if (!created)
                {
                    FREE(cache, *s_arenaPtr);
                    return nullptr;
                }
            }
            
            return cache;
        }
        
        return nullptr;
    }

    void DestroyCache(r2::asset::AssetCache* cache)
    {
        if (s_arenaPtr)
        {
            s64 slot = cache->GetSlot();
            R2_CHECK(slot >= 0, "We don't have a proper asset cache slot!");
            cache->Shutdown();
            
            FREE(cache, *s_arenaPtr);
        }
    }
}
