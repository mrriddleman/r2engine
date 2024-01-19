#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"

#include "r2/Render/Model/Model_generated.h"
#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Model.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Model/Materials/MaterialHelpers.h"
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

		R2_CHECK(mnoptrAssetCache != nullptr, "We should have a model system for this to work!");

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

		model->globalInverseTransform = glm::mat4(1);
		model->globalTransform = glm::mat4(1);
		model->assetName.hashID = flatModel->name();
		model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<const r2::draw::Mesh*>::MemorySize(numMeshes));

		model->optrMaterialNames = EMPLACE_SARRAY(startOfArrayPtr, r2::mat::MaterialName, numMaterialNames);
		
		for (flatbuffers::uoffset_t i = 0; i < numMeshes; ++i)
		{	
			r2::asset::Asset meshAsset{ flatModel->meshNames()->Get(i)->c_str(), r2::asset::MESH };
			r2::asset::AssetHandle meshHandle = mnoptrAssetCache->LoadAsset(meshAsset);

			R2_CHECK(meshHandle.handle != r2::asset::INVALID_ASSET_HANDLE, "We should have a valid mesh handle here!");

			r2::asset::AssetCacheRecord record = mnoptrAssetCache->GetAssetBuffer(meshHandle);
			r2::draw::Mesh* mesh = (r2::draw::Mesh*)record.GetAssetBuffer()->MutableData();
			mesh->assetName = meshHandle.handle;

			r2::sarr::Push(*model->optrMeshes, static_cast<const r2::draw::Mesh*>(mesh));

			//@TODO(Serge): This is sort of an issue since now it's possible that it could be freed since there's nothing holding a reference to it
			mnoptrAssetCache->ReturnAssetBuffer(record);
		}

		for (flatbuffers::uoffset_t i = 0; i < numMaterialNames; ++i)
		{
			const auto* flatMaterialName = flatModel->materials()->Get(i);
			R2_CHECK(flatMaterialName->materialPackName()->assetName() != 0, "The material pack name should never be nullptr");
			r2::sarr::Push(*model->optrMaterialNames, r2::mat::MakeMaterialNameFromFlatMaterial(flatMaterialName));
		}
		

		auto numMaterialsToAdd = numMeshes - numMaterialNames;

		if (numMaterialsToAdd > 0)
		{
			auto defaultMaterialName = STRING_ID("Default");
			char materialsPath[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN, materialsPath, "engine_material_pack.mpak");//"engine_material_params_pack.mppk");
			const auto materialPackName = r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

			for (u64 i = numMaterialNames; i < numMeshes; ++i)
			{
				r2::sarr::Push(*model->optrMaterialNames, { 
					{
						defaultMaterialName
#ifdef R2_ASSET_PIPELINE
						, "default"
#endif
					}
				, materialPackName });
			}
		}

		return model != nullptr;
	}

	bool ModelAssetLoader::FreeAsset(const AssetBuffer& assetBuffer)
	{
		return true;
	}


}