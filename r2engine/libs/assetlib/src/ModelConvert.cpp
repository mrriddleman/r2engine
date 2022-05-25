#include "assetlib/ModelConvert.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>
#include <vector>
#include "glm/gtx/string_cast.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "Hash.h"
#include "assetlib/DiskAssetFile.h"
#include "MaterialParamsPack_generated.h"

#include "assetlib/RModelMetaData_generated.h"
#include "assetlib/RModel_generated.h"
#include "assetlib/ModelAsset.h"

#define MAX_BONE_WEIGHTS 4

namespace fs = std::filesystem;

namespace r2::assets::assetlib
{
	const std::string RMDL_EXTENSION = ".rmdl";

	const float EPSILON = 0.00001f;

	bool NearEq(float x, float y)
	{
		return fabsf(x - y) < EPSILON;
	}

	struct Joint
	{
		std::string name;
		int parentIndex;
		int jointIndex;
		glm::mat4 localTransform;
	};

	struct Bone
	{
		std::string name;
		glm::mat4 invBindPoseMat;
	};

	struct BoneData
	{
		glm::vec4 boneWeights = glm::vec4(0.0f);
		glm::ivec4 boneIDs = glm::ivec4(0);
	};

	struct BoneInfo
	{
		glm::mat4 offsetTransform;
	};

