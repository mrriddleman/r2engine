#include "r2pch.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssimpAssetLoader.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Render/Model/Model.h"
#include "r2/Utils/Hash.h"
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

			bytes += r2::draw::Mesh::MemorySize(mesh->mNumVertices, mesh->mNumFaces * 3, numMaterials, alignment, header, boundsChecking);
		
			if (mesh->mNumBones > 0)
			{
				numBones += mesh->mNumBones;
			}
		
		}

		return bytes;
	}


	u64 GetTotalAnimationBytes(aiNode* node, const aiScene* scene, u64 alignment, u32 header, u32 boundsChecking)
	{
		u64 bytes = 0;

		for (u32 i = 0; i < scene->mNumAnimations; ++i)
		{
			aiAnimation* anim = scene->mAnimations[i];
			bytes += r2::draw::Animation::MemorySizeNoData(anim->mNumChannels, alignment, header, boundsChecking);
				
			for (u32 j = 0; j < anim->mNumChannels; ++j)
			{
				aiNodeAnim* animChannel = anim->mChannels[j];
				bytes += r2::draw::AnimationChannel::MemorySizeNoData(animChannel->mNumPositionKeys, animChannel->mNumScalingKeys, animChannel->mNumRotationKeys, alignment, header, boundsChecking);
			}
		}

		return bytes;
	}

	void ProcessMesh(r2::SArray<r2::draw::Mesh>* meshes,  aiMesh* mesh, const aiScene* scene, void** dataPtr, const char* directory)
	{
		r2::draw::Mesh nextMesh;

		u32 indexOffset = 0;

		u64 numMeshes = r2::sarr::Size(*meshes);

		for (u64 i = 0; i < numMeshes; ++i)
		{
			indexOffset += r2::sarr::Size(*r2::sarr::At(*meshes, i).optrIndices);
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

		r2::sarr::Push(*meshes, nextMesh);
	}

	void ProcessBones(r2::draw::AnimModel& model, u32 baseVertex, const aiMesh* mesh, const aiScene* scene)
	{
		for (u32 i = 0; i < mesh->mNumBones; ++i)
		{
			s32 boneIndex = 0;

			u64 boneName = STRING_ID(mesh->mBones[i]->mName.C_Str());

			s32 theDefault = -1;
			s32 result = r2::shashmap::Get(*model.boneMapping, boneName, theDefault); 

			if (result == theDefault)
			{
				boneIndex = (u32)r2::sarr::Size(*model.boneInfo);
				r2::draw::BoneInfo info;
				info.offsetTransform = AssimpMat4ToGLMMat4(mesh->mBones[i]->mOffsetMatrix);
				r2::sarr::Push(*model.boneInfo, info);

				r2::shashmap::Set(*model.boneMapping, boneName, boneIndex);
			}
			else
			{
				boneIndex = result;
			}

			size_t numWeights = mesh->mBones[i]->mNumWeights;

			for (u32 j = 0; j < numWeights; ++j)
			{
				u32 vertexID = baseVertex + mesh->mBones[i]->mWeights[j].mVertexId;

				bool found = false;

				float currentWeightTotal = 0.0f;
				for (u32 k = 0; k < MAX_BONE_WEIGHTS; ++k)
				{
					currentWeightTotal += r2::sarr::At(*model.boneData, vertexID).boneWeights[k];

					if (!r2::math::NearEq(currentWeightTotal, 1.0f) && r2::math::NearEq(r2::sarr::At(*model.boneData, vertexID).boneWeights[k], 0.0f))
					{
						r2::sarr::At(*model.boneData, vertexID).boneIDs[k] = boneIndex;
						r2::sarr::At(*model.boneData, vertexID).boneWeights[k] = mesh->mBones[i]->mWeights[j].mWeight;

						found = true;
						break;
					}
				}
			}
		}
	}

	void ProcessMeshForModel(r2::draw::Model& model, aiMesh* mesh, const aiScene* scene, void** dataPtr, const char* directory)
	{
		ProcessMesh(model.optrMeshes, mesh, scene, dataPtr, directory);
	}

	void ProcessMeshForAnimModel(r2::draw::AnimModel& model, aiMesh* mesh, const aiScene* scene, void** dataPtr, const char* directory)
	{
		ProcessMesh(model.meshes, mesh, scene, dataPtr, directory);
	}

	void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene, void** dataPtr, const char* directory)
	{
		//process all of the node's meshes
		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForModel(model, mesh, scene, dataPtr, directory);
		}

		//then do the same for each of its children
		for (u32 i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNode(model, node->mChildren[i], scene, dataPtr, directory);
		}
	}

	void ProcessAnimNode(r2::draw::AnimModel& model, aiNode* node, const aiScene* scene, r2::draw::Skeleton& skeleton, void** dataPtr, u32& numVertices, const char* directory)
	{
		for (u32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForAnimModel(model, mesh, scene, dataPtr, directory);

			ProcessBones(model, numVertices, mesh, scene);

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
			child.hashName = STRING_ID(node->mChildren[i]->mName.C_Str());
			child.transform = AssimpMat4ToGLMMat4(node->mChildren[i]->mTransformation);
			child.parent = &skeleton;
			r2::sarr::Push(*skeleton.children, child);

			ProcessAnimNode(model, node->mChildren[i], scene, r2::sarr::Last(*skeleton.children), dataPtr, numVertices, directory);
		}
	}

	void ProcessAnimations(r2::draw::AnimModel& model, void** dataPtr,  const aiNode* node, const aiScene* scene)
	{
		if (scene->HasAnimations())
		{
			u32 oldSize = (u32)r2::sarr::Size(*model.animations);
			u32 startingIndex = oldSize;
			u32 newSize = oldSize + scene->mNumAnimations;

			for (u32 i = startingIndex; i < newSize; ++i)
			{
				aiAnimation* anim = scene->mAnimations[i - oldSize];
				r2::draw::Animation r2Anim;
				r2Anim.duration = anim->mDuration;
				r2Anim.ticksPerSeconds = anim->mTicksPerSecond;
				r2Anim.hashName = STRING_ID(anim->mName.C_Str());

				if (anim->mNumChannels > 0)
				{
					r2Anim.channels = EMPLACE_SARRAY(*dataPtr, r2::draw::AnimationChannel, anim->mNumChannels);
					*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::AnimationChannel>::MemorySize(anim->mNumChannels));
				}

				for (u32 j = 0; j < anim->mNumChannels; ++j)
				{
					aiNodeAnim* animChannel = anim->mChannels[j];
					r2::draw::AnimationChannel channel;
					channel.hashName = STRING_ID(animChannel->mNodeName.C_Str());

					if (animChannel->mNumPositionKeys > 0)
					{
						channel.positionKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::VectorKey, animChannel->mNumPositionKeys);
						*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(animChannel->mNumPositionKeys));
					}

					for (u32 pKey = 0; pKey < animChannel->mNumPositionKeys; ++pKey)
					{
						r2::sarr::Push(*channel.positionKeys, { animChannel->mPositionKeys[pKey].mTime, AssimpVec3ToGLMVec3(animChannel->mPositionKeys[pKey].mValue) });
					}

					if (animChannel->mNumScalingKeys > 0)
					{
						channel.scaleKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::VectorKey, animChannel->mNumScalingKeys);
						*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(animChannel->mNumScalingKeys));
					}

					for (u32 sKey = 0; sKey < animChannel->mNumScalingKeys; ++sKey)
					{
						r2::sarr::Push(*channel.scaleKeys, { animChannel->mScalingKeys[sKey].mTime, AssimpVec3ToGLMVec3(animChannel->mScalingKeys[sKey].mValue) });
					}

					if (animChannel->mNumRotationKeys > 0)
					{
						channel.rotationKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::RotationKey, animChannel->mNumRotationKeys);
						*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::RotationKey>::MemorySize(animChannel->mNumRotationKeys));
					}

					for (u32 rKey = 0; rKey < animChannel->mNumRotationKeys; ++rKey)
					{
						r2::sarr::Push(*channel.rotationKeys, {animChannel->mRotationKeys[rKey].mTime, AssimpQuatToGLMQuat(animChannel->mRotationKeys[rKey].mValue.Normalize())});
					}

					r2::sarr::Push(*r2Anim.channels, channel);
				}

				r2::sarr::Push(*model.animations, r2Anim);
			}
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
		mNumBones = 0;
		mNumVertices = 0;
		mNumAnimations = 0;
		u64 totalSizeInBytes = GetTotalMeshBytes(scene->mRootNode, scene, mNumMeshes, mNumBones, mNumVertices, alignment, header, boundsChecking);

		if (scene->HasAnimations())
		{
			//figure out how many animations
			mNumAnimations = scene->mNumAnimations;
			totalSizeInBytes += GetTotalAnimationBytes(scene->mRootNode, scene, alignment, header, boundsChecking);
		}
		import.FreeScene();

		if (mNumAnimations > 0)
		{
			return totalSizeInBytes + r2::draw::AnimModel::MemorySizeNoData(mNumBones * r2::SHashMap<u32>::LoadFactorMultiplier(), mNumVertices, mNumBones, mNumMeshes, mNumAnimations, alignment, header, boundsChecking);
		}

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
		
		void* startOfArrayPtr = nullptr;

		if (scene->HasAnimations())
		{
			r2::draw::AnimModel* model = new (dataPtr) r2::draw::AnimModel();

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::AnimModel));

			model->meshes = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Mesh, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(mNumMeshes));

			model->boneData = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneData, mNumVertices);

			model->boneData->mSize = mNumVertices;

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneData>::MemorySize(mNumVertices));

			model->boneInfo = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneInfo, mNumBones);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneInfo>::MemorySize(mNumBones));

			model->animations = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Animation, mNumAnimations);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Animation>::MemorySize(mNumAnimations));

			u64 hashCapacity = static_cast<u64>(std::round( mNumBones * r2::SHashMap<u32>::LoadFactorMultiplier() ) );

			model->boneMapping = MAKE_SHASHMAP_IN_PLACE(s32, startOfArrayPtr, hashCapacity);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<u32>::MemorySize(hashCapacity));

			model->globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

			//Process the Nodes
			u32 numVertices = 0;
			model->skeleton.hashName = STRING_ID(scene->mRootNode->mName.C_Str());
			model->skeleton.transform = AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation);
			model->skeleton.parent = nullptr;

			ProcessAnimNode(*model, scene->mRootNode, scene, model->skeleton, &startOfArrayPtr, numVertices, "");
			//Process the animations
			ProcessAnimations(*model, &startOfArrayPtr, scene->mRootNode, scene);
		}
		else
		{
			r2::draw::Model* model = new (dataPtr) r2::draw::Model();

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

			model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Mesh, mNumMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(mNumMeshes));

			model->globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

			ProcessNode(*model, scene->mRootNode, scene, &startOfArrayPtr, "");
		}
		
		import.FreeScene();

		return true;
	}
}

