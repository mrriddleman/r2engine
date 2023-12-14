#include "r2pch.h"

#include "r2/Render/Model/ModelCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/MeshAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"

namespace r2::draw::modlche
{
	const u64 ALIGNMENT = 16;
	const u32 FILE_CAPACITY = 16;

	ModelCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, b32 cacheModelReferences, const char* areaName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(FILE_CAPACITY, modelCacheSize);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes",
				memoryArea->UnAllocatedSpace(), subAreaSize);
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* modelArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(modelArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

		ModelCache* newModelSystem = ALLOC(ModelCache, *modelArena);

		newModelSystem->mMemoryAreaHandle = memoryAreaHandle;
		newModelSystem->mSubAreaHandle = subAreaHandle;
		newModelSystem->mSubAreaArena = modelArena;
		
		newModelSystem->mCacheModelReferences = cacheModelReferences;
		newModelSystem->mAssetBoundary = MAKE_BOUNDARY(*modelArena, r2::asset::AssetCache::TotalMemoryNeeded(FILE_CAPACITY, modelCacheSize, ALIGNMENT), ALIGNMENT);
		newModelSystem->mModels = MAKE_SHASHMAP(*modelArena, r2::asset::AssetCacheRecord, FILE_CAPACITY * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier());
		newModelSystem->mMeshes = MAKE_SHASHMAP(*modelArena, r2::asset::AssetCacheRecord, FILE_CAPACITY * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier());
		newModelSystem->mModelCache = r2::asset::lib::CreateAssetCache(newModelSystem->mAssetBoundary, modelCacheSize, FILE_CAPACITY);

		r2::asset::MeshAssetLoader* meshLoader = (r2::asset::MeshAssetLoader*)newModelSystem->mModelCache->MakeAssetLoader<r2::asset::MeshAssetLoader>();
		newModelSystem->mModelCache->RegisterAssetLoader(meshLoader);

		r2::asset::ModelAssetLoader* modelLoader = (r2::asset::ModelAssetLoader*)newModelSystem->mModelCache->MakeAssetLoader<r2::asset::ModelAssetLoader>();
		modelLoader->SetAssetCache(newModelSystem->mModelCache);

		newModelSystem->mModelCache->RegisterAssetLoader(modelLoader);

		//r2::asset::RModelAssetLoader* rmodelLoader = (r2::asset::RModelAssetLoader*)newModelSystem->mModelCache->MakeAssetLoader<r2::asset::RModelAssetLoader>();
		//newModelSystem->mModelCache->RegisterAssetLoader(rmodelLoader);

		return newModelSystem;
	}

	void Shutdown(ModelCache* system)
	{
		if (!system)
		{
			R2_CHECK(false, "Trying to Shutdown a null model system");
			return;
		}

		r2::mem::LinearArena* arena = system->mSubAreaArena;
		//return all of the models back to the cache
		r2::draw::modlche::FlushAll(system);

		FREE(system->mMeshes, *arena);
		FREE(system->mModels, *arena);

		r2::asset::lib::DestroyCache(system->mModelCache);
		FREE(system->mAssetBoundary.location, *arena);

		FREE(system, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes)
	{
		u32 boundsChecking = 0;
#if defined(R2_DEBUG) || defined(R2_RELEASE)
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ModelCache), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::asset::AssetCacheRecord>::MemorySize(numAssets * 2), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::asset::AssetCacheRecord>::MemorySize(numAssets * 2), ALIGNMENT, headerSize, boundsChecking) +
			r2::asset::AssetCache::TotalMemoryNeeded(numAssets, assetCapacityInBytes, ALIGNMENT);
	}

	bool HasMesh(ModelCache* system, const r2::asset::Asset& mesh)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return false;
		}

		if (!system->mModelCache || !system->mMeshes)
		{
			R2_CHECK(false, "We haven't initialized the system!");
			return false;
		}

		return  r2::shashmap::Has(*system->mMeshes, mesh.HashID());
	}

	MeshHandle GetMeshHandle(ModelCache* system, const r2::asset::Asset& mesh)
	{
		if (HasMesh(system, mesh))
		{
			return { mesh.HashID(), static_cast<s64>(system->mModelCache->GetSlot()) };
		}

		return MeshHandle{};
	}

	MeshHandle LoadMesh(ModelCache* system, const r2::asset::Asset& mesh)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return {};
		}

		return system->mModelCache->LoadAsset(mesh);
	}

	const Mesh* GetMesh(ModelCache* system, const MeshHandle& handle)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return nullptr;
		}

		if (handle.assetCache != system->mModelCache->GetSlot())
		{
			R2_CHECK(false, "Trying to get a model that doesn't exist in this model system");
			return nullptr;
		}

		r2::asset::AssetCacheRecord record;
		if (system->mCacheModelReferences)
		{
			if (!r2::shashmap::Has(*system->mMeshes, handle.handle))
			{
				record = system->mModelCache->GetAssetBuffer(handle);
				r2::shashmap::Set(*system->mMeshes, handle.handle, record);
			}
			else
			{
				r2::asset::AssetCacheRecord defaultRecord;
				record = r2::shashmap::Get(*system->mMeshes, handle.handle, defaultRecord);

				R2_CHECK(record.GetAssetBuffer() != defaultRecord.GetAssetBuffer(), "We couldn't get the record!");
			}
		}
		else
		{
			record = system->mModelCache->GetAssetBuffer(handle);

			if (!r2::shashmap::Has(*system->mMeshes, handle.handle))
			{
				r2::shashmap::Set(*system->mMeshes, handle.handle, record);
			}
		}

		r2::draw::Mesh* mesh = (r2::draw::Mesh*)record.GetAssetBuffer()->MutableData();

		mesh->assetName = handle.handle;

		return mesh;
	}

	void ReturnMesh(ModelCache* system, const Mesh* mesh)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return;
		}

		if (!mesh)
		{
			R2_CHECK(false, "Passed in a null mesh");
			return;
		}

		r2::asset::AssetCacheRecord defaultRecord;

		r2::asset::AssetCacheRecord theRecord = r2::shashmap::Get(*system->mMeshes, mesh->assetName, defaultRecord);

		if (theRecord.GetAssetBuffer() == defaultRecord.GetAssetBuffer())
		{
			R2_CHECK(false, "Failed to get the asset cache record!");
			return;
		}

		system->mModelCache->ReturnAssetBuffer(theRecord);
	}

	ModelHandle LoadModel(ModelCache* system, const r2::asset::Asset& model)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return {};
		}

		return system->mModelCache->LoadAsset(model);
	}

	const Model* GetModel(ModelCache* system, const ModelHandle& handle)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return nullptr;
		}

		if (handle.assetCache != system->mModelCache->GetSlot())
		{
			R2_CHECK(false, "Trying to get a model that doesn't exist in this model system");
			return nullptr;
		}

		r2::asset::AssetCacheRecord record;
		if (system->mCacheModelReferences)
		{
			if (!r2::shashmap::Has(*system->mModels, handle.handle))
			{
				record = system->mModelCache->GetAssetBuffer(handle);
				r2::shashmap::Set(*system->mModels, handle.handle, record);
			}
			else
			{
				r2::asset::AssetCacheRecord defaultRecord;
				record = r2::shashmap::Get(*system->mModels, handle.handle, defaultRecord);

				R2_CHECK(record.GetAssetBuffer() != defaultRecord.GetAssetBuffer(), "We couldn't get the record!");
			}
		}
		else
		{
			record = system->mModelCache->GetAssetBuffer(handle);

			if (!r2::shashmap::Has(*system->mModels, handle.handle))
			{
				r2::shashmap::Set(*system->mModels, handle.handle, record);
			}
		}

		r2::draw::Model* model = (r2::draw::Model*)record.GetAssetBuffer()->MutableData();

		model->assetName.hashID = handle.handle;

		return model;
	}

	void ReturnModel(ModelCache* system, const Model* model)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return;
		}

		if (!model)
		{
			R2_CHECK(false, "Passed in a null model");
			return;
		}

		r2::asset::AssetCacheRecord defaultRecord;

		r2::asset::AssetCacheRecord theRecord = r2::shashmap::Get(*system->mModels, model->assetName.hashID, defaultRecord);

		if (theRecord.GetAssetBuffer() == defaultRecord.GetAssetBuffer())
		{
			R2_CHECK(false, "Failed to get the asset cache record!");
			return;
		}

		system->mModelCache->ReturnAssetBuffer(theRecord);

		

	}

	void LoadModels(ModelCache* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<ModelHandle>& handles)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return;
		}

		u64 numAssetsToLoad = r2::sarr::Size(assets);

		for (u64 i = 0; i < numAssetsToLoad; ++i)
		{
			auto modelHandle = system->mModelCache->LoadAsset(r2::sarr::At(assets, i));
			r2::sarr::Push(handles, modelHandle);
		}
	}

	void LoadMeshes(ModelCache* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<MeshHandle>& handles)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return;
		}

		u64 numAssetsToLoad = r2::sarr::Size(assets);

		for (u64 i = 0; i < numAssetsToLoad; ++i)
		{
			auto modelHandle = system->mModelCache->LoadAsset(r2::sarr::At(assets, i));
			r2::sarr::Push(handles, modelHandle);
		}
	}

	void FlushAll(ModelCache* system)
	{
		if (system)
		{
			auto beginMeshHash = r2::shashmap::Begin(*system->mMeshes);

			auto meshIter = beginMeshHash;

			for (; meshIter != r2::shashmap::End(*system->mMeshes); ++meshIter)
			{
				r2::asset::AssetCacheRecord defaultCacheRecord;
				r2::asset::AssetCacheRecord result = r2::shashmap::Get(*system->mMeshes, meshIter->key, defaultCacheRecord);

				if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
				{
					system->mModelCache->ReturnAssetBuffer(meshIter->value);
				}
			}

			auto beginHash = r2::shashmap::Begin(*system->mModels);

			auto iter = beginHash;
			for (; iter != r2::shashmap::End(*system->mModels); ++iter)
			{
				r2::asset::AssetCacheRecord defaultCacheRecord;
				r2::asset::AssetCacheRecord result = r2::shashmap::Get(*system->mModels, iter->key, defaultCacheRecord);

				if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
				{
					system->mModelCache->ReturnAssetBuffer(iter->value);
				}
			}

			r2::shashmap::Clear(*system->mMeshes);
			r2::shashmap::Clear(*system->mModels);

			system->mModelCache->FlushAll();
		}
	}
}