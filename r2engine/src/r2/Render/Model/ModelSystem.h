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
	struct ModelSystem
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

	namespace modlsys
	{
		ModelSystem* Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, b32 cacheModelReferences, r2::asset::FileList files, const char* areaName);
		void Shutdown(ModelSystem* system);
		u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes);


		bool HasMesh(ModelSystem* system, const r2::asset::Asset& mesh);
		MeshHandle GetMeshHandle(ModelSystem* system, const r2::asset::Asset& mesh);
		MeshHandle LoadMesh(ModelSystem* system, const r2::asset::Asset& mesh);
		const Mesh* GetMesh(ModelSystem* system, const MeshHandle& handle);

		void ReturnMesh(ModelSystem* system, const Mesh* mesh);


		ModelHandle LoadModel(ModelSystem* system, const r2::asset::Asset& model);
		const Model* GetModel(ModelSystem* system, const ModelHandle& handle);
		const AnimModel* GetAnimModel(ModelSystem* system, const ModelHandle& handle);

		void ReturnModel(ModelSystem* system, const Model* model);
		void ReturnAnimModel(ModelSystem* system, const AnimModel* model);

		void LoadModels(ModelSystem* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<ModelHandle>& handles);
		void LoadMeshes(ModelSystem* system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<MeshHandle>& handles);
		void FlushAll(ModelSystem* system);
	}
}


#endif // 


