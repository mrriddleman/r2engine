#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"

#include "r2/Render/Model/Model_generated.h"
#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelCache.h"
#include "r2/Utils/Hash.h"

namespace r2::asset
{
	const char* ModelAssetLoader::GetPattern()
	{
		return  flat::ModelExtension();
	}

	AssetType ModelAssetLoader::GetType() const
	{
		return MODEL;
	}

	bool ModelAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 ModelAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		if (!rawBuffer)
		{
			R2_CHECK(false, "Got passed in a nullptr for the rawBuffer!");
			return 0;
		}
		//here we do the memory sizing
		const flat::Model* flatModel = flat::GetModel(rawBuffer);

		if (!flatModel)
		{
			R2_CHECK(flatModel != nullptr, "We couldn't get the flat buffer model!");
			return 0;
		}

		const auto numMeshes = flatModel->meshNames()->size();
	
		u64 totalMemorySize = 0;

		totalMemorySize += r2::draw::Model::ModelMemorySize(numMeshes, numMeshes, alignment, header, boundsChecking);

		return totalMemorySize;
	}

	bool ModelAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		if (!rawBuffer)
		{
			R2_CHECK(false, "Got passed in a nullptr for the rawBuffer!");
			return false;
		}

		R2_CHECK(mnoptrModelSystem != nullptr, "We should have a model system for this to work!");

		//here we do the memory sizing
		const flat::Model* flatModel = flat::GetModel(rawBuffer);

		if (!flatModel)
		{
			R2_CHECK(flatModel != nullptr, "We couldn't get the flat buffer model!");
			return false;
		}

		const auto numMeshes = flatModel->meshNames()->size();
		const auto numMaterialNames = flatModel->materials()->size();

		void* dataPtr = assetBuffer.MutableData();
		r2::draw::Model* model = new (dataPtr) r2::draw::Model();

		void* startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

		model->hash = flatModel->name();
		model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<u64>::MemorySize(numMeshes));

		model->optrMaterialHandles = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, numMeshes);
		
		for (flatbuffers::uoffset_t i = 0; i < numMeshes; ++i)
		{	
			r2::asset::Asset meshAsset{ flatModel->meshNames()->Get(i)->c_str(), r2::asset::MESH };
			r2::draw::MeshHandle meshHandle = r2::draw::modlche::GetMeshHandle(mnoptrModelSystem, meshAsset);

			if (meshHandle.handle == r2::asset::INVALID_ASSET_HANDLE || meshHandle.assetCache == r2::asset::INVALID_ASSET_CACHE)
			{
				meshHandle = r2::draw::modlche::LoadMesh(mnoptrModelSystem, meshAsset);
			}

			R2_CHECK(meshHandle.handle != r2::asset::INVALID_ASSET_HANDLE, "We should have a valid mesh handle here!");

			r2::sarr::Push(*model->optrMeshes, r2::draw::modlche::GetMesh(mnoptrModelSystem, meshHandle));
		}

		//@TODO(Serge): Speed this up and use a faster way
		for (flatbuffers::uoffset_t i = 0; i < numMaterialNames; ++i)
		{
			auto materialHandle = r2::draw::matsys::FindMaterialHandle(flatModel->materials()->Get(i)->name());

			r2::sarr::Push(*model->optrMaterialHandles, materialHandle);
		}

		auto numMaterialsToAdd = numMeshes - numMaterialNames;

		if (numMaterialsToAdd > 0)
		{
			auto materialHandle = r2::draw::matsys::FindMaterialHandle(STRING_ID("Default"));

			for (u64 i = numMaterialNames; i < numMeshes; ++i)
			{
				r2::sarr::Push(*model->optrMaterialHandles, materialHandle);
			}
		}

		return model != nullptr;
	}
	
}