	struct Transform
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = { 0,0,0,1 };
	};

	struct Vertex
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 normal = glm::vec3(0.0f);
		glm::vec3 texCoords = glm::vec3(0.0f);
		glm::vec3 tangent = glm::vec3(0.0f);
	};

	struct Skeleton
	{
		std::vector<uint64_t> mJointNames;
		std::vector<int> mParents;
		std::vector<Transform> mRestPoseTransforms;
		std::vector<Transform> mBindPoseTransforms;
		std::vector<int> mRealParentBones;
	};

	struct Mesh
	{
		uint64_t hashName = 0;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		uint64_t materialName = 0;
		int materialIndex = -1;
	};

	struct Model
	{
		fs::path binaryMaterialPath;
		std::string modelName;
		std::string originalPath;

		std::vector<uint64_t> materialNames;
		std::vector<Mesh> meshes;

		glm::mat4 globalInverseTransform;
		std::vector<BoneData> boneData;
		std::vector<BoneInfo> boneInfo;

		std::unordered_map<int64_t, int> boneMapping;

		Skeleton skeleton;
	};

	std::unordered_map<std::string, uint32_t> mBoneNecessityMap;
	std::vector<Joint> mJoints;
	std::unordered_map<std::string, Bone> mBoneMap;

	aiNode* FindNodeInHeirarchy(const aiScene* scene, aiNode* node, const std::string& strName);
	void MarkBonesInMesh(aiNode* node, const aiScene* scene, aiMesh* mesh);
	void MarkBones(aiNode* node, const aiScene* scene);
	void CreateBonesVector(const aiScene* scene, aiNode* node, int parentIndex);
	glm::mat4 GetGlobalTransform(uint32_t i);
	int FindJointIndex(const std::string& jointName);
	void LoadBindPose(Model& model, Skeleton& skeleton);
	void ProcessAnimNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene, Skeleton& skeleton, int parentDebugBone, uint32_t& numVertices);
	void ProcessNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene);
	void ProcessMeshForModel(Model& model, aiMesh* mesh, uint32_t meshIndex, const aiNode* node, const aiScene* scene);
	void GetMeshData(aiNode* node, const aiScene* scene, uint64_t& numMeshes, uint64_t& numBones, uint64_t& numVertices);

	bool ConvertModelToFlatbuffer(Model& model, const fs::path& inputFilePath, const fs::path& outputPath);


	

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

	Transform ToTransform(const glm::mat4& mat)
	{
		Transform out;

		float m[16] = { 0.0 };

		const float* pSource = (const float*)glm::value_ptr(mat);

		memcpy(m, pSource, sizeof(float) * 16);

		out.position = glm::vec3(m[12], m[13], m[14]);
		out.rotation = glm::quat_cast(mat);

		glm::mat4 rotScaleMat(
			glm::vec4(m[0], m[1], m[2], 0.f),
			glm::vec4(m[4], m[5], m[6], 0.f),
			glm::vec4(m[8], m[9], m[10], 0.f),
			glm::vec4(0.f, 0.f, 0.f, 1.f)
		);

		glm::mat4 invRotMat = glm::mat4_cast(glm::inverse(out.rotation));
		glm::mat4 scaleSkewMat = rotScaleMat * invRotMat;

		pSource = (const float*)glm::value_ptr(scaleSkewMat);

		memcpy(m, pSource, sizeof(float) * 16);

		out.scale = glm::vec3(
			m[0],
			m[5],
			m[10]
		);

		return out;
	}

	Transform AssimpMat4ToTransform(const aiMatrix4x4& mat)
	{
		return ToTransform(AssimpMat4ToGLMMat4(mat));
	}

	flat::Vertex4 ToFlatVertex4(const glm::vec4& vec)
	{
		flat::Vertex4 v4;

		v4.mutable_v()->Mutate(0, vec[0]);
		v4.mutable_v()->Mutate(1, vec[1]);
		v4.mutable_v()->Mutate(2, vec[2]);
		v4.mutable_v()->Mutate(3, vec[3]);

		return v4;
	}

	flat::Vertex4i ToFlatVertex4i(const glm::ivec4& vec)
	{
		flat::Vertex4i v4;

		v4.mutable_v()->Mutate(0, vec[0]);
		v4.mutable_v()->Mutate(1, vec[1]);
		v4.mutable_v()->Mutate(2, vec[2]);
		v4.mutable_v()->Mutate(3, vec[3]);

		return v4;
	}

	flat::Matrix4 ToFlatMatrix4(const glm::mat4& mat)
	{
		flat::Matrix4 mat4;

		mat4.mutable_cols()->Mutate(0, ToFlatVertex4(mat[0]));
		mat4.mutable_cols()->Mutate(1, ToFlatVertex4(mat[1]));
		mat4.mutable_cols()->Mutate(2, ToFlatVertex4(mat[2]));
		mat4.mutable_cols()->Mutate(3, ToFlatVertex4(mat[3]));

		return mat4;
	}

	flat::Transform ToFlatTransform(const Transform& t)
	{
		flat::Transform transform;

		transform.mutable_position()->Mutate(0, t.position[0]);
		transform.mutable_position()->Mutate(1, t.position[1]);
		transform.mutable_position()->Mutate(2, t.position[2]);

		transform.mutable_scale()->Mutate(0, t.scale[0]);
		transform.mutable_scale()->Mutate(1, t.scale[1]);
		transform.mutable_scale()->Mutate(2, t.scale[2]);

		transform.mutable_rotation()->Mutate(0, t.rotation[0]);
		transform.mutable_rotation()->Mutate(1, t.rotation[1]);
		transform.mutable_rotation()->Mutate(2, t.rotation[2]);
		transform.mutable_rotation()->Mutate(3, t.rotation[3]);

		return transform;
	}

	bool ConvertModel(const std::filesystem::path& inputFilePath, const std::filesystem::path& parentOutputDir, const std::filesystem::path& binaryMaterialParamPacksManifestFile, const std::string& extension)
	{
		mJoints.clear();
		mBoneNecessityMap.clear();
		mBoneMap.clear();

		Assimp::Importer import;

		uint8_t flags = 
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_ImproveCacheLocality |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FindDegenerates |
			aiProcess_GenUVCoords |
			aiProcess_LimitBoneWeights |
			aiProcess_OptimizeMeshes |
			aiProcess_SplitByBoneCount;

		const aiScene* scene = import.ReadFile(inputFilePath.string(), flags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::string errString = "Failed to load model: " + std::string( import.GetErrorString() );

			assert(false && errString.c_str());
			return false;
		}

		uint64_t numMeshes = 0;
		uint64_t numBones = 0;
		uint64_t numVertices = 0;

		MarkBones(scene->mRootNode, scene);
		CreateBonesVector(scene, scene->mRootNode, -1);

		GetMeshData(scene->mRootNode, scene, numMeshes, numBones, numVertices);

		bool isAnimated = mJoints.size() > 0;
		uint32_t meshIndex = 0;
		
		Model model;
		model.binaryMaterialPath = binaryMaterialParamPacksManifestFile;
		model.originalPath = inputFilePath.string();

		model.modelName = scene->mRootNode->mName.C_Str();

		model.materialNames.reserve(numMeshes);

		model.meshes.reserve(numMeshes);

		model.globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));

		if (isAnimated)
		{
			model.boneData.resize(numVertices);

			model.boneInfo.reserve(numBones);

			model.boneMapping.reserve(numBones);

			uint32_t vertices = 0;

			const auto numJoints = mJoints.size();

			model.skeleton.mJointNames.reserve(numJoints);

			model.skeleton.mParents.reserve(numJoints);

			model.skeleton.mRestPoseTransforms.reserve(numJoints);

			model.skeleton.mBindPoseTransforms.resize(numJoints);

			model.skeleton.mRealParentBones.resize(numJoints);

			ProcessAnimNode(model, scene->mRootNode, meshIndex, scene, model.skeleton, 0, vertices);

			LoadBindPose(model, model.skeleton);
		}
		else
		{
			ProcessNode(model, scene->mRootNode, meshIndex, scene);
		}
		
		import.FreeScene();

		return ConvertModelToFlatbuffer(model, inputFilePath, parentOutputDir);
	}

	void CreateBonesVector(const aiScene* scene, aiNode* node, int parentIndex)
	{
		std::string nodeName = std::string(node->mName.data);

		auto iter = mBoneNecessityMap.find(nodeName);
		if (iter != mBoneNecessityMap.end() && (bool)iter->second == true)
		{
			mJoints.push_back(Joint());

			mJoints.back().parentIndex = parentIndex;
			mJoints.back().jointIndex = mJoints.size() - 1;
			mJoints.back().localTransform = AssimpMat4ToGLMMat4(node->mTransformation);
			mJoints.back().name = nodeName;

			parentIndex = mJoints.size() - 1;
		}

		for (uint64_t i = 0; i < node->mNumChildren; ++i)
		{
			CreateBonesVector(scene, node->mChildren[i], parentIndex);
		}
	}

	aiNode* FindNodeInHeirarchy(const aiScene* scene, aiNode* node, const std::string& strName)
	{
		if (strName == std::string(node->mName.data))
		{
			return node;
		}

		auto numChildren = node->mNumChildren;

		for (uint64_t i = 0; i < numChildren; ++i)
		{
			aiNode* child = node->mChildren[i];
			aiNode* result = FindNodeInHeirarchy(scene, child, strName);

			if (result != nullptr)
			{
				return result;
			}
		}

		return nullptr;
	}

	void MarkBonesInMesh(aiNode* node, const aiScene* scene, aiMesh* mesh)
	{
		for (uint32_t i = 0; i < mesh->mNumBones; ++i)
		{
			std::string boneNameStr = std::string(mesh->mBones[i]->mName.data);

			mBoneMap[boneNameStr] = Bone();
			mBoneMap[boneNameStr].invBindPoseMat = AssimpMat4ToGLMMat4(mesh->mBones[i]->mOffsetMatrix);
			mBoneMap[boneNameStr].name = boneNameStr;


			aiNode* boneNode = FindNodeInHeirarchy(scene, scene->mRootNode, boneNameStr);

			assert(boneNode != nullptr && "HMMM");

			mBoneNecessityMap[boneNameStr] = true;

			aiNode* nextNode = boneNode->mParent;
			while (nextNode != nullptr && nextNode != node && nextNode != node->mParent)
			{
				std::string nextNodeName = std::string(nextNode->mName.data);

				mBoneNecessityMap[nextNodeName] = true;

				nextNode = nextNode->mParent;
			}
		}
	}

	void MarkBones(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			MarkBonesInMesh(node, scene, mesh);
		}

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			MarkBones(node->mChildren[i], scene);
		}
	}

	void GetMeshData(aiNode* node, const aiScene* scene, uint64_t& numMeshes, uint64_t& numBones, uint64_t& numVertices)
	{
		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			GetMeshData(node->mChildren[i], scene, numMeshes, numBones, numVertices);
		}

		numMeshes += node->mNumMeshes;

		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			numVertices += mesh->mNumVertices;

			if (mesh->mNumBones > 0)
			{
				numBones += mesh->mNumBones;
			}
		}
	}

	glm::mat4 GetGlobalTransform(uint32_t i)
	{
		glm::mat4 result = mJoints[i].localTransform;

		for (int p = mJoints[i].parentIndex; p >= 0; p = mJoints[p].parentIndex)
		{
			result = mJoints[p].localTransform * result;
		}

		return result;
	}

	int FindJointIndex(const std::string& jointName)
	{
		for (uint32_t i = 0; i < mJoints.size(); ++i)
		{
			if (mJoints[i].name == jointName)
			{
				return (int)i;
			}
		}

		return -1;
	}

	void LoadBindPose(Model& model, Skeleton& skeleton)
	{

		//@TODO(Serge): Right now I don't think this is correct. It SEEMS correct since we can put the model into bind pose no problem BUT when it comes to animating the model, this messes up if we first put the model into bind pose which doesn't seems correct.
		//				This might be a weird artifact of ASSIMP since the rest pose is frame 1 of the animation. It might be that we can't load an animation as our "base model" as we're doing for the Skeleton Archer and Ellen. Interestingly, this works for the Micro Bat
		//				Could also be that our math is messed up somewhere but currently I don't see where....
		std::vector<glm::mat4> worldBindPos(mJoints.size());

		for (uint32_t i = 0; i < mJoints.size(); ++i)
		{
			worldBindPos[i] = GetGlobalTransform(i);
		}

		for (auto iter = mBoneMap.begin(); iter != mBoneMap.end(); iter++)
		{
			int jointIndex = FindJointIndex(iter->first);

			if (jointIndex >= 0)
			{
				worldBindPos[jointIndex] = glm::inverse(iter->second.invBindPoseMat);
			}
			else
			{
				assert(jointIndex != -1 && "Fail....");
			}
		}

		for (uint32_t i = 0; i < mJoints.size(); ++i)
		{
			glm::mat4 current = worldBindPos[i];

			int p = mJoints[i].parentIndex;
			if (p >= 0)
			{
				glm::mat4 parent = worldBindPos[p];
				current = glm::inverse(parent) * current;
			}

			skeleton.mBindPoseTransforms[i] = ToTransform(current);
		}
	}

	void ProcessMeshForModel(Model& model, aiMesh* mesh, uint32_t meshIndex, const aiNode* node, const aiScene* scene)
	{
		Mesh nextMesh;


		//@NOTE: I think this is still wrong - we're not weighting the mesh vertices based on the bone influences (if that matters)

		//@NOTE: What this is doing, for the mesh we're looking at we take the local transform up to the first bone we find, then we multiply that transforms by the bind pose of the bone
		//		 Not sure. I think the mTransformation matrices for the meshes remain the same (as the bind pose) up until they encounter the bone which is why this works
		glm::mat4 localTransform = AssimpMat4ToGLMMat4(node->mTransformation);
		{
			const aiNode* nextNode = node->mParent;

			glm::mat4 bindMat = glm::mat4(1.0f);

			aiNode* foundNode = nullptr;
			while (nextNode)
			{
				auto iter = mBoneMap.find(nextNode->mName.C_Str());

				if (iter != mBoneMap.end())
				{
					glm::mat4 temp = iter->second.invBindPoseMat;

					bindMat = glm::inverse(temp);

					break;
				}
				else
				{
					localTransform = AssimpMat4ToGLMMat4(nextNode->mTransformation) * localTransform;
				}

				nextNode = nextNode->mParent;
			}

			localTransform = bindMat * localTransform * AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation);
		}

		nextMesh.vertices.reserve(mesh->mNumVertices);


		if (mesh->mNumFaces > 0)
		{
			nextMesh.indices.reserve(mesh->mNumFaces * 3);
		}

		uint64_t materialName = 0;

		int materialIndex = -1;

		//@NOTE(Serge): change later 
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			assert(material != nullptr && "Material is null!");
			if (material)
			{
				const uint32_t numTextures = aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);

				DiskAssetFile diskFile;

				bool openResult = diskFile.OpenForRead(model.binaryMaterialPath.string().c_str());

				assert(openResult && "Couldn't open the file!");

				auto fileSize = diskFile.Size();

				char* materialManifestData = new char[fileSize];

				auto readSize = diskFile.Read(materialManifestData, fileSize);

				assert(readSize == fileSize && "Didn't read whole file");

				const auto materialManifest = flat::GetMaterialParamsPack(materialManifestData);
				
				if (numTextures > 0)
				{
					for (int i = 0; i < numTextures; ++i)
					{
						aiString diffuseStr;

						material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseStr);

						fs::path diffuseTexturePath = diffuseStr.C_Str();

						diffuseTexturePath.make_preferred();

						fs::path diffuseTextureName = diffuseTexturePath.filename();

						diffuseTextureName.replace_extension(".rtex");
						std::string diffuseTextureNameStr = diffuseTextureName.string();

						std::transform(diffuseTextureNameStr.begin(), diffuseTextureNameStr.end(), diffuseTextureNameStr.begin(),
							[](unsigned char c) { return std::tolower(c); });

						//now look into all the materials and find the material that matches this texture name


						//@TODO(Serge): we've put the texture pack name into the material for each texture (we only need the diffuse/albedo texture)
						//				thus we need to build the texture name like: texturePackNameStr() / albedo / diffuseTextureName

						const auto numMaterials = materialManifest->pack()->size();
						bool found = false;

						for (flatbuffers::uoffset_t i = 0; i < numMaterials && !found; ++i)
						{
							const flat::MaterialParams* materialParams = materialManifest->pack()->Get(i);

							for (flatbuffers::uoffset_t t = 0; t < materialParams->textureParams()->size() && !found; ++t)
							{
								const flat::MaterialTextureParam* texParam = materialParams->textureParams()->Get(t);

								if (texParam->propertyType() == flat::MaterialPropertyType_ALBEDO)
								{
									auto packNameStr = texParam->texturePackNameStr()->str();
									
									std::string textureNameWithParents = packNameStr + "/albedo/" + diffuseTextureNameStr;

									auto textureNameID = STRING_ID(textureNameWithParents.c_str());

									if (textureNameID == texParam->value())
									{
										materialName = materialParams->name();
										found = true;
										break;
									}
								}
							}
						}

						assert(found && "Couldn't find it");
					}
				}
				else
				{
					const char* matName = material->GetName().C_Str();

					materialName = STRING_ID(matName);
				}


				delete[] materialManifestData;
				//r2::draw::MaterialHandle materialHandle = {};

				//if (numTextures > 0)
				//{
				//	//find the first one that isn't invalid
				//	for (int i = 0; i < numTextures && r2::draw::mat::IsInvalidHandle(materialHandle); ++i)
				//	{
				//		//@TODO(Serge): check other texture types if we don't find this one
				//		//@TODO(Serge): what do we do if we don't have any textures?
				//		aiString diffuseStr;

				//		material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseStr);


				//		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
				//		char textureName[r2::fs::FILE_PATH_LENGTH];
				//		r2::fs::utils::SanitizeSubPath(diffuseStr.C_Str(), sanitizedPath);

				//		r2::fs::utils::CopyFileNameWithExtension(sanitizedPath, textureName);

				//		char extType[r2::fs::FILE_PATH_LENGTH];

				//		//@TODO(Serge): clean up this MAJOR HACK!
				//		if (r2::fs::utils::CopyFileExtension(textureName, extType))
				//		{
				//			if (strcmp(extType, RTEX_EXT.c_str()) != 0)
				//			{
				//				r2::fs::utils::SetNewExtension(textureName, RTEX_EXT.c_str());
				//			}
				//		}

				//		r2::draw::tex::Texture tex;
				//		materialHandle = r2::draw::matsys::FindMaterialFromTextureName(textureName, tex);
				//	}
				//}
				//else
				//{
				//	const char* matName = material->GetName().C_Str();

				//	auto matNameID = STRING_ID(matName);

				//	materialHandle = r2::draw::matsys::FindMaterialHandle(matNameID);
				//}

				if (materialName != 0)
				{
					bool found = false;

					auto numMaterialHandles = model.materialNames.size();

					for (uint64_t i = 0; i < numMaterialHandles; ++i)
					{
						auto nextMatHandle = model.materialNames[i];

						if (nextMatHandle == materialName)
						{
							materialIndex = i;
							found = true;
							break;
						}
					}

					if (!found)
					{
						materialIndex = numMaterialHandles;
						model.materialNames.push_back(materialName);
					}
				}
				else
				{
					assert(false && "Failed to load the material for the mesh");
				}

			}
		}

		assert(materialIndex != -1 && "We should have a material for the mesh!");

		assert(materialName != 0 && "We should have a proper material");

		nextMesh.materialName = materialName;
		nextMesh.materialIndex = materialIndex;

		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex nextVertex;

			nextVertex.position = localTransform * glm::vec4(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), 1.0);

			if (mesh->mNormals)
			{
				nextVertex.normal = glm::mat3(glm::transpose(glm::inverse(localTransform))) * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			if (mesh->mTextureCoords)
			{
				nextVertex.texCoords = glm::vec3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, materialIndex);
			}

			if (mesh->mTangents)
			{
				nextVertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
				//		printf("nextVertex.tangent - x: %f, y: %f, z: %f\n", nextVertex.tangent.x, nextVertex.tangent.y, nextVertex.tangent.y);
			}

			//if (mesh->mBitangents)
			//{
			//	nextVertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			//}
			nextMesh.vertices.push_back(nextVertex);
		}

		//u32 indexOffsetToUse = indexOffset;
		for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; ++j)
			{
				uint32_t nextIndex = face.mIndices[j];
				//	printf("face.mIndices[j] + indexOffsetToUse: %zu\n", nextIndex);
				//r2::sarr::Push(*nextMeshPtr->optrIndices, nextIndex);
				nextMesh.indices.push_back(nextIndex);
				//if (nextIndex > indexOffset)
				//{
				//	indexOffset = nextIndex;
				//}
			}

		}

		//		indexOffset++;

		model.meshes.push_back(nextMesh);
		//r2::sarr::Push(*model.optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
	}

	void ProcessNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene)
	{
		//process all of the node's meshes
		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForModel(model, mesh, meshIndex, node, scene);

			++meshIndex;
		}

		//then do the same for each of its children
		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNode(model, node->mChildren[i], meshIndex, scene);
		}
	}

	void ProcessBones(Model& model, uint32_t baseVertex, const aiMesh* mesh, const aiNode* node, const aiScene* scene)
	{
		const aiMesh* meshToUse = mesh;
		//printf("Num bones: %zu\n", meshToUse->mNumBones);

		for (uint32_t i = 0; i < meshToUse->mNumBones; ++i)
		{
			int boneIndex = -1;
			std::string boneNameStr = std::string(meshToUse->mBones[i]->mName.data);
			//		printf("boneNameStr name: %s\n", boneNameStr.c_str());

			uint64_t boneName = STRING_ID(meshToUse->mBones[i]->mName.C_Str());

			//int theDefault = -1;
			//s32 result = r2::shashmap::Get(*model.boneMapping, boneName, theDefault);


			auto result = model.boneMapping.find(boneName);

			if (result == model.boneMapping.end())
			{
				boneIndex = model.boneInfo.size();//(u32)r2::sarr::Size(*model.boneInfo);
				BoneInfo info;
				info.offsetTransform = AssimpMat4ToGLMMat4(meshToUse->mBones[i]->mOffsetMatrix);
				model.boneInfo.push_back(info);

				model.boneMapping[boneName] = boneIndex;
			}
			else
			{
				boneIndex = result->second;
			}

			//	printf("boneIndex: %i\n", boneIndex);

			size_t numWeights = meshToUse->mBones[i]->mNumWeights;

			for (uint32_t j = 0; j < numWeights; ++j)
			{
				uint32_t vertexID = baseVertex + meshToUse->mBones[i]->mWeights[j].mVertexId;

				float currentWeightTotal = 0.0f;
				for (uint32_t k = 0; k < MAX_BONE_WEIGHTS; ++k)
				{
					currentWeightTotal += model.boneData[vertexID].boneWeights[k];

					if (!NearEq(currentWeightTotal, 1.0f) && NearEq(model.boneData[vertexID].boneWeights[k], 0.0f))
					{
						model.boneData[vertexID].boneIDs[k] = boneIndex;
						model.boneData[vertexID].boneWeights[k] = meshToUse->mBones[i]->mWeights[j].mWeight;
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
				uint64_t boneName = STRING_ID(nodeToUse->mName.C_Str());
				
				//s32 boneIndex = r2::shashmap::Get(*model.boneMapping, boneName, theDefault);

				auto boneIter = model.boneMapping.find(boneName);

				if (boneIter != model.boneMapping.end())
				{
					for (uint64_t i = 0; i < meshToUse->mNumVertices; ++i)
					{
						uint32_t vertexID = baseVertex + i;

						model.boneData[vertexID].boneIDs[0] = boneIter->second;
						model.boneData[vertexID].boneWeights[0] = 1.0;
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

	void ProcessAnimNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene, Skeleton& skeleton, int parentDebugBone, uint32_t& numVertices)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMeshForModel(model, mesh, meshIndex, node, scene);

			ProcessBones(model, numVertices, mesh, node, scene);

			numVertices += mesh->mNumVertices;

			++meshIndex;
		}

		auto iter = mBoneNecessityMap.find(node->mName.C_Str());
		if (iter != mBoneNecessityMap.end())
		{
			skeleton.mJointNames.push_back(STRING_ID(node->mName.C_Str()));
			skeleton.mRestPoseTransforms.push_back(AssimpMat4ToTransform(node->mTransformation));

			int jointIndex = (int)skeleton.mJointNames.size() - 1;

			const Joint& joint = mJoints[jointIndex];

			skeleton.mParents.push_back(joint.parentIndex);

			//#ifdef R2_DEBUG
			//			skeleton.mDebugBoneNames.push_back(node->mName.C_Str());
			//#endif // R2_DEBUG

			auto realBoneIter = mBoneMap.find(node->mName.C_Str());
			if (realBoneIter != mBoneMap.end())
			{
			//	r2::sarr::At(*skeleton.mRealParentBones, jointIndex) = parentDebugBone;

				skeleton.mRealParentBones[jointIndex] = parentDebugBone;

				parentDebugBone = jointIndex;
			}
		}

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			ProcessAnimNode(model, node->mChildren[i], meshIndex, scene, model.skeleton, parentDebugBone, numVertices);
		}
	}

	bool ConvertModelToFlatbuffer(Model& model, const fs::path& inputFilePath, const fs::path& outputPath )
	{
		//meta data
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::MeshInfo>> meshInfos;

		const auto numMeshes = model.meshes.size();

		meshInfos.reserve(numMeshes);

		std::vector<Position> modelPositions;

		uint32_t totalVertices = 0;
		uint32_t totalIndices = 0;

		for (size_t i = 0; i < numMeshes; ++i)
		{
			std::vector<Position> meshPositions;

			const Mesh& mesh = model.meshes[i];

			const auto numVertices = mesh.vertices.size();
			const auto numIndices = mesh.indices.size();

			meshPositions.reserve(numVertices);

			for (size_t p = 0; p < numVertices; ++p)
			{
				Position newPosition;

				newPosition.v[0] = mesh.vertices[p].position.x;
				newPosition.v[1] = mesh.vertices[p].position.y;
				newPosition.v[2] = mesh.vertices[p].position.z;

				meshPositions.push_back(newPosition);
			}
			modelPositions.insert(modelPositions.end(), meshPositions.begin(), meshPositions.end());

			Bounds meshBounds = CalculateBounds(meshPositions.data(), meshPositions.size());
			flat::Bounds flatBounds = flat::Bounds(
				flat::Vertex3(meshBounds.origin[0], meshBounds.origin[1], meshBounds.origin[2]),
				meshBounds.radius,
				flat::Vertex3(meshBounds.extents[0], meshBounds.extents[1], meshBounds.extents[2]));

			auto meshInfo = flat::CreateMeshInfo(
				builder,
				mesh.hashName,
				flat::VertexOrdering_INTERLEAVED,
				flat::VertexFormat_P32N32UV32T32,
				numVertices * sizeof(Vertex),
				numIndices * sizeof(uint32_t),
				&flatBounds,
				sizeof(uint32_t),
				flat::MeshCompressionMode_LZ4,
				numVertices * sizeof(Vertex) + numIndices * sizeof(uint32_t),
				builder.CreateString(model.originalPath));

			meshInfos.push_back(meshInfo);

			totalVertices += numVertices;
			totalIndices += numIndices;
		}

		Bounds modelBounds = CalculateBounds(modelPositions.data(), modelPositions.size());
		flat::Bounds flatBounds = flat::Bounds(
			flat::Vertex3(modelBounds.origin[0], modelBounds.origin[1], modelBounds.origin[2]),
			modelBounds.radius,
			flat::Vertex3(modelBounds.extents[0], modelBounds.extents[1], modelBounds.extents[2]));

		auto modelMetaDataOffset = flat::CreateRModelMetaData(
			builder,
			STRING_ID(model.modelName.c_str()),
			builder.CreateVector(meshInfos),
			&flatBounds,
			numMeshes,
			model.materialNames.size(),
			totalVertices,
			totalIndices,
			model.boneData.size() > 0,
			flat::CreateBoneMetaData(builder, model.boneData.size(), model.boneInfo.size()),
			flat::CreateSkeletonMetaData(builder, model.skeleton.mJointNames.size(), model.skeleton.mParents.size(), model.skeleton.mRestPoseTransforms.size(), model.skeleton.mBindPoseTransforms.size()),
			builder.CreateString(model.originalPath));

		builder.Finish(modelMetaDataOffset, "mdmd");

		uint8_t* metaDataBuf = builder.GetBufferPointer();
		size_t metaDataSize = builder.GetSize();

		auto modelMetaData = flat::GetMutableRModelMetaData(metaDataBuf);


		//mesh data
		flatbuffers::FlatBufferBuilder dataBuilder;

		std::vector<flatbuffers::Offset<flat::MaterialName>> flatMaterialNames;
		std::vector<flatbuffers::Offset<flat::RMesh>> flatMeshes;

		const auto numMeshesInModel = model.meshes.size();

		std::vector<std::vector< int8_t> > meshData;
		meshData.resize(model.meshes.size());

		for (size_t i = 0; i < numMeshesInModel; ++i)
		{
			auto& mesh = model.meshes[i];

			//const auto vertexBufferSize = sizeof(Vertex) * mesh.vertices.size();
			//const auto indexBufferSize = sizeof(uint32_t) * mesh.indices.size();
			//meshData[i].resize(vertexBufferSize + indexBufferSize);

			//memcpy(meshData[i].data(), mesh.vertices.data(), vertexBufferSize);
			//memcpy(meshData[i].data() + vertexBufferSize, mesh.indices.data(), indexBufferSize);
			auto compressedSize = modelMetaData->meshInfos()->Get(i)->compressedSize();

			meshData[i].resize(compressedSize);

			pack_mesh(dataBuilder, modelMetaData, i, reinterpret_cast<char*>(meshData[i].data()), mesh.materialIndex, reinterpret_cast<char*>(mesh.vertices.data()), reinterpret_cast<char*>(mesh.indices.data()));

			compressedSize = modelMetaData->meshInfos()->Get(i)->compressedSize();

			meshData[i].resize(compressedSize);

			flatMeshes.push_back(flat::CreateRMesh(dataBuilder, mesh.materialIndex, dataBuilder.CreateVector(meshData[i])));


		}

		const auto numMaterialsInModel = model.materialNames.size();
		for (size_t i = 0; i < numMaterialsInModel; ++i)
		{
			flatMaterialNames.push_back(flat::CreateMaterialName(dataBuilder, model.materialNames[i]));
		}

		flat::Matrix4 flatGlobalInverseTransform = ToFlatMatrix4(model.globalInverseTransform);

		flatbuffers::Offset<flat::AnimationData> animationData = 0;

		if (model.boneInfo.size() > 0)
		{
			std::vector<flat::BoneData> flatBoneDatas;
			std::vector<flat::BoneInfo> flatBoneInfos;
			std::vector<flat::Transform> flatRestPoseTransforms;
			std::vector<flat::Transform> flatBindPoseTransforms;
			std::vector<uint64_t> flatJointNames;
			std::vector<int32_t> flatParents;
			std::vector<int32_t> flatRealParentBones;

			const auto numBoneDatas = model.boneData.size();

			for (size_t i = 0; i < numBoneDatas; ++i)
			{
				const auto& nextBoneData = model.boneData[i];

				flat::BoneData flatBoneData;
				flatBoneData.mutable_boneWeights() = ToFlatVertex4(nextBoneData.boneWeights);
				flatBoneData.mutable_boneIDs() = ToFlatVertex4i(nextBoneData.boneIDs);

				flatBoneDatas.push_back(flatBoneData);
			}

			for (size_t i = 0; i < model.boneInfo.size(); ++i)
			{
				const auto& nextBoneInfo = model.boneInfo[i];

				flat::BoneInfo flatBoneInfo;
				flatBoneInfo.mutable_offsetTransform() = ToFlatMatrix4(model.boneInfo[i].offsetTransform);

				flatBoneInfos.push_back(flatBoneInfo);
			}

			for (size_t i = 0; i < model.skeleton.mRestPoseTransforms.size(); ++i)
			{
				const auto& restPoseTransform = model.skeleton.mRestPoseTransforms[i];

				flatRestPoseTransforms.push_back(ToFlatTransform(model.skeleton.mRestPoseTransforms[i]));
			}

			for (size_t i = 0; i < model.skeleton.mBindPoseTransforms.size(); ++i)
			{
				const auto& bindPoseTransform = model.skeleton.mBindPoseTransforms[i];

				flatBindPoseTransforms.push_back(ToFlatTransform(model.skeleton.mBindPoseTransforms[i]));
			}

			for (size_t i = 0; i < model.skeleton.mJointNames.size(); ++i)
			{
				flatJointNames.push_back(model.skeleton.mJointNames[i]);
			}

			for (size_t i = 0; i < model.skeleton.mParents.size(); ++i)
			{
				flatParents.push_back(model.skeleton.mParents[i]);
			}

			for (size_t i = 0; i < model.skeleton.mRealParentBones.size(); ++i)
			{
				flatRealParentBones.push_back(model.skeleton.mRealParentBones[i]);
			}

			animationData = flat::CreateAnimationData(
				dataBuilder,
				dataBuilder.CreateVectorOfStructs(flatBoneDatas),
				dataBuilder.CreateVectorOfStructs(flatBoneInfos),
				dataBuilder.CreateVector(flatJointNames),
				dataBuilder.CreateVector(flatParents),
				dataBuilder.CreateVectorOfStructs(flatRestPoseTransforms),
				dataBuilder.CreateVectorOfStructs(flatBindPoseTransforms),
				dataBuilder.CreateVector(flatRealParentBones));
		}
		
		const auto rmodel = flat::CreateRModel(
			dataBuilder,
			STRING_ID(model.modelName.c_str()),
			&flatGlobalInverseTransform,
			dataBuilder.CreateVector(flatMaterialNames),
			dataBuilder.CreateVector(flatMeshes),
			animationData);

		dataBuilder.Finish(rmodel, "rmdl");

		uint8_t* modelData = dataBuilder.GetBufferPointer();
		size_t modelDataSize = dataBuilder.GetSize();

		DiskAssetFile assetFile;
		assetFile.SetFreeDataBlob(false);
		pack_model(assetFile, metaDataBuf, metaDataSize, modelData, modelDataSize);


		fs::path filenamePath = inputFilePath.filename();

		fs::path outputFilePath = outputPath / filenamePath.replace_extension(RMDL_EXTENSION);

		bool result = save_binaryfile(outputFilePath.string().c_str(), assetFile);

		if (!result)
		{
			printf("failed to output file: %s\n", outputFilePath.string().c_str());
			return false;
		}

		return result;
	}
}
