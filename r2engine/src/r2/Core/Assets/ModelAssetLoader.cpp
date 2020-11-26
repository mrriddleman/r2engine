#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/ModelAssetLoader.h"

#include "r2/Render/Model/Model_generated.h"
#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelSystem.h"
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

	u64 ModelAssetLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
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

		totalMemorySize += r2::draw::Model::ModelMemorySize(numMeshes, alignment, header, boundsChecking);

		return totalMemorySize;
	}

	bool ModelAssetLoader::LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
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
			r2::draw::MeshHandle meshHandle = r2::draw::modlsys::GetMeshHandle(mnoptrModelSystem, meshAsset);

			if (meshHandle.handle == r2::asset::INVALID_ASSET_HANDLE || meshHandle.assetCache == r2::asset::INVALID_ASSET_CACHE)
			{
				meshHandle = r2::draw::modlsys::LoadMesh(mnoptrModelSystem, meshAsset);
			}

			R2_CHECK(meshHandle.handle != r2::asset::INVALID_ASSET_HANDLE, "We should have a valid mesh handle here!");

			r2::sarr::Push(*model->optrMeshes, r2::draw::modlsys::GetMesh(mnoptrModelSystem, meshHandle));
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
		


		//startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(numMeshes));

		//for (flatbuffers::uoffset_t i = 0; i < numMeshes; i++)
		//{
		//	const u64 numVertices = flatModel->meshes()->Get(i)->numVertices();
		//	const u64 numIndices = flatModel->meshes()->Get(i)->numFaces() * 3; //* 3 for indices
		//	const u64 numMaterials = flatModel->meshes()->Get(i)->materials()->size();
		//
		//	r2::draw::Mesh nextMesh;

		//	nextMesh.optrVertices = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Vertex, numVertices);
		//	
		//	startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Vertex>::MemorySize(numVertices));

		//	if (numIndices > 0)
		//	{
		//		nextMesh.optrIndices = EMPLACE_SARRAY(startOfArrayPtr, u32, numIndices);

		//		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<u32>::MemorySize(numIndices));
		//	}

		//	if (numMaterials)
		//	{
		//		nextMesh.optrMaterials = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, numMaterials);
		//		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(numMaterials));
		//	}

		//	for (flatbuffers::uoffset_t v = 0; v < numVertices; ++v)
		//	{
		//		r2::draw::Vertex nextVertex;

		//		const auto* positions = flatModel->meshes()->Get(i)->positions();
		//		const auto* normals = flatModel->meshes()->Get(i)->normals();
		//		const auto* texCoords = flatModel->meshes()->Get(i)->textureCoords();

		//		R2_CHECK(positions != nullptr, "We should have positions always!");

		//		nextVertex.position = glm::vec3(positions->Get(v)->x(), positions->Get(v)->y(), positions->Get(v)->z());

		//		if (normals)
		//		{
		//			nextVertex.normal = glm::vec3(normals->Get(v)->x(), normals->Get(v)->y(), normals->Get(v)->z());
		//		}

		//		if (texCoords)
		//		{
		//			nextVertex.texCoords = glm::vec3(texCoords->Get(v)->x(), texCoords->Get(v)->y(), 0);
		//		}

		//		r2::sarr::Push(*nextMesh.optrVertices, nextVertex);
		//	}

		//	const auto numFaces = flatModel->meshes()->Get(i)->numFaces();

		//	for (flatbuffers::uoffset_t f = 0; f < numFaces; ++f)
		//	{
		//		for (flatbuffers::uoffset_t j = 0; j < flatModel->meshes()->Get(i)->faces()->Get(f)->numIndices(); ++j)
		//		{
		//			r2::sarr::Push(*nextMesh.optrIndices, flatModel->meshes()->Get(i)->faces()->Get(f)->indices()->Get(j));
		//		}
		//	}

		//	for (flatbuffers::uoffset_t m = 0; m < numMaterials; ++m)
		//	{
		//		//@NOTE: this assumes that the materials are already loaded so that needs to happen first

		//		u64 materialName = flatModel->meshes()->Get(i)->materials()->Get(m)->name();

		//		//@Temporary: This is really slow since we need to search all of the material systems in order to find the material
		//		//		 I think instead we shouldn't have models reference materials, we should do this some other way
		//		auto materialHandle = r2::draw::matsys::FindMaterialHandle(materialName);

		//		R2_CHECK(!r2::draw::mat::IsInvalidHandle(materialHandle), "We have an invalid handle!");

		//		r2::sarr::Push(*nextMesh.optrMaterials, materialHandle);
		//	}

		//	r2::sarr::Push(*model->optrMeshes, nextMesh);
		//}

		return model != nullptr;
	}
	
}