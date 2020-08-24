#include "r2pch.h"

#include "r2/Render/Model/ModelSystem.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/ModelAssetLoader.h"
#include "r2/Core/Assets/AssimpAssetLoader.h"

namespace r2::draw::modlsys
{
	const u64 ALIGNMENT = 64;

	ModelSystem* Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, b32 cacheModelReferences, r2::asset::FileList files, const char* areaName)
	{
		if (!files)
		{
			R2_CHECK(false, "Trying to Init() a model system with no files!");
			return nullptr;
		}

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(r2::sarr::Size(*files), modelCacheSize);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enought space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes",
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

		ModelSystem* newModelSystem = ALLOC(ModelSystem, *modelArena);

		newModelSystem->mMemoryAreaHandle = memoryAreaHandle;
		newModelSystem->mSubAreaHandle = subAreaHandle;
		newModelSystem->mSubAreaArena = modelArena;
		newModelSystem->mModels = MAKE_SHASHMAP(*modelArena, r2::asset::AssetCacheRecord, r2::sarr::Size(*files) * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier());
		newModelSystem->mCacheModelReferences = cacheModelReferences;
		newModelSystem->mAssetBoundary = MAKE_BOUNDARY(*modelArena, modelCacheSize, ALIGNMENT);

		newModelSystem->mModelCache = r2::asset::lib::CreateAssetCache(newModelSystem->mAssetBoundary, files);

		r2::asset::ModelAssetLoader* modelLoader = (r2::asset::ModelAssetLoader*)newModelSystem->mModelCache->MakeAssetLoader<r2::asset::ModelAssetLoader>();
		newModelSystem->mModelCache->RegisterAssetLoader(modelLoader);

		r2::asset::AssimpAssetLoader* assimpModelLoader = (r2::asset::AssimpAssetLoader*)newModelSystem->mModelCache->MakeAssetLoader<r2::asset::AssimpAssetLoader>();
		newModelSystem->mModelCache->RegisterAssetLoader(assimpModelLoader);

		return newModelSystem;
	}

	void Shutdown(ModelSystem* system)
	{
		if (!system)
		{
			R2_CHECK(false, "Trying to Shutdown a null model system");
			return;
		}

		r2::mem::LinearArena* arena = system->mSubAreaArena;

		//return all of the models back to the cache

		auto beginHash = r2::shashmap::Begin(*system->mModels);

		auto iter = beginHash;
		for (; iter != r2::shashmap::End(*system->mModels); ++iter)
		{
			system->mModelCache->ReturnAssetBuffer(iter->value);
		}

		
		r2::asset::lib::DestroyCache(system->mModelCache);
		FREE(system->mAssetBoundary.location, *arena);

		FREE(system->mModels, *arena);

		FREE(system, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ModelSystem), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::asset::AssetCacheRecord>::MemorySize(numAssets * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier()), ALIGNMENT, headerSize, boundsChecking) +
			r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, numAssets, assetCapacityInBytes, ALIGNMENT);
	}

	ModelHandle LoadModel(ModelSystem* system, const r2::asset::Asset& model)
	{
		if (!system)
		{
			R2_CHECK(false, "Passed in a null model system");
			return {};
		}

		return system->mModelCache->LoadAsset(model);
	}

	const Model* GetModel(ModelSystem* system, const ModelHandle& handle)
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

				R2_CHECK(record.buffer != defaultRecord.buffer, "We couldn't get the record!");
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

		r2::draw::Model* model = (r2::draw::Model*)record.buffer->MutableData();

		model->hash = handle.handle;

		return model;
	}

	void ReturnModel(ModelSystem* system, const Model* model)
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

		r2::asset::AssetCacheRecord theRecord = r2::shashmap::Get(*system->mModels, model->hash, defaultRecord);

		if (theRecord.buffer == defaultRecord.buffer)
		{
			R2_CHECK(false, "Failed to get the asset cache record!");
			return;
		}

		system->mModelCache->ReturnAssetBuffer(theRecord);
	}

	void LoadModels(ModelSystem* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<ModelHandle>& handles)
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

	void FlushAll(ModelSystem* system)
	{
		if (system)
		{
			system->mModelCache->FlushAll();
		}
	}
}