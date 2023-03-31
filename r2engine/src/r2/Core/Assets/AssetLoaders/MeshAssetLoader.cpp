#include "r2pch.h"

#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLoaders/MeshAssetLoader.h"
#include "r2/Render/Model/Mesh_generated.h"
#include "r2/Render//Model/Mesh.h"

namespace r2::asset
{

	const char* MeshAssetLoader::GetPattern()
	{
		return flat::MeshExtension();
	}

	AssetType MeshAssetLoader::GetType() const
	{
		return MESH;
	}

	bool MeshAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 MeshAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		if (!rawBuffer)
		{
			R2_CHECK(false, "Got passed in a nullptr for the rawbuffer!");
			return 0;
		}

		const flat::Mesh* flatMesh = flat::GetMesh(rawBuffer);

		if (!flatMesh)
		{
			R2_CHECK(false, "We couldn't get the flatbuffer mesh");
			return 0;
		}

		u64 totalMemorySize = 0;

		const u64 numVertices = flatMesh->numVertices();
		const u64 numIndices = flatMesh->numFaces() * 3;
		

		//@TODO(Serge): I think we're allocating too much memory here - we only do 1 allocation for the buffer but we're acting like there are multiple here
		totalMemorySize += r2::draw::Mesh::MemorySize(numVertices, numIndices, alignment, header, boundsChecking);

		return totalMemorySize;
	}

	bool MeshAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		if (!rawBuffer)
		{
			R2_CHECK(false, "Got passed in a nullptr for the rawbuffer!");
			return 0;
		}

		const flat::Mesh* flatMesh = flat::GetMesh(rawBuffer);

		if (!flatMesh)
		{
			R2_CHECK(false, "We couldn't get the flatbuffer mesh");
			return 0;
		}

		void* dataPtr = assetBuffer.MutableData();
		r2::draw::Mesh* mesh = new (dataPtr) r2::draw::Mesh();

		mesh->hashName = flatMesh->name();

		void* startOfData = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Mesh));

		mesh->optrVertices = EMPLACE_SARRAY(startOfData, r2::draw::Vertex, flatMesh->numVertices());

		startOfData = r2::mem::utils::PointerAdd(startOfData, r2::SArray<r2::draw::Vertex>::MemorySize(flatMesh->numVertices()));

		mesh->optrIndices = EMPLACE_SARRAY(startOfData, u32, flatMesh->numFaces() * 3);

		startOfData = r2::mem::utils::PointerAdd(startOfData, r2::SArray<u32>::MemorySize(flatMesh->numFaces() * 3));

		for (flatbuffers::uoffset_t v = 0; v < flatMesh->numVertices(); ++v)
		{
			r2::draw::Vertex nextVertex;

			const auto* positions = flatMesh->positions();
			const auto* normals = flatMesh->normals();
			const auto* texCoords = flatMesh->textureCoords();
			const auto* tangents = flatMesh->tangents();


			R2_CHECK(positions != nullptr, "We should always have positions!");

			nextVertex.position = glm::vec3(positions->Get(v)->x(), positions->Get(v)->y(), positions->Get(v)->z());

			if (normals)
			{
				nextVertex.normal = glm::vec3(normals->Get(v)->x(), normals->Get(v)->y(), normals->Get(v)->z());
			}

			if (texCoords)
			{
				nextVertex.texCoords = glm::vec3(texCoords->Get(v)->x(), texCoords->Get(v)->y(), 0);
			}

			if (tangents)
			{
				
				nextVertex.tangent = glm::vec3(tangents->Get(v)->x(), tangents->Get(v)->y(), tangents->Get(v)->z());
			}

			//if (bitangents)
			//{
			//	nextVertex.bitangent = glm::vec3(bitangents->Get(v)->x(), bitangents->Get(v)->y(), bitangents->Get(v)->z());
			//}

			r2::sarr::Push(*mesh->optrVertices, nextVertex);
		}

		const auto numFaces = flatMesh->numFaces();

		for (flatbuffers::uoffset_t f = 0; f < numFaces; ++f)
		{
			for (flatbuffers::uoffset_t j = 0; j < flatMesh->faces()->Get(f)->numIndices(); ++j)
			{
				r2::sarr::Push(*mesh->optrIndices, flatMesh->faces()->Get(f)->indices()->Get(j));
			}
		}

		return mesh != nullptr;
	}

}
