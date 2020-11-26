#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssimpAssetLoader.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Model/Model.h"
#include "r2/Utils/Hash.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "r2/Core/Assets/AssimpHelpers.h"
#include "glm/gtx/string_cast.hpp"

namespace
{
	u64 GetTotalMeshBytes(aiNode* node, const aiScene* scene, u64& numMeshes, u64& numBones, u64& numVertices, u64 alignment, u32 header, u32 boundsChecking)
	{
		u64 bytes = 0;
		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			bytes += r2::draw::Skeleton::MemorySizeNoData(node->mNumChildren, alignment, header, boundsChecking);
			bytes += GetTotalMeshBytes(node->mChildren[i], scene, numMeshes, numBones, numVertices, alignment, header, boundsChecking);
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

			numVertices += mesh->mNumVertices;

			bytes += r2::draw::Mesh::MemorySize(mesh->mNumVertices, mesh->mNumFaces * 3, alignment, header, boundsChecking);
		
			if (mesh->mNumBones > 0)
			{
				numBones += mesh->mNumBones;
			}
		
		}

		return bytes;
	}




	void ProcessMesh(r2::draw::Model& model, aiMesh* mesh, const aiNode* node, const aiScene* scene, void** dataPtr)
	{
		//r2::draw::Mesh nextMesh;


		r2::draw::Mesh* nextMeshPtr = new (*dataPtr) r2::draw::Mesh();

		*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, sizeof(r2::draw::Mesh));

	//	printf("Mesh: %s\n", mesh->mName.C_Str());
		
		glm::mat4 localTransform = AssimpMat4ToGLMMat4(node->mTransformation);
		const aiNode* nextNode = node->mParent;

		while (nextNode)
		{
			localTransform = AssimpMat4ToGLMMat4(nextNode->mTransformation) * localTransform;
			nextNode = nextNode->mParent;
		}

		nextMeshPtr->optrVertices = EMPLACE_SARRAY(*dataPtr, r2::draw::Vertex, mesh->mNumVertices);

		*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::Vertex>::MemorySize(mesh->mNumVertices));

		if (mesh->mNumFaces > 0)
		{
			nextMeshPtr->optrIndices = EMPLACE_SARRAY(*dataPtr, u32, mesh->mNumFaces * 3);

			*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<u32>::MemorySize(mesh->mNumFaces * 3));
		}

		s32 textureIndex = 0;
		if (mesh->mMaterialIndex >= 0)
		{
			//nextMeshPtr->optrMaterials = EMPLACE_SARRAY(*dataPtr, r2::draw::MaterialHandle, 1);
			//*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(1));
		
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

					char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
					char textureName[r2::fs::FILE_PATH_LENGTH];
					r2::fs::utils::SanitizeSubPath(str.C_Str(), sanitizedPath);

					r2::fs::utils::CopyFileNameWithExtension(sanitizedPath, textureName);

					r2::draw::tex::Texture tex;
					auto materialHandle = r2::draw::matsys::FindMaterialFromTextureName(textureName, tex);

					if (!r2::draw::mat::IsInvalidHandle(materialHandle))
					{
						r2::sarr::Push(*model.optrMaterialHandles, materialHandle);

						r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(materialHandle.slot);
						R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

						const r2::SArray<r2::draw::tex::Texture>* textures = r2::draw::mat::GetTexturesForMaterial(*matSystem, materialHandle);

						u64 numTextures = r2::sarr::Size(*textures);
						for (u64 i = 0; i < numTextures; ++i)
						{
							const r2::draw::tex::Texture& nextTexture = r2::sarr::At(*textures, i);

							if (nextTexture.textureAssetHandle.handle == tex.textureAssetHandle.handle &&
								nextTexture.textureAssetHandle.assetCache == tex.textureAssetHandle.assetCache)
							{
								textureIndex = i;
								break;
							}
						}
					}
					else
					{
						R2_LOGW("Failed to load the material texture: %s\n", str.C_Str());
					}
					//R2_CHECK(!, "We have an invalid handle!");

				}
			}
		}


		for (u32 i = 0; i < mesh->mNumVertices; ++i)
		{
			r2::draw::Vertex nextVertex;

			nextVertex.position = localTransform * glm::vec4(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), 1.0);

			if (mesh->mNormals)
			{
				nextVertex.normal = glm::mat3(glm::inverse(glm::transpose(localTransform))) * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			if (mesh->mTextureCoords)
			{
				nextVertex.texCoords = glm::vec3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, textureIndex);
			}

			r2::sarr::Push(*nextMeshPtr->optrVertices, nextVertex);
		}

		//u32 indexOffsetToUse = indexOffset;
		for (u32 i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (u32 j = 0; j < face.mNumIndices; ++j)
			{
				u32 nextIndex = face.mIndices[j];
			//	printf("face.mIndices[j] + indexOffsetToUse: %zu\n", nextIndex);
				r2::sarr::Push(*nextMeshPtr->optrIndices, nextIndex);
				
				//if (nextIndex > indexOffset)
				//{
				//	indexOffset = nextIndex;
				//}
			}
			
		}

