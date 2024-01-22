#ifndef __MODEL_SYSTEM_H__
#define __MODEL_SYSTEM_H__

#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Render/Model/Model.h"

namespace r2::draw
{
	struct ModelCache
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::asset::AssetCache* mModelCache = nullptr;
		
		r2::SHashMap<r2::asset::AssetCacheRecord>* mMeshes = nullptr;

		r2::SHashMap<r2::asset::AssetCacheRecord>* mModels = nullptr;

		r2::mem::utils::MemBoundary mAssetBoundary;
		b32 mCacheModelReferences;
	};

	using ModelHandle = r2::asset::AssetHandle;
	using MeshHandle = r2::asset::AssetHandle;

	namespace modlche
	{
		ModelCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, b32 cacheModelReferences, const char* areaName);
		void Shutdown(ModelCache* system);
		u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes);

		bool HasMesh(ModelCache* system, const r2::asset::Asset& mesh);
		MeshHandle GetMeshHandle(ModelCache* system, const r2::asset::Asset& mesh);
		MeshHandle LoadMesh(ModelCache* system, const r2::asset::Asset& mesh);
		const Mesh* GetMesh(ModelCache* system, const MeshHandle& handle);

		void ReturnMesh(ModelCache* system, const Mesh* mesh);

		ModelHandle LoadModel(ModelCache* system, const r2::asset::Asset& model);
		const Model* GetModel(ModelCache* system, const ModelHandle& handle);
		bool HasModel(ModelCache* system, const ModelHandle& handle);

		void ReturnModel(ModelCache* system, const Model* model);
		void LoadModels(ModelCache* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<ModelHandle>& handles);
		void LoadMeshes(ModelCache* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<MeshHandle>& handles);
		void FlushAll(ModelCache* system);
	}
}


#endif // 


