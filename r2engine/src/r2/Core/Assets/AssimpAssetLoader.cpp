#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssimpAssetLoader.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Render/Model/Model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace
{
	glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat)
	{
		//result[col][row]
		//mat[row][col] a, b, c, d - rows
		//              1, 2, 3, 4 - cols
		glm::mat4 result = glm::mat4();

		result[0][0] = mat.a1; result[1][0] = mat.a2; result[2][0] = mat.a3; result[3][0] = mat.a4;
		result[0][1] = mat.b1; result[1][1] = mat.b2; result[2][1] = mat.b3; result[3][1] = mat.b4;
		result[0][2] = mat.c1; result[1][2] = mat.c2; result[2][2] = mat.c3; result[3][2] = mat.c4;
		result[0][3] = mat.d1; result[1][3] = mat.d2; result[2][3] = mat.d3; result[3][3] = mat.d4;

		return result;
	}

	glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D)
	{
		return glm::vec3(vec3D.x, vec3D.y, vec3D.z);
	}

	glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat)
	{
		return glm::quat(quat.w, quat.x, quat.y, quat.z);
	}

	u64 GetTotalBytes(aiNode* node, const aiScene* scene, u64& numMeshes, u64 alignment, u32 header, u32 boundsChecking)
	{
		u64 bytes = 0;
		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			bytes += GetTotalBytes(node->mChildren[i], scene, numMeshes, alignment, header, boundsChecking);
		}

		numMeshes += node->mNumMeshes;

		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			//@Temporary
			u64 numMaterials = 0;
			if (mesh->mMaterialIndex >= 0)
			{
				numMaterials = 1;
			}

			bytes += r2::draw::Mesh::MemorySize(mesh->mNumVertices, mesh->mNumFaces * 3, numMaterials, alignment, header, boundsChecking);
		}

		return bytes;
	}

	void ProcessMesh(r2::draw::Model& model, aiMesh* mesh, const aiScene* scene, void** dataPtr, const char* directory)
	{
		r2::draw::Mesh nextMesh;

		u32 indexOffset = 0;
		
		u64 numMeshes = r2::sarr::Size(*model.optrMeshes);

		for ( u64 i = 0; i < numMeshes; ++i)
		{
			indexOffset += r2::sarr::Size(*r2::sarr::At(*model.optrMeshes, i).optrIndices);
		}

		nextMesh.optrVertices = EMPLACE_SARRAY(*dataPtr, r2::draw::Vertex, mesh->mNumVertices);

		*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::Vertex>::MemorySize(mesh->mNumVertices));

		if (mesh->mNumFaces > 0)
		{
			nextMesh.optrIndices = EMPLACE_SARRAY(*dataPtr, u32, mesh->mNumFaces * 3);

			*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<u32>::MemorySize(mesh->mNumFaces * 3));
		}

		if (mesh->mMaterialIndex >= 0)
		{
			nextMesh.optrMaterials = EMPLACE_SARRAY(*dataPtr, r2::draw::MaterialHandle, 1);
			*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(1));
		}

		for (u32 i = 0; i < mesh->mNumVertices; ++i)
		{
			r2::draw::Vertex nextVertex;

			nextVertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			if (mesh->mNormals)
			{
				nextVertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			if (mesh->mTextureCoords)
			{
				nextVertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}

			r2::sarr::Push(*nextMesh.optrVertices, nextVertex);
		}

		for (u32 i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (u32 j = 0; j < face.mNumIndices; ++j)
			{
				r2::sarr::Push(*nextMesh.optrIndices, face.mIndices[j] + indexOffset);
			}
		}

		if (mesh->mMaterialIndex >= 0)
		{
			//@TODO(Serge): hmm we somehow need to map the assimp textures to our own material handle
			//we'd have to find the texture in the material system and return the handle from that...

			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			R2_CHECK(material != nullptr, "Material is null!");
			if (material)
			{
				const u32 numTextures = aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);
				if (numTextures > 0)
				{
					aiString str;
					material->GetTexture(aiTextureType_DIFFUSE, 0, &str);

					auto materialHandle = r2::draw::matsys::FindMaterialFromTextureName(str.C_Str());

					R2_CHECK(!r2::draw::mat::IsInvalidHandle(materialHandle), "We have an invalid handle!");

					r2::sarr::Push(*nextMesh.optrMaterials, materialHandle);
				}
			}
		}

		r2::sarr::Push(*model.optrMeshes, nextMesh);

	}

	void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene, void** dataPtr, const char* directory)
	{
		//process all of the node's meshes
		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMesh(model, mesh, scene, dataPtr, directory);
		}

		//then do the same for each of its children
		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNode(model, node->mChildren[i], scene, dataPtr, directory);
		}
	}
}

namespace r2::asset
{
	const char* AssimpAssetLoader::GetPattern()
	{
		return "*";
	}

	AssetType AssimpAssetLoader::GetType() const
	{
		return ASSIMP;
	}

	bool AssimpAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 AssimpAssetLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		Assimp::Importer import;

		const aiScene* scene = import.ReadFileFromMemory(rawBuffer, size, aiProcess_Triangulate |
			// aiProcess_SortByPType | // ?
			aiProcess_GenSmoothNormals |
			aiProcess_ImproveCacheLocality |
			aiProcess_RemoveRedundantMaterials |
			 aiProcess_FindDegenerates |
			 aiProcess_GenUVCoords |
			// aiProcess_TransformUVCoords |
			// aiProcess_FindInstances |
			aiProcess_CalcTangentSpace |
			aiProcess_LimitBoneWeights |
			aiProcess_OptimizeMeshes |
			aiProcess_SplitByBoneCount);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
			return 0;
		}

		aiNode* node = scene->mRootNode;
		mNumMeshes = 0;
		u64 totalSizeInBytes = GetTotalBytes(scene->mRootNode, scene, mNumMeshes, alignment, header, boundsChecking);

		import.FreeScene();

		return totalSizeInBytes + r2::draw::Model::ModelMemorySize(mNumMeshes, alignment, header, boundsChecking);
	}

	bool AssimpAssetLoader::LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		Assimp::Importer import;

		const aiScene* scene = import.ReadFileFromMemory(rawBuffer, rawSize, aiProcess_Triangulate |
			// aiProcess_SortByPType | // ?
			aiProcess_GenSmoothNormals |
			aiProcess_ImproveCacheLocality |
			aiProcess_RemoveRedundantMaterials |
			 aiProcess_FindDegenerates |
			 aiProcess_GenUVCoords |
			 //aiProcess_TransformUVCoords |
			// aiProcess_FindInstances |
			aiProcess_CalcTangentSpace |
			aiProcess_LimitBoneWeights |
			aiProcess_OptimizeMeshes |
			aiProcess_SplitByBoneCount);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
			return false;
		}

		void* dataPtr = assetBuffer.MutableData();
		r2::draw::Model* model = new (dataPtr) r2::draw::Model();

		void* startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

		model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Mesh, mNumMeshes);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(mNumMeshes));

		model->globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

		ProcessNode(*model, scene->mRootNode, scene, &startOfArrayPtr, "");

		import.FreeScene();

		return true;
	}
}