//		indexOffset++;

		r2::sarr::Push(*model.optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
	}

	void ProcessBones(r2::draw::AnimModel& model, u32 baseVertex, const aiMesh* mesh, const aiNode* node, const aiScene* scene)
	{
		const aiMesh* meshToUse = mesh;
		//printf("Num bones: %zu\n", meshToUse->mNumBones);

		for (u32 i = 0; i < meshToUse->mNumBones; ++i)
		{
			s32 boneIndex = -1;
			std::string boneNameStr = std::string(meshToUse->mBones[i]->mName.data);
			//printf("boneNameStr name: %s\n", boneNameStr.c_str());

			u64 boneName = STRING_ID(meshToUse->mBones[i]->mName.C_Str());

			s32 theDefault = -1;
			s32 result = r2::shashmap::Get(*model.boneMapping, boneName, theDefault); 

			if (result == theDefault)
			{
				boneIndex = (u32)r2::sarr::Size(*model.boneInfo);
				r2::draw::BoneInfo info;
				info.offsetTransform = AssimpMat4ToGLMMat4(meshToUse->mBones[i]->mOffsetMatrix);
				r2::sarr::Push(*model.boneInfo, info);

				r2::shashmap::Set(*model.boneMapping, boneName, boneIndex);
			}
			else
			{
				boneIndex = result;
			}

		//	printf("boneIndex: %i\n", boneIndex);

			size_t numWeights = meshToUse->mBones[i]->mNumWeights;

			for (u32 j = 0; j < numWeights; ++j)
			{
				u32 vertexID = baseVertex + meshToUse->mBones[i]->mWeights[j].mVertexId;

				float currentWeightTotal = 0.0f;
				for (u32 k = 0; k < MAX_BONE_WEIGHTS; ++k)
				{
					currentWeightTotal += r2::sarr::At(*model.boneData, vertexID).boneWeights[k];

					if (!r2::math::NearEq(currentWeightTotal, 1.0f) && r2::math::NearEq(r2::sarr::At(*model.boneData, vertexID).boneWeights[k], 0.0f))
					{
						r2::sarr::At(*model.boneData, vertexID).boneIDs[k] = boneIndex;
						r2::sarr::At(*model.boneData, vertexID).boneWeights[k] = meshToUse->mBones[i]->mWeights[j].mWeight;
						break;
					}
				}
			}
		}

		if (meshToUse->mNumBones == 0)
		{
			//manually find the bone that goes with this guy
			const aiNode* nodeToUse = node->mParent;

			while (nodeToUse)
			{
				u64 boneName = STRING_ID(nodeToUse->mName.C_Str());
				s32 theDefault = -1;
				s32 boneIndex = r2::shashmap::Get(*model.boneMapping, boneName, theDefault);
				if (boneIndex != theDefault)
				{
					for (u64 i = 0; i < meshToUse->mNumVertices; ++i)
					{
						u32 vertexID = baseVertex + i;

						r2::sarr::At(*model.boneData, vertexID).boneIDs[0] = boneIndex;
						r2::sarr::At(*model.boneData, vertexID).boneWeights[0] = 1.0;
					}

					break;
				}
				else
				{
					nodeToUse = nodeToUse->mParent;
				}
			}
		}
	}

	void ProcessMeshForModel(r2::draw::Model& model, aiMesh* mesh,const aiNode* node, const aiScene* scene, void** dataPtr)
	{
		ProcessMesh(model, mesh, node, scene, dataPtr);
	}

	void ProcessMeshForAnimModel(r2::draw::AnimModel& model, aiMesh* mesh, const aiNode* node, const aiScene* scene, void** dataPtr)
	{
		ProcessMesh(model.model, mesh, node, scene, dataPtr);
	}

	void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene, void** dataPtr)
	{
		//process all of the node's meshes
		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForModel(model, mesh, node, scene, dataPtr);
		}

		//then do the same for each of its children
		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNode(model, node->mChildren[i], scene, dataPtr);
		}
	}

	void ProcessAnimNode(r2::draw::AnimModel& model, aiNode* node, const aiScene* scene, r2::draw::Skeleton& skeleton, void** dataPtr, u32& numVertices)
	{
		//if (node->mMetaData)
		//{
		//	printf("Node: %s's metadata\n", node->mName.C_Str());
		//	for (u32 i = 0; i < node->mMetaData->mNumProperties; ++i)
		//	{
		//		printf("Property: %s\n", node->mMetaData->mKeys[i].C_Str());
		//	}

		//}

		//printf("node: %s, transform: %s\n", node->mName.C_Str(), glm::to_string(AssimpMat4ToGLMMat4(node->mTransformation)).c_str());

		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForAnimModel(model, mesh, node, scene, dataPtr);

			ProcessBones(model, numVertices, mesh, node, scene);

			numVertices += mesh->mNumVertices;
		}

		if (node->mNumChildren > 0)
		{
			skeleton.children = EMPLACE_SARRAY(*dataPtr, r2::draw::Skeleton, node->mNumChildren);
			*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::Skeleton>::MemorySize(node->mNumChildren));
		}

		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			r2::draw::Skeleton child;

			child.boneName = node->mChildren[i]->mName.C_Str();

		//	printf("skeleton part name: %s\n", child.boneName.c_str());
			child.hashName = STRING_ID(child.boneName.c_str());
			child.transform = AssimpMat4ToGLMMat4(node->mChildren[i]->mTransformation) ;

			child.parent = &skeleton;
			r2::sarr::Push(*skeleton.children, child);

			ProcessAnimNode(model, node->mChildren[i], scene, r2::sarr::Last(*skeleton.children), dataPtr, numVertices);
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
		return ASSIMP_MODEL;
	}

	bool AssimpAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 AssimpAssetLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		Assimp::Importer import;
	//	import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);
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
		mNumBones = 0;
		mNumVertices = 0;

		u64 totalSizeInBytes = GetTotalMeshBytes(scene->mRootNode, scene, mNumMeshes, mNumBones, mNumVertices, alignment, header, boundsChecking);


		import.FreeScene();

		if (mNumBones > 0)
		{
			return totalSizeInBytes + r2::draw::AnimModel::MemorySizeNoData(mNumBones * r2::SHashMap<u32>::LoadFactorMultiplier(), mNumVertices, mNumBones, mNumMeshes, alignment, header, boundsChecking);
		}

		return totalSizeInBytes + r2::draw::Model::ModelMemorySize(mNumMeshes, alignment, header, boundsChecking);
	}

	bool AssimpAssetLoader::LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		Assimp::Importer import;
		//import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);
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
		
		void* startOfArrayPtr = nullptr;

		bool isAnimated = mNumBones > 0;
		u32 indexOffset = 0;
		if (isAnimated) 
		{
			r2::draw::AnimModel* model = new (dataPtr) r2::draw::AnimModel();

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::AnimModel));

			model->model.optrMaterialHandles = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(mNumMeshes));

			model->model.optrMeshes = EMPLACE_SARRAY(startOfArrayPtr,const r2::draw::Mesh*, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(mNumMeshes));

			model->boneData = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneData, mNumVertices);

			model->boneData->mSize = mNumVertices;

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneData>::MemorySize(mNumVertices));

			model->boneInfo = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneInfo, mNumBones);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneInfo>::MemorySize(mNumBones));

			u64 hashCapacity = static_cast<u64>(std::round( mNumBones * r2::SHashMap<u32>::LoadFactorMultiplier() ) );

			model->boneMapping = MAKE_SHASHMAP_IN_PLACE(s32, startOfArrayPtr, hashCapacity);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<u32>::MemorySize(hashCapacity));

			model->model.globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

			//Process the Nodes
			u32 numVertices = 0;
			model->skeleton.hashName = STRING_ID(scene->mRootNode->mName.C_Str());
			model->skeleton.boneName = scene->mRootNode->mName.C_Str();
			model->skeleton.transform = AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation);
			model->skeleton.parent = nullptr;
			
			ProcessAnimNode(*model, scene->mRootNode, scene, model->skeleton, &startOfArrayPtr, numVertices);

		}
		else
		{
			r2::draw::Model* model = new (dataPtr) r2::draw::Model();

			R2_CHECK(model != nullptr, "We should have a proper model!");

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

			model->optrMaterialHandles = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(mNumMeshes));

			model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr,const r2::draw::Mesh*, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(mNumMeshes));

			model->globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

			ProcessNode(*model, scene->mRootNode, scene, &startOfArrayPtr);
		}
		
		import.FreeScene();

		return true;
	}
}

