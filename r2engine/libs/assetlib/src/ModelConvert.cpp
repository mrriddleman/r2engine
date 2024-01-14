#include "assetlib/ModelConvert.h"

//#include <assimp/Importer.hpp>
//#include <assimp/postprocess.h>
//#include <assimp/scene.h>
#include <cassert>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hash.h"

#include "AssetName_generated.h"
#include "Materials/MaterialPack_generated.h"
#include "Shader/ShaderParams_generated.h"

#include "Textures/TexturePackManifest_generated.h"
#include "assetlib/DiskAssetFile.h"
#include "assetlib/RModelMetaData_generated.h"
#include "assetlib/RModel_generated.h"
#include "assetlib/ModelAsset.h"
#include "assetlib/AssetUtils.h"
#include "assetlib/AnimationConvert.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include <unordered_map>
#include <vector>
#include <map>

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

	bool WriteFile(const std::string& filePath, char* data, size_t size)
	{
		std::fstream fs;
		fs.open(filePath, std::fstream::out | std::fstream::binary );
		if (fs.good())
		{
			fs.write((const char*)data, size);

			assert(fs.good()&& "Failed to write out the buffer!");
			if (!fs.good())
			{
				return false;
			}
		}

		fs.close();
		return true;
	}

	struct MaterialName
	{
		uint64_t name = 0;
		uint64_t materialPackName = 0;
		std::string materialNameStr = "";
		std::string materialPackNameStr = "";
	};

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

	inline glm::quat QuatMult(const glm::quat& Q1, const glm::quat& Q2)
	{
		glm::quat r;

		r.x = Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x;
		r.y = -Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y;
		r.z = Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z;
		r.w = -Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w;

		return r;
	}

	Transform Combine(const Transform& a, const Transform& b) {
		Transform out;

		out.scale.x = a.scale.x * b.scale.x;
		out.scale.y = a.scale.y * b.scale.y;
		out.scale.z = a.scale.z * b.scale.z;

		out.rotation = QuatMult(b.rotation, a.rotation);

		glm::vec3 temp = b.position;

		temp.x *= a.scale.x;
		temp.y *= a.scale.y;
		temp.z *= a.scale.z;

		out.position = a.rotation * temp;

		out.position.x += a.position.x;
		out.position.y += a.position.y;
		out.position.z += a.position.z;

		return out;
	}

	Transform Inverse(const Transform& t)
	{
		Transform inv;

		inv.rotation = glm::inverse(t.rotation);

		inv.scale.x = fabs(t.scale.x) < 0.0001 ? 0.0f : 1.0f / t.scale.x;
		inv.scale.y = fabs(t.scale.y) < 0.0001 ? 0.0f : 1.0f / t.scale.y;
		inv.scale.z = fabs(t.scale.z) < 0.0001 ? 0.0f : 1.0f / t.scale.z;

		glm::vec3 invTrans = t.position * -1.0f;
		inv.position = inv.rotation * (inv.scale * invTrans);

		return inv;
	}

	glm::mat4 TransformToMat4(const Transform& t)
	{
		// First, extract the rotation basis of the transform
		glm::vec3 x = t.rotation * glm::vec3(1, 0, 0);
		glm::vec3 y = t.rotation * glm::vec3(0, 1, 0);
		glm::vec3 z = t.rotation * glm::vec3(0, 0, 1);

		// Next, scale the basis vectors
		x = x * t.scale.x;
		y = y * t.scale.y;
		z = z * t.scale.z;

		// Extract the position of the transform
		glm::vec3 p = t.position;

		// Create matrix
		return glm::mat4(
			glm::vec4(x, 0), // X basis (& Scale)
			glm::vec4(y, 0), // Y basis (& scale)
			glm::vec4(z, 0), // Z basis (& scale)
			glm::vec4(p, 1));  // Position

	}


	struct Vertex
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 normal = glm::vec3(0.0f);
		glm::vec3 texCoords = glm::vec3(0.0f);
		glm::vec3 tangent = glm::vec3(0.0f);
	};

	struct Pose
	{
		std::vector<int> parents;
		std::vector<Transform> joints;

		Transform GetGlobalTransform(uint32_t index) const
		{
			Transform result = joints[index];
			for (int p = parents[index]; p >= 0; p = parents[p])
			{
				result = Combine(joints[p], result);
			}
			return result;
		}

		void Clear()
		{
			parents.clear();
			joints.clear();
		}

		void Load(const Pose& otherPose)
		{
			Clear();
			parents.insert(parents.end(), otherPose.parents.begin(), otherPose.parents.end());
			joints.insert(joints.end(), otherPose.joints.begin(), otherPose.joints.end());
		}
	};

	struct Skeleton
	{
		//std::vector<uint64_t> mJointNames;
		//std::vector<int> mParents;
		//std::vector<Transform> mRestPoseTransforms;
		//std::vector<Transform> mBindPoseTransforms;
		//std::vector<int> mRealParentBones;
		

		Pose mRestPose;
		Pose mBindPose;
		std::vector<std::string> mJointNameStrings;
		std::vector<glm::mat4> mInvBindPose;

		void UpdateInverseBindPose()
		{
			unsigned int size = mBindPose.joints.size();
			mInvBindPose.resize(size);
			for (unsigned int i = 0; i < size; ++i)
			{
				Transform world = mBindPose.GetGlobalTransform(i);
				mInvBindPose[i] = glm::inverse(TransformToMat4(world));
			}
		}
	};

	template<typename T>
	struct Frame
	{
		float mTime;
		T mValue;
		T mIn;
		T mOut;
	};

	typedef Frame<float> ScalarFrame;
	typedef Frame<glm::vec3> VectorFrame;
	typedef Frame<glm::quat> QuatFrame;

	template Frame<float>;
	template Frame<glm::vec3>;
	template Frame<glm::quat>;

	template<typename T>
	struct Track
	{
		std::vector<uint32_t> mSampledFrames;
		std::vector<Frame<T>> mFrames;
		fastgltf::AnimationInterpolation interpolation; //@TODO(Serge): change to the flat version maybe
		unsigned int mNumberOfSamples;

		void UpdateIndexLookupTable(unsigned int numberOfSamples)
		{
			int numFrames = (int)mFrames.size();
			if (numFrames <= 1)
			{
				return;
			}

			mNumberOfSamples = numberOfSamples;

			float startTime = mFrames[0].mTime;
			float duration = mFrames[mFrames.size()-1].mTime - startTime;

			unsigned int numSamples = duration * static_cast<float>(numberOfSamples);
			mSampledFrames.resize(numSamples);

			for (unsigned int i = 0; i < numSamples; ++i)
			{
				float t = (float)i / (float(numSamples - 1));
				float time = t * duration + startTime;
				unsigned int frameIndex = 0;
				for (int j = numFrames - 1; j >= 0; --j)
				{
					if (time >= mFrames[j].mTime)
					{
						frameIndex = (unsigned int)j;
						if ((int)frameIndex >= numFrames - 2)
						{
							frameIndex = numFrames - 2;
						}
						break;
					}
				}

				mSampledFrames[i] = frameIndex;
			}
		}
	};

	typedef Track<float> ScalarTrack;
	typedef Track<glm::vec3> VectorTrack;
	typedef Track<glm::quat> QuatTrack;

	template Track<float>;
	template Track<glm::vec3>;
	template Track<glm::quat>;

	struct TransformTrack
	{
		int32_t mJointID;
		VectorTrack mPosition;
		QuatTrack mRotation;
		VectorTrack mScale;

		float GetStartTime() const 
		{
			float result = 0.0f;
			bool isSet = false;
			if (mPosition.mFrames.size() > 1)
			{
				result = mPosition.mFrames[0].mTime;
				isSet = true;
			}

			if (mRotation.mFrames.size() > 1)
			{
				float rotationStart = mRotation.mFrames[0].mTime;
				if (rotationStart < result || !isSet)
				{
					result = rotationStart;
					isSet = true;
				}
			}

			if (mScale.mFrames.size() > 1)
			{
				float scaleStart = mScale.mFrames[0].mTime;
				if (scaleStart < result || !isSet)
				{
					result = scaleStart;
					isSet = true;
				}
			}

			return 0.0f;
		}

		float GetEndTime() const
		{
			float result = 0.0f;
			bool isSet = false;
			if (mPosition.mFrames.size() > 1)
			{
				result = mPosition.mFrames[mPosition.mFrames.size()-1].mTime;
				isSet = true;
			}
			if (mRotation.mFrames.size() > 1)
			{
				float rotationEnd = mRotation.mFrames[mRotation.mFrames.size() - 1].mTime;
				if (rotationEnd > result || !isSet)
				{
					result = rotationEnd;
					isSet = true;
				}
			}
			if (mScale.mFrames.size() > 1)
			{
				float scaleEnd = mScale.mFrames[mScale.mFrames.size() - 1].mTime;
				if (scaleEnd > result || !isSet)
				{
					result = scaleEnd;
					isSet = true;
				}
			}
			return result;
		}

		bool IsValid() const
		{
			return mPosition.mFrames.size() > 1 || mScale.mFrames.size() > 1 || mRotation.mFrames.size() > 1;
		}
	};

	struct Clip
	{
		std::vector<TransformTrack> mTracks;
		std::string mAnimationName;
		float mStartTime;
		float mEndTime;

		TransformTrack& operator[](unsigned int joint)
		{
			unsigned int s = (unsigned int)mTracks.size();
			for (unsigned int i = 0; i < s; ++i)
			{
				if (mTracks[i].mJointID == joint)
				{
					return mTracks[i];
				}
			}

			mTracks.push_back(TransformTrack{});
			mTracks[mTracks.size() - 1].mJointID = joint;
			return mTracks[mTracks.size() - 1];
		}

		void RecalculateDuration()
		{
			mStartTime = 0.0f;
			mEndTime = 0.0f;
			bool startSet = false;
			bool endSet = false;
			unsigned int channelsSize = (unsigned int)mTracks.size();
			for (unsigned int i = 0; i < channelsSize; ++i)
			{
				if (mTracks[i].IsValid())
				{
					float startTime = mTracks[i].GetStartTime();
					float endTime = mTracks[i].GetEndTime();
					if (startTime < mStartTime || !startSet)
					{
						mStartTime = startTime;
						startSet = true;
					}
					if (endTime > mEndTime || !endSet)
					{
						mEndTime = endTime;
						endSet = true;
					}
				}
			}
		}
	};



	struct Mesh
	{
		uint64_t hashName = 0;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		//std::vector<BoneData> animVertices;
		int materialIndex = -1;
	};

	struct Model
	{
		fs::path binaryMaterialPath;
		std::string modelName;
		std::string originalPath;
		//std::string animationParentDirectory;
		glm::mat4 globalInverseTransform;

		std::vector<MaterialName> materialNames;
		std::vector<Mesh> meshes;
		
		std::vector<BoneData> boneData;
		std::vector<glm::mat4> boneInfo;
		//std::vector<Animation> animations;

		//std::unordered_map<uint64_t, int> boneMapping;

		Skeleton skeleton;
		std::vector<Clip> clips;
	};

	struct Sampler
	{
		uint32_t samplerIndex;
		std::string name;

		flat::MinTextureFilter minFilter;
		flat::MagTextureFilter magFilter;
		flat::TextureWrapMode wrapS;
		flat::TextureWrapMode wrapT;
		flat::TextureWrapMode wrapR;
	};

	struct Texture
	{
		std::string name;
		size_t samplerIndex;
		size_t imageIndex;
	};

	template<typename T>
	struct ShaderParameter
	{
		flat::ShaderPropertyType propertyType;
		T value;
	};

	struct AssetName
	{
		//@TODO(Serge): UUID
		uint64_t assetNameHash;
		std::string assetNameString;
	};

	struct TextureShaderParam
	{
		AssetName textureAssetName;
		flat::ShaderPropertyPackingType packingType;
		AssetName texturePackAssetName;
		size_t samplerIndex;
		float anisotropicFiltering;
	};

	struct ShaderStageParam
	{
		AssetName shader;
		AssetName shaderStageName;
		std::string value;
	};

	struct ShaderParams
	{
		std::vector<ShaderParameter<uint64_t>> ulongShaderParams;
		std::vector<ShaderParameter<float>> floatShaderParams;
		std::vector<ShaderParameter<bool>> boolShaderParams;
		std::vector<ShaderParameter<glm::vec4>> colorShaderParams;
		std::vector<ShaderParameter<std::string>> stringShaderParams;
		std::vector<ShaderParameter<TextureShaderParam>> textureShaderParams;
		std::vector<ShaderParameter<ShaderStageParam>> shaderStageParams;
	};

	struct ShaderEffect
	{
		AssetName assetName;
		AssetName staticShader;
		AssetName dynamicShader;
		flat::eVertexLayoutType staticVertexLayout;
		flat::eVertexLayoutType dynamicVertexLayout;
	};

	struct ShaderEffectPasses
	{
		std::vector<ShaderEffect> shaderEffects;
		ShaderParams defaultShaderParams;
	};

	//@TODO(Serge): UUID for all of the shader effects
	static ShaderEffect forwardOpaqueShaderEffect = {
		{STRING_ID("forward_opaque_shader_effect"), "forward_opaque_shader_effect"},
		{STRING_ID("Sandbox"), "Sandbox"},
		{STRING_ID("AnimModel"), "AnimModel"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	static ShaderEffect forwardTransparentShaderEffect = {
		{STRING_ID("forward_transparent_shader_effect"), "forward_transparent_shader_effect"},
		{STRING_ID("TransparentModel"), "TransparentModel"},
		{STRING_ID("TransparentAnimModel"), "TransparentAnimModel"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	static ShaderEffect depthShaderEffect = {
		{STRING_ID("depth_shader_effect"), "depth_shader_effect"},
		{STRING_ID("StaticDepth"), "StaticDepth"},
		{STRING_ID("DynamicDepth"), "DynamicDepth"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	static ShaderEffect directionShadowEffect = {
		{STRING_ID("direction_shadow_effect"), "direction_shadow_effect"},
		{STRING_ID("StaticShadowDepth"), "StaticShadowDepth"},
		{STRING_ID("DynamicShaderDepth"), "DynamicShaderDepth"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	static ShaderEffect pointShadowEffect = {
		{STRING_ID("point_shadow_effect"), "point_shadow_effect"},
		{STRING_ID("PointLightStaticShadowDepth"), "PointLightStaticShadowDepth"},
		{STRING_ID("PointLightDynamicShadowDepth"), "PointLightDynamicShadowDepth"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	static ShaderEffect spotLightShadowEffect = {
		{STRING_ID("spotlight_shadow_effect"), "spotlight_shadow_effect"},
		{STRING_ID("SpotLightStaticShadowDepth"), "SpotLightStaticShadowDepth"},
		{STRING_ID("SpotLightDynamicShadowDepth"), "SpotLightDynamicShadowDepth"},
		flat::eVertexLayoutType_VLT_DEFAULT_STATIC,
		flat::eVertexLayoutType_VLT_DEFAULT_DYNAMIC
	};

	//All of the names of the ShaderEffectPasses we have created
	namespace
	{
		const std::string OPAQUE_FORWARD_PASS = "OPAQUE_FORWARD_PASS";
		const std::string TRANSPARENT_FORWARD_PASS = "TRANSPARENT_FORWARD_PASS";
	}

	static std::unordered_map<std::string, ShaderEffectPasses> g_shaderEffectPassesMap =
	{
		{OPAQUE_FORWARD_PASS, {{forwardOpaqueShaderEffect, {}, depthShaderEffect, directionShadowEffect, pointShadowEffect, spotLightShadowEffect}, {}}},
		{TRANSPARENT_FORWARD_PASS, {{{}, forwardTransparentShaderEffect, {}, {}, {}, {}}, {}}}
	};

	struct MaterialData
	{
		MaterialName materialName;
		flat::eTransparencyType transparencyType;
		ShaderEffectPasses shaderEffectPasses;
		ShaderParams shaderParams;
	};

	//std::unordered_map<std::string, uint32_t> mBoneNecessityMap;
	//std::vector<Joint> mJoints;
	//std::unordered_map<std::string, Bone> mBoneMap;

	//aiNode* FindNodeInHeirarchy(const aiScene* scene, aiNode* node, const std::string& strName);
	//void MarkBonesInMesh(aiNode* node, const aiScene* scene, aiMesh* mesh);
	//void MarkBones(aiNode* node, const aiScene* scene);
	//void CreateBonesVector(const aiScene* scene, aiNode* node, int parentIndex);
	//glm::mat4 GetGlobalTransform(uint32_t i);
	//int FindJointIndex(const std::string& jointName);
	//void LoadBindPose(Model& model, Skeleton& skeleton);
	//void ProcessAnimNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene, Skeleton& skeleton, int parentDebugBone, uint32_t& numVertices);
	//void ProcessNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene);
	//void ProcessMeshForModel(Model& model, aiMesh* mesh, uint32_t meshIndex, const aiNode* node, const aiScene* scene);
	//void GetMeshData(aiNode* node, const aiScene* scene, uint64_t& numMeshes, uint64_t& numBones, uint64_t& numVertices);

	bool ConvertModelToFlatbuffer(Model& model, const fs::path& inputFilePath, const fs::path& outputPath, uint32_t numberOfSamples);

	//void ProcessMaterial(const std::string& modelName, aiMaterial* aiMaterial);
	
	bool ConvertGLTFModel(
		const std::filesystem::path& inputFilePath,
		const std::filesystem::path& parentOutputDir,
		const std::filesystem::path& binaryMaterialParamPacksManifestFile,
		const std::filesystem::path& rawMaterialsParentDirectory,
		const std::filesystem::path& engineTexturePacksManifestFile,
		const std::filesystem::path& texturePacksManifestFile,
		bool forceRebuild,
		uint32_t numAnimationSamples);

	//For loading the GLTF Skeleton
	Pose LoadRestPose(const fastgltf::Asset& gltf, const std::unordered_map<size_t, Transform>& nodeLocalTransforms);
	Pose LoadBindPose(const fastgltf::Asset& gltf, const Pose& restPose);
	std::vector<std::string> LoadJointNames(const fastgltf::Asset& gltf);
	using BoneMap = std::map<int, int>;
	BoneMap RearrangeSkeleton(Pose& restPose, Pose& bindPose, std::vector<std::string>& jointNames);
	BoneMap LoadSkeleton(const fastgltf::Asset& gltf, const std::unordered_map<size_t, Transform>& nodeLocalTransforms, Skeleton& skeleton);

	//For Loading GLTF Animation Clips
	std::vector<Clip> LoadAnimationClips(const fastgltf::Asset& gltf, BoneMap boneMap, unsigned int numSamples);








	//glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat)
	//{
	//	//result[col][row]
	//	//mat[row][col] a, b, c, d - rows
	//	//              1, 2, 3, 4 - cols
	//	glm::mat4 result = glm::mat4();

	//	result[0][0] = mat.a1; result[1][0] = mat.a2; result[2][0] = mat.a3; result[3][0] = mat.a4;
	//	result[0][1] = mat.b1; result[1][1] = mat.b2; result[2][1] = mat.b3; result[3][1] = mat.b4;
	//	result[0][2] = mat.c1; result[1][2] = mat.c2; result[2][2] = mat.c3; result[3][2] = mat.c4;
	//	result[0][3] = mat.d1; result[1][3] = mat.d2; result[2][3] = mat.d3; result[3][3] = mat.d4;

	//	return result;
	//}

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

	/*Transform AssimpMat4ToTransform(const aiMatrix4x4& mat)
	{
		return ToTransform(AssimpMat4ToGLMMat4(mat));
	}*/

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

	flat::MagTextureFilter GetMagFilterFromFastGLTF(fastgltf::Filter filter)
	{
		switch (filter)
		{
		case fastgltf::Filter::Nearest:
			return flat::MagTextureFilter_NEAREST;
		case fastgltf::Filter::Linear:
			return flat::MagTextureFilter_LINEAR;
		default:
			return flat::MagTextureFilter_NEAREST;
		};
	}

	flat::MinTextureFilter GetMinFilterFromFastGLTF(fastgltf::Filter filter)
	{
		switch (filter)
		{
		case fastgltf::Filter::Linear:
			return flat::MinTextureFilter_LINEAR;
		case fastgltf::Filter::LinearMipMapLinear:
			return flat::MinTextureFilter_LINEAR_MIPMAP_LINEAR;
		case fastgltf::Filter::LinearMipMapNearest:
			return flat::MinTextureFilter_LINEAR_MIPMAP_NEAREST;
		case fastgltf::Filter::Nearest:
			return flat::MinTextureFilter_NEAREST;
		case fastgltf::Filter::NearestMipMapLinear:
			return flat::MinTextureFilter_NEAREST_MIPMAP_LINEAR;
		case fastgltf::Filter::NearestMipMapNearest:
			return flat::MinTextureFilter_NEAREST_MIPMAP_NEAREST;
		default:
			return flat::MinTextureFilter_NEAREST;
		}
	}

	flat::TextureWrapMode GetWrapModeFromFastGLTF(fastgltf::Wrap wrapMode)
	{
		switch (wrapMode)
		{
		case fastgltf::Wrap::ClampToEdge:
			return flat::TextureWrapMode_CLAMP_TO_EDGE;
			break;
		case fastgltf::Wrap::MirroredRepeat:
			return flat::TextureWrapMode_MIRRORED_REPEAT;
			break;
		case fastgltf::Wrap::Repeat:
			return flat::TextureWrapMode_REPEAT;
			break;
		default:
			return flat::TextureWrapMode_REPEAT;
			break;
		}
	}

	Sampler ExtractSampler(size_t samplerIndex, const fastgltf::Sampler& fastgltfSampler)
	{
		Sampler newSampler;
		 
		newSampler.minFilter = GetMinFilterFromFastGLTF( fastgltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));
		newSampler.magFilter = GetMagFilterFromFastGLTF( fastgltfSampler.magFilter.value_or(fastgltf::Filter::Nearest));
		newSampler.wrapS = GetWrapModeFromFastGLTF(fastgltfSampler.wrapS);
		newSampler.wrapT = GetWrapModeFromFastGLTF(fastgltfSampler.wrapT);
		newSampler.wrapR = newSampler.wrapS;
		newSampler.name = fastgltfSampler.name;
		newSampler.samplerIndex = samplerIndex;

		return newSampler;
	}

	bool FindMaterialByMaterialName(const flat::MaterialPack* materialPack, const std::string& materialName, MaterialName& result)
	{

		//first do a simple thing which is to look for the material name in the materialPack
		const auto materials = materialPack->pack();
		const auto numMaterials = materials->size();

		std::string materialNameToCheck = materialName;

		std::transform(materialNameToCheck.begin(), materialNameToCheck.end(), materialNameToCheck.begin(),
			[](unsigned char c) { return std::tolower(c); });

		for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
		{
			const flat::Material* material = materials->Get(i);

			//set all lowercase

			std::string materialAssetName = material->assetName()->stringName()->str();
			std::transform(materialAssetName.begin(), materialAssetName.end(), materialAssetName.begin(),
				[](unsigned char c) { return std::tolower(c); });
			//@TODO(Serge): UUID
			if (materialAssetName == materialNameToCheck)
			{
				result.name = material->assetName()->assetName();
				result.materialNameStr = material->assetName()->stringName()->str();
				result.materialPackName = materialPack->assetName()->assetName();
				result.materialPackNameStr = materialPack->assetName()->stringName()->str();

				return true;
			}
		}

		return false;
	}

	MaterialName GetMaterialNameFromMaterialPack(const flat::MaterialPack* materialPack, const std::string& materialName, const std::filesystem::path& path)
	{
		MaterialName result;
		if (FindMaterialByMaterialName(materialPack, materialName, result))
		{
			return result;
		}

		const auto materials = materialPack->pack();
		const auto numMaterials = materials->size();

		bool found = false;
		std::filesystem::path diffuseTexturePath = path;

		diffuseTexturePath.make_preferred();

		fs::path diffuseTextureName = diffuseTexturePath.filename();

		diffuseTextureName.replace_extension(".rtex");
		std::string diffuseTextureNameStr = diffuseTextureName.string();

		std::transform(diffuseTextureNameStr.begin(), diffuseTextureNameStr.end(), diffuseTextureNameStr.begin(),
			[](unsigned char c) { return std::tolower(c); });


		//now look into all the materials and find the material that matches this texture name
		for (flatbuffers::uoffset_t i = 0; i < numMaterials && !found; ++i)
		{
			const flat::Material* material = materials->Get(i);

			const auto* textureParams = material->shaderParams()->textureParams();

			for (flatbuffers::uoffset_t t = 0; t < textureParams->size() && !found; ++t)
			{
				const flat::ShaderTextureParam* texParam = textureParams->Get(t);

				if (texParam->propertyType() == flat::ShaderPropertyType_ALBEDO)
				{
					auto packNameStr = texParam->texturePack()->stringName()->str();

					std::string textureNameWithParents = packNameStr + "/albedo/" + diffuseTextureNameStr;

					auto textureNameID = STRING_ID(textureNameWithParents.c_str());

					if (textureNameID == texParam->value()->assetName())
					{
						result.name = material->assetName()->assetName();
						result.materialNameStr = material->assetName()->stringName()->str();
						result.materialPackName = materialPack->assetName()->assetName();
						result.materialPackNameStr = materialPack->assetName()->stringName()->str();
						found = true;
						break;
					}
				}
			}
		}

		assert(found && "Couldn't find it");

		return result;
	}

	void PopulateShaderEffectPasses(MaterialData& materialData)
	{

		//@TODO(Serge): add more sophisticated material mapping - we don't really have many shader effect passes setup yet so keep it simple for now

		if (materialData.transparencyType == flat::eTransparencyType_OPAQUE || 
			materialData.transparencyType == flat::eTransparencyType_MASK) //for now we use the same ones
		{
			materialData.shaderEffectPasses = g_shaderEffectPassesMap[OPAQUE_FORWARD_PASS];
		}
		else if (materialData.transparencyType == flat::eTransparencyType_TRANSPARENT)
		{
			materialData.shaderEffectPasses = g_shaderEffectPassesMap[TRANSPARENT_FORWARD_PASS];
		}
		else
		{
			assert(false && "Should never happen");
		}
	}

	bool BuildShaderTextureParam(
		ShaderParameter<TextureShaderParam>& outTextureShaderParam,
		const Texture& texture,
		size_t samplerIndex,
		const flat::TexturePacksManifest* engineTexturePacksManifest,
		const flat::TexturePacksManifest* texturePacksManifest,
		bool replaceWithMissingTexture)
	{
		std::filesystem::path textureFileName = texture.name;

		outTextureShaderParam.value.samplerIndex = samplerIndex;
		outTextureShaderParam.value.anisotropicFiltering = 0.0f; //@TODO(Serge): dunno where to get this from yet

		//we need to find the texture in the texturePacksManifest - if replaceWithMissingTexture is true, then replace it with that instead
		const auto numTexturePacks = texturePacksManifest->texturePacks()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			const flat::TexturePack* texturePack = texturePacksManifest->texturePacks()->Get(i);

			const auto* texturesInTexturePack = texturePack->textures();
			const auto numTexturesInPack = texturesInTexturePack->size();

			for (flatbuffers::uoffset_t j = 0; j < numTexturesInPack; ++j)
			{
				const auto textureInPack = texturesInTexturePack->Get(j);
				std::filesystem::path rawPath = textureInPack->rawPath()->str();

				if (rawPath.filename() == textureFileName)
				{
					//we found it in our packs - populate the outTextureShaderParam

					//@TODO(Serge): UUID
					outTextureShaderParam.value.texturePackAssetName.assetNameHash = texturePack->assetName()->assetName();
					outTextureShaderParam.value.texturePackAssetName.assetNameString = texturePack->assetName()->stringName()->str();

					//@TODO(Serge): UUID
					outTextureShaderParam.value.textureAssetName.assetNameHash = textureInPack->assetName()->assetName();
					outTextureShaderParam.value.textureAssetName.assetNameString = textureInPack->assetName()->stringName()->str();

					return true;
				}
			}
		}

		if (replaceWithMissingTexture)
		{
			const auto numEngineTexturePacks = engineTexturePacksManifest->texturePacks()->size();

			for (flatbuffers::uoffset_t i = 0; i < numEngineTexturePacks; ++i)
			{
				const flat::TexturePack* texturePack = engineTexturePacksManifest->texturePacks()->Get(i);

				const auto* texturesInTexturePack = texturePack->textures();
				const auto numTexturesInPack = texturesInTexturePack->size();

				//@NOTE(Serge): hardcoding this for simplicity - don't think this will ever change... if it does then this needs to change
				if (texturePack->assetName()->stringName()->str() == "MissingTexture")
				{
					//@TODO(Serge): UUID
					outTextureShaderParam.value.texturePackAssetName.assetNameHash = texturePack->assetName()->assetName();
					outTextureShaderParam.value.texturePackAssetName.assetNameString = texturePack->assetName()->stringName()->str();

					const auto* missingTextureAssetRef = texturePack->textures()->Get(0);
					//@TODO(Serge): UUID
					outTextureShaderParam.value.textureAssetName.assetNameHash = missingTextureAssetRef->assetName()->assetName();
					outTextureShaderParam.value.textureAssetName.assetNameString = missingTextureAssetRef->assetName()->stringName()->str();

					return true;
				}
			}
		}

		return false;
	}

	int RunSystemCommand(const char* command)
	{
		int result = system(command);
		if (result != 0)
		{
			printf("Failed to run command: %s\n\n with result: %i\n", command, result);
		}

		return result;
	}

	const char PATH_SEPARATOR = '/';
	bool SanitizeSubPath(const char* rawSubPath, char* result)
	{
		size_t startingIndex = 0;
		//@TODO(Serge): do in a loop
		if (strlen(rawSubPath) > 2) {
			if (rawSubPath[0] == '.' && rawSubPath[1] == '.')
			{
				startingIndex = 2;
			}
			else if (rawSubPath[0] == '.' && rawSubPath[1] != '.')
			{
				startingIndex = 1;
			}
		}

		size_t len = strlen(rawSubPath);
		size_t resultIndex = 0;
		for (; startingIndex < len; startingIndex++)
		{
			if (PATH_SEPARATOR != '\\' && rawSubPath[startingIndex] == '\\')
			{
				result[resultIndex] = PATH_SEPARATOR;
			}
			else
			{
				result[resultIndex] = rawSubPath[startingIndex];
			}

			++resultIndex;
		}

		result[resultIndex] = '\0';

		return true;
	}

	bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
	{
		char command[2048];
		std::string flatc = R2_FLATC;

		std::filesystem::path flatCEXEPath = flatc;
		flatCEXEPath = flatCEXEPath.lexically_normal();

		char sanitizedFlatCEXEPath[256];

		SanitizeSubPath(flatCEXEPath.string().c_str(), sanitizedFlatCEXEPath);

		char sanitizedSourcePath[256];

		SanitizeSubPath(sourcePath.c_str(), sanitizedSourcePath);
#ifdef R2_PLATFORM_WINDOWS
		sprintf_s(command, Kilobytes(2), "%s -t -o %s %s -- %s --raw-binary", sanitizedFlatCEXEPath, outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
#else
		sprintf(command, "%s -t -o %s %s -- %s --raw-binary", sanitizedFlatCEXEPath, outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
#endif
		return RunSystemCommand(command) == 0;
	}

	bool ExtractMaterialDataFromFastGLTF(
		const std::string& meshName,
		size_t materialIndex,
		const fastgltf::Material& fastgltfMaterial,
		const flat::MaterialPack* materialPack,
		MaterialData& materialData,
		const std::vector<Texture>& textures,
		const flat::TexturePacksManifest* engineTexturePacksManifest,
		const flat::TexturePacksManifest* texturePacksManifest,
		bool forceRebuild)
	{
		//build the new material
		std::string materialNameString = fastgltfMaterial.name.c_str();
		char materialNameToUse[256];

		if (materialNameString.empty())
		{
			std::filesystem::path meshPathName = meshName;
			materialNameString = std::string(materialNameToUse) + "_material_" + std::to_string(materialIndex);
		}

		MakeAssetNameStringForFilePath(materialNameString.c_str(), materialNameToUse, r2::asset::MATERIAL);

		if (!forceRebuild && FindMaterialByMaterialName(materialPack, materialNameString, materialData.materialName))
		{
			//False here means it's not a new material - it came from materialPack
			return false;
		}

		//@TODO(Serge): UUID
		materialData.materialName.materialNameStr = materialNameToUse;
		materialData.materialName.name = STRING_ID(materialNameToUse);
		materialData.materialName.materialPackNameStr = materialPack->assetName()->stringName()->str();
		materialData.materialName.materialPackName = materialPack->assetName()->assetName();

		if (fastgltfMaterial.alphaMode == fastgltf::AlphaMode::Opaque)
		{
			materialData.transparencyType = flat::eTransparencyType_OPAQUE;
		}
		else if (fastgltfMaterial.alphaMode == fastgltf::AlphaMode::Blend)
		{
			materialData.transparencyType = flat::eTransparencyType_TRANSPARENT;
		}
		else
		{
			materialData.transparencyType = flat::eTransparencyType_OPAQUE; //this should be mask but...
		}
		
		ShaderParameter<float> alphaCutoffParam;

		alphaCutoffParam.propertyType = flat::ShaderPropertyType_ALPHA_CUTOFF;
		alphaCutoffParam.value = fastgltfMaterial.alphaCutoff;

		materialData.shaderParams.floatShaderParams.push_back(alphaCutoffParam);

		//first go with the PBR stuff
		ShaderParameter<glm::vec4> baseColorFactorParam;
		baseColorFactorParam.propertyType = flat::ShaderPropertyType_ALBEDO;
		baseColorFactorParam.value = glm::vec4(fastgltfMaterial.pbrData.baseColorFactor[0], fastgltfMaterial.pbrData.baseColorFactor[1], fastgltfMaterial.pbrData.baseColorFactor[2], fastgltfMaterial.pbrData.baseColorFactor[3]);
		materialData.shaderParams.colorShaderParams.push_back(baseColorFactorParam);

		ShaderParameter<float> metallicFactorParam;
		metallicFactorParam.propertyType = flat::ShaderPropertyType_METALLIC;
		metallicFactorParam.value = fastgltfMaterial.pbrData.metallicFactor;
		materialData.shaderParams.floatShaderParams.push_back(metallicFactorParam);

		ShaderParameter<float> roughnessFactorParam;
		roughnessFactorParam.propertyType = flat::ShaderPropertyType_ROUGHNESS;
		roughnessFactorParam.value = fastgltfMaterial.pbrData.roughnessFactor;
		materialData.shaderParams.floatShaderParams.push_back(roughnessFactorParam);

		if (fastgltfMaterial.pbrData.baseColorTexture.has_value())
		{
			//we have a texture 
			const auto& baseTextureInfo = fastgltfMaterial.pbrData.baseColorTexture.value();

			const Texture& texture = textures[baseTextureInfo.textureIndex];

			ShaderParameter<TextureShaderParam> textureShaderParam;
			bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, true);
			assert(created && "If we have a baseColorTexture then this should always return true");
			//set the property type and packing type directly here
			textureShaderParam.propertyType = flat::ShaderPropertyType_ALBEDO;
			textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

			materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
		}

		if (fastgltfMaterial.pbrData.metallicRoughnessTexture.has_value())
		{
			const auto& metallicRoughnessTextureInfo = fastgltfMaterial.pbrData.metallicRoughnessTexture.value();

			const Texture& texture = textures[metallicRoughnessTextureInfo.textureIndex];

			ShaderParameter<TextureShaderParam> textureShaderParam;
			bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

			if (created)
			{
				textureShaderParam.propertyType = flat::ShaderPropertyType_METALLIC;
				textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_B;

				materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);

				textureShaderParam.propertyType = flat::ShaderPropertyType_ROUGHNESS;
				textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_G;

				materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
			}
			else
			{
				printf("Material Creation - Metallic Roughness Texture - Error with creating the metallic roughness textures shader params - probably couldn't find the texture name in the pack!\n");
			}
		}

		if (fastgltfMaterial.normalTexture.has_value())
		{
			const auto& normalTextureInfo = fastgltfMaterial.normalTexture.value();

			const Texture& texture = textures[normalTextureInfo.textureIndex];

			ShaderParameter<TextureShaderParam> textureShaderParam;
			bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

			if (created)
			{
				textureShaderParam.propertyType = flat::ShaderPropertyType_NORMAL;
				textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

				materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
			}
			else
			{
				printf("Material Creation - Normal Texture - Error with creating the normal texture shader params - probably couldn't find the texture name in the pack!\n");
			}
		}

		if (fastgltfMaterial.occlusionTexture.has_value())
		{
			const auto& occlusionTextureInfo = fastgltfMaterial.occlusionTexture.value();

			const Texture& texture = textures[occlusionTextureInfo.textureIndex];

			ShaderParameter<TextureShaderParam> textureShaderParam;
			bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

			if (created)
			{
				textureShaderParam.propertyType = flat::ShaderPropertyType_AMBIENT_OCCLUSION;
				textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_R;

				materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
			}
			else
			{
				printf("Material Creation - Occlusion Texture - Error with creating the normal texture shader params - probably couldn't find the texture name in the pack!\n");
			}
		}

		if (fastgltfMaterial.emissiveTexture.has_value())
		{
			const auto& emissiveTextureInfo = fastgltfMaterial.emissiveTexture.value();

			const Texture& texture = textures[emissiveTextureInfo.textureIndex];

			ShaderParameter<TextureShaderParam> textureShaderParam;
			bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

			if (created)
			{
				textureShaderParam.propertyType = flat::ShaderPropertyType_EMISSION;
				textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

				materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
			}
			else
			{
				printf("Material Creation - Emissive Texture - Error with creating the emissive texture shader params - probably couldn't find the texture name in the pack!\n");
			}
		}
		
		if (fastgltfMaterial.anisotropy)
		{
			ShaderParameter<float> anisotropyStrengthParam;
			anisotropyStrengthParam.propertyType = flat::ShaderPropertyType_ANISOTROPY;
			anisotropyStrengthParam.value = fastgltfMaterial.anisotropy->anisotropyStrength;

			materialData.shaderParams.floatShaderParams.push_back(anisotropyStrengthParam);

			ShaderParameter<float> anisotropyRotationParam;
			anisotropyRotationParam.propertyType = flat::ShaderPropertyType_ANISOTROPY_DIRECTION;
			anisotropyRotationParam.value = fastgltfMaterial.anisotropy->anisotropyRotation;

			materialData.shaderParams.floatShaderParams.push_back(anisotropyRotationParam);

			if (fastgltfMaterial.anisotropy->anisotropyTexture.has_value())
			{
				const auto& anisotropyTextureInfo = fastgltfMaterial.anisotropy->anisotropyTexture.value();

				const Texture& texture = textures[anisotropyTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_ANISOTROPY;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - Anisotropy Texture - Error with creating the anisotropy texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}
		}

		if (fastgltfMaterial.clearcoat)
		{
			ShaderParameter<float> clearCoatFactorParam;
			clearCoatFactorParam.propertyType = flat::ShaderPropertyType_CLEAR_COAT;
			clearCoatFactorParam.value = fastgltfMaterial.clearcoat->clearcoatFactor;

			materialData.shaderParams.floatShaderParams.push_back(clearCoatFactorParam);

			ShaderParameter<float> clearCoatRoughnessFactorParam;
			clearCoatRoughnessFactorParam.propertyType = flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS;
			clearCoatRoughnessFactorParam.value = fastgltfMaterial.clearcoat->clearcoatRoughnessFactor;

			materialData.shaderParams.floatShaderParams.push_back(clearCoatRoughnessFactorParam);

			
			if (fastgltfMaterial.clearcoat->clearcoatTexture.has_value())
			{
				const auto& clearcoatTextureInfo = fastgltfMaterial.clearcoat->clearcoatTexture.value();

				const Texture& texture = textures[clearcoatTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_CLEAR_COAT;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - ClearCoat Texture - Error with creating the clearcoat texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}

			if (fastgltfMaterial.clearcoat->clearcoatNormalTexture.has_value())
			{
				const auto& clearcoatNormalTextureInfo = fastgltfMaterial.clearcoat->clearcoatNormalTexture.value();

				const Texture& texture = textures[clearcoatNormalTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_CLEAR_COAT_NORMAL;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - ClearCoat Normal Texture - Error with creating the clearcoat normal texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}

			if (fastgltfMaterial.clearcoat->clearcoatRoughnessTexture.has_value())
			{
				const auto& clearcoatTextureInfo = fastgltfMaterial.clearcoat->clearcoatRoughnessTexture.value();

				const Texture& texture = textures[clearcoatTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_G;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - ClearCoat Roughness Texture - Error with creating the clearcoat Roughness texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}
		}

		if (fastgltfMaterial.sheen)
		{
			ShaderParameter<glm::vec4> sheenColorParam;
			sheenColorParam.propertyType = flat::ShaderPropertyType_SHEEN_COLOR;
			sheenColorParam.value = glm::vec4(fastgltfMaterial.sheen->sheenColorFactor[0], fastgltfMaterial.sheen->sheenColorFactor[1], fastgltfMaterial.sheen->sheenColorFactor[2], 1.0f);

			materialData.shaderParams.colorShaderParams.push_back(sheenColorParam);

			ShaderParameter<float> sheenRoughnessFactorParam;
			sheenRoughnessFactorParam.propertyType = flat::ShaderPropertyType_SHEEN_ROUGHNESS;
			sheenRoughnessFactorParam.value = fastgltfMaterial.sheen->sheenRoughnessFactor;

			materialData.shaderParams.floatShaderParams.push_back(sheenRoughnessFactorParam);

			if (fastgltfMaterial.sheen->sheenColorTexture.has_value())
			{
				const auto& sheenColorTextureInfo = fastgltfMaterial.sheen->sheenColorTexture.value();

				const Texture& texture = textures[sheenColorTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_SHEEN_COLOR;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - Sheen Color Texture - Error with creating the sheen color texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}

			if (fastgltfMaterial.sheen->sheenRoughnessTexture.has_value())
			{
				const auto& sheenRoughnessTextureInfo = fastgltfMaterial.sheen->sheenRoughnessTexture.value();

				const Texture& texture = textures[sheenRoughnessTextureInfo.textureIndex];

				ShaderParameter<TextureShaderParam> textureShaderParam;
				bool created = BuildShaderTextureParam(textureShaderParam, texture, texture.samplerIndex, engineTexturePacksManifest, texturePacksManifest, false);

				if (created)
				{
					textureShaderParam.propertyType = flat::ShaderPropertyType_SHEEN_ROUGHNESS;
					textureShaderParam.value.packingType = flat::ShaderPropertyPackingType_RGBA;

					materialData.shaderParams.textureShaderParams.push_back(textureShaderParam);
				}
				else
				{
					printf("Material Creation - Sheen Roughness Texture - Error with creating the sheen roughness texture shader params - probably couldn't find the texture name in the pack!\n");
				}
			}
		}

		ShaderParameter<glm::vec4> emissiveFactor;
		emissiveFactor.propertyType = flat::ShaderPropertyType_EMISSION;
		emissiveFactor.value = glm::vec4(fastgltfMaterial.emissiveFactor[0], fastgltfMaterial.emissiveFactor[1], fastgltfMaterial.emissiveFactor[2], 1.0f);

		materialData.shaderParams.colorShaderParams.push_back(emissiveFactor);

		ShaderParameter<float> emissiveStrengthParam;
		emissiveStrengthParam.propertyType = flat::ShaderPropertyType_EMISSION_STRENGTH;
		emissiveStrengthParam.value = fastgltfMaterial.emissiveStrength.value_or(1.0f);

		materialData.shaderParams.floatShaderParams.push_back(emissiveStrengthParam);

		ShaderParameter<bool> doubleSidedParam;
		doubleSidedParam.propertyType = flat::ShaderPropertyType_DOUBLE_SIDED;
		doubleSidedParam.value = fastgltfMaterial.doubleSided;

		materialData.shaderParams.boolShaderParams.push_back(doubleSidedParam);

		ShaderParameter<bool> unlitParam;
		unlitParam.propertyType = flat::ShaderPropertyType_UNLIT;
		unlitParam.value = fastgltfMaterial.unlit;

		materialData.shaderParams.boolShaderParams.push_back(unlitParam);

		//somehow figure out which shader effects to use with this material
		PopulateShaderEffectPasses(materialData);

		//true here means we need to make the material
		return true;
	}

	void BuildNewMaterials(const std::filesystem::path& rawMaterialOutputPath, const std::filesystem::path& binaryMaterialParamPacksManifestFile, const std::vector<MaterialData>& materialsToBuild, const std::vector<Sampler>& samplers)
	{
		for (size_t i = 0; i < materialsToBuild.size(); ++i)
		{
			const MaterialData& materialData = materialsToBuild[i];

			flatbuffers::FlatBufferBuilder fbb;

			std::vector<flatbuffers::Offset<flat::ShaderEffect>> flatShaderEffects;

			const size_t numShaderEffects = materialData.shaderEffectPasses.shaderEffects.size();

			for (size_t j = 0; j < numShaderEffects; ++j)
			{
				const auto shaderEffect = materialData.shaderEffectPasses.shaderEffects[j];

				//@TODO(Serge): Shader effects should have AssetName - fix this in the UUID change
				auto flatShaderEffect = flat::CreateShaderEffect(
					fbb,
					shaderEffect.assetName.assetNameHash,
					fbb.CreateString(shaderEffect.assetName.assetNameString),
					shaderEffect.staticShader.assetNameHash,
					shaderEffect.dynamicShader.assetNameHash,
					shaderEffect.staticVertexLayout,
					shaderEffect.dynamicVertexLayout);

				flatShaderEffects.push_back(flatShaderEffect);
			}

			auto flatShaderEffectPasses = flat::CreateShaderEffectPasses(fbb, fbb.CreateVector(flatShaderEffects), 0);
			
			std::vector<flatbuffers::Offset<flat::ShaderULongParam>> shaderULongParams;
			std::vector<flatbuffers::Offset<flat::ShaderBoolParam>> shaderBoolParams;
			std::vector<flatbuffers::Offset<flat::ShaderFloatParam>> shaderFloatParams;
			std::vector<flatbuffers::Offset<flat::ShaderColorParam>> shaderColorParams;
			std::vector<flatbuffers::Offset<flat::ShaderTextureParam>> shaderTextureParams;
			std::vector<flatbuffers::Offset<flat::ShaderStringParam>> shaderStringParams;

			if (materialData.shaderParams.ulongShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.ulongShaderParams.size(); ++j)
				{
					const auto& ulongParam = materialData.shaderParams.ulongShaderParams[j];
					shaderULongParams.push_back(flat::CreateShaderULongParam(fbb, ulongParam.propertyType, ulongParam.value));
				}
			}

			if (materialData.shaderParams.boolShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.boolShaderParams.size(); ++j)
				{
					const auto& boolParam = materialData.shaderParams.boolShaderParams[j];
					shaderBoolParams.push_back(flat::CreateShaderBoolParam(fbb, boolParam.propertyType, boolParam.value));
				}
			}

			if (materialData.shaderParams.floatShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.floatShaderParams.size(); ++j)
				{
					const auto& floatParam = materialData.shaderParams.floatShaderParams[j];
					shaderFloatParams.push_back(flat::CreateShaderFloatParam(fbb, floatParam.propertyType, floatParam.value));
				}
			}

			if (materialData.shaderParams.colorShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.colorShaderParams.size(); ++j)
				{
					const auto& colorParam = materialData.shaderParams.colorShaderParams[j];
					flat::Colour color{ colorParam.value.r, colorParam.value.g, colorParam.value.b, colorParam.value.a };
					shaderColorParams.push_back(flat::CreateShaderColorParam(fbb, colorParam.propertyType, &color));
				}
			}

			if (materialData.shaderParams.textureShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.textureShaderParams.size(); ++j)
				{
					const auto& textureParam = materialData.shaderParams.textureShaderParams[j];
					const Sampler& sampler = samplers[textureParam.value.samplerIndex];

					auto textureAssetName = flat::CreateAssetName(fbb, 0, textureParam.value.textureAssetName.assetNameHash, fbb.CreateString(textureParam.value.textureAssetName.assetNameString));
					auto texturePackAssetName = flat::CreateAssetName(fbb, 0, textureParam.value.texturePackAssetName.assetNameHash, fbb.CreateString(textureParam.value.texturePackAssetName.assetNameString));

					shaderTextureParams.push_back(
						flat::CreateShaderTextureParam(
							fbb, 
							textureParam.propertyType,
							textureAssetName,
							textureParam.value.packingType,
							texturePackAssetName,
							sampler.minFilter,
							sampler.magFilter,
							textureParam.value.anisotropicFiltering,
							sampler.wrapS,
							sampler.wrapT,
							sampler.wrapR)
					);
				}
			}


			if (materialData.shaderParams.stringShaderParams.size() > 0)
			{
				for (size_t j = 0; j < materialData.shaderParams.stringShaderParams.size(); ++j)
				{
					const auto& stringParam = materialData.shaderParams.stringShaderParams[j];

					shaderStringParams.push_back(flat::CreateShaderStringParam(fbb, stringParam.propertyType, fbb.CreateString(stringParam.value)));
				}
			}

			auto flatShaderParams = flat::CreateShaderParams(
				fbb,
				fbb.CreateVector(shaderULongParams),
				fbb.CreateVector(shaderBoolParams),
				fbb.CreateVector(shaderFloatParams),
				fbb.CreateVector(shaderColorParams),
				fbb.CreateVector(shaderTextureParams),
				fbb.CreateVector(shaderStringParams),
				0); //@TODO(Serge): there are no ShaderStageParams yet

			auto materialAssetName = flat::CreateAssetName(fbb, 0, materialData.materialName.name, fbb.CreateString(materialData.materialName.materialNameStr));

			auto flatMaterial = flat::CreateMaterial(fbb, materialAssetName, materialData.transparencyType, flatShaderEffectPasses, flatShaderParams);

			fbb.Finish(flatMaterial);

			uint8_t* dataBuf = fbb.GetBufferPointer();
			size_t dataSize = fbb.GetSize();

			//Now write out the material file

			//first make the directory of the material
			std::filesystem::path materialDirectoryPath = rawMaterialOutputPath / materialData.materialName.materialNameStr;

			
			if (!std::filesystem::exists(materialDirectoryPath))
			{
				std::filesystem::create_directory(materialDirectoryPath);
			}

			std::filesystem::path materialBinPath = materialDirectoryPath / (materialData.materialName.materialNameStr + ".bin");

			char sanitizedMaterialPathPath[256];
			SanitizeSubPath(materialBinPath.string().c_str(), sanitizedMaterialPathPath);

			char sanitizedRawMaterialOutputPath[256];
			SanitizeSubPath(materialDirectoryPath.string().c_str(), sanitizedRawMaterialOutputPath);


			WriteFile(sanitizedMaterialPathPath, (char*)dataBuf, dataSize);

			std::filesystem::path flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
			flatbufferSchemaPath = flatbufferSchemaPath.lexically_normal();

			std::filesystem::path materialSchemaPath = std::filesystem::path(flatbufferSchemaPath) / "Material.fbs";

			bool generated = GenerateFlatbufferJSONFile(sanitizedRawMaterialOutputPath, materialSchemaPath.string().c_str(), sanitizedMaterialPathPath);
			
			assert(generated);
			
			if (std::filesystem::exists(sanitizedMaterialPathPath))
			{
				std::filesystem::remove(sanitizedMaterialPathPath);
			}

		}
		
		//assert(false && "TODO(Serge): Build the materials based on the materialsToBuild vector and the raw output path");
		
		
		//assert(false && "TODO(Serge): Rebuild the material manifest");
	}

	char* LoadAndReadFile(const std::filesystem::path& filePath)
	{
		DiskAssetFile diskFile;

		bool openResult = diskFile.OpenForRead(filePath.string().c_str());

		assert(openResult && "Couldn't open the file!");

		auto fileSize = diskFile.Size();

		char* theData = new char[fileSize];

		auto readSize = diskFile.Read(theData, fileSize);

		assert(readSize == fileSize && "Didn't read whole file");

		return theData;
	}


	int GetParentNodeIndex(const fastgltf::Asset& gltf, const fastgltf::Node* target)
	{
		if (target == nullptr)
		{
			return -1;
		}

		for (unsigned int j = 0; j < gltf.nodes.size(); ++j)
		{
			const fastgltf::Node& nextNode = gltf.nodes[j];

			for (unsigned int k = 0; k < nextNode.children.size(); ++k)
			{
				if (target == &(gltf.nodes[nextNode.children[k]]))
				{
					return (int)j;
				}
			}
		}

		return -1;
	}

	Pose LoadRestPose(const fastgltf::Asset& gltf, const std::unordered_map<size_t, Transform>& nodeLocalTransforms)
	{
		size_t boneCount = gltf.nodes.size();

		Pose result;
		result.joints.resize(boneCount);
		result.parents.resize(boneCount);

		for (size_t i = 0; i < boneCount; ++i)
		{
			result.joints[i] = nodeLocalTransforms.find(i)->second;
			result.parents[i] = GetParentNodeIndex(gltf, &gltf.nodes[i]);
		}

		return result;
	}

	Pose LoadBindPose(const fastgltf::Asset& gltf, const Pose& restPose)
	{
		Pose bindPose;

		size_t numBones = restPose.joints.size();
		std::vector<Transform> worldBindPose(numBones);

		for (size_t i = 0; i < numBones; ++i)
		{
			worldBindPose[i] = restPose.GetGlobalTransform(i);
		}

		size_t numSkins = gltf.skins.size();

		for (size_t i = 0; i < numSkins; ++i)
		{
			std::vector<glm::mat4> invBindMatrices;

			fastgltf::iterateAccessorWithIndex<glm::mat4>(gltf, gltf.accessors[gltf.skins[i].inverseBindMatrices.value()],
				[&](glm::mat4 v, size_t index) {
					invBindMatrices.push_back(v);
				});
			size_t numJoints = gltf.skins[i].joints.size();
			for (size_t j = 0; j < numJoints; ++j)
			{
				glm::mat4 invBindMatrix = invBindMatrices[j];
				glm::mat4 bindMatrix = glm::inverse(invBindMatrix);
				
				worldBindPose[gltf.skins[i].joints[j]] = ToTransform(bindMatrix);
			}
		}

		bindPose.Load(restPose);

		for (size_t i = 0; i < numBones; ++i)
		{
			Transform current = worldBindPose[i];

			int p = bindPose.parents[i];
			if (p >= 0)
			{
				const Transform& parent = worldBindPose[p];
				current = Combine(Inverse(parent), current);
			}

			bindPose.joints[i] = current;
		}


		return bindPose;
	}

	std::vector<std::string> LoadJointNames(const fastgltf::Asset& gltf)
	{
		size_t boneCount = gltf.nodes.size();

		std::vector<std::string> jointNames(boneCount);
		for (size_t i = 0; i < boneCount; ++i)
		{
			const fastgltf::Node& nextNode = gltf.nodes[i];
			if (nextNode.name.empty())
			{
				jointNames[i] = std::string("Joint_") + std::to_string(i);
			}
			else
			{
				jointNames[i] = nextNode.name;
			}
		}

		return jointNames;
	}

	BoneMap RearrangeSkeleton(Pose& restPose, Pose& bindPose, std::vector<std::string>& jointNames)
	{
		if (restPose.joints.empty())
		{
			return BoneMap();
		}

		size_t size = restPose.joints.size();
		std::vector<std::vector<int>> hierarchy(size);
		std::list<int> process;

		for (int i = 0; i < size; ++i)
		{
			int parent = restPose.parents[i];
			if (parent > 0)
			{
				hierarchy[parent].push_back(i);
			}
			else
			{
				process.push_back(i);
			}
		}

		BoneMap mapForward;
		BoneMap mapBackward;

		int index = 0;
		while (process.size() > 0)
		{
			int current = *process.begin();

			process.pop_front();
			std::vector<int>& children = hierarchy[current];
			auto numChildren = children.size();
			for (size_t i = 0; i < numChildren; ++i)
			{
				process.push_back(children[i]);
			}

			mapForward[index] = current;
			mapBackward[current] = index;
			index++;
		}

		mapForward[-1] = -1;
		mapBackward[-1] = -1;

		Pose newRestPose;
		newRestPose.joints.resize(size);
		newRestPose.parents.resize(size);
		Pose newBindPose;
		newBindPose.joints.resize(size);
		newBindPose.parents.resize(size);

		std::vector<std::string> newNames(size);
		for (size_t i = 0; i < size; ++i)
		{
			int thisBone = mapForward[i];
			newRestPose.joints[i] = restPose.joints[thisBone];
			newBindPose.joints[i] = bindPose.joints[thisBone];
			newNames[i] = jointNames[thisBone];

			int parent = mapBackward[bindPose.parents[thisBone]];
			newRestPose.parents[i] = parent;
			newBindPose.parents[i] = parent;
		}

		bindPose.Load(newBindPose);
		restPose.Load(newRestPose);

		return mapBackward;
	}

	BoneMap LoadSkeleton(const fastgltf::Asset& gltf, const std::unordered_map<size_t, Transform>& nodeLocalTransforms, Skeleton& skeleton)
	{
		Pose restPose = LoadRestPose(gltf, nodeLocalTransforms);
		Pose bindPose = LoadBindPose(gltf, restPose);
		std::vector<std::string> jointNames = LoadJointNames(gltf);

		auto boneMap = RearrangeSkeleton(restPose, bindPose, jointNames);

		skeleton.mRestPose.Load(restPose);
		skeleton.mBindPose.Load(bindPose);
		skeleton.mJointNameStrings.insert(skeleton.mJointNameStrings.end(), jointNames.begin(), jointNames.end());

		skeleton.UpdateInverseBindPose();

		//check for parents
		for (size_t i= 0; i < restPose.parents.size(); ++i)
		{
			assert(restPose.parents[i] == bindPose.parents[i] && "We're assuming this atm");
		}

		return boneMap;
	}
	
	void ParseAnimationChannelVec3(const fastgltf::Asset& gltf, const fastgltf::Animation& animation, const fastgltf::AnimationChannel& channel, VectorTrack& result)
	{
		const fastgltf::AnimationSampler& animationSampler = animation.samplers[channel.samplerIndex];

		//@TODO(Serge): change to flat version
		result.interpolation = animationSampler.interpolation;
		bool isSamplerCubic = result.interpolation == fastgltf::AnimationInterpolation::CubicSpline;

		std::vector<float> times;
		//inputAccessor is the times
		fastgltf::iterateAccessorWithIndex<float>(gltf, gltf.accessors[animationSampler.inputAccessor],
			[&](float v, size_t index) {
				times.push_back(v);
			});
		
		std::vector<glm::vec3> values;
		fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[animationSampler.outputAccessor],
			[&](glm::vec3 v, size_t index) {
				values.push_back(v);
			});

		auto numFrames = gltf.accessors[animationSampler.inputAccessor].count;
		result.mFrames.resize(numFrames);

		for (size_t i = 0; i < numFrames; ++i)
		{
			VectorFrame& frame = result.mFrames[i];
			frame.mTime = times[i];

			frame.mValue = values[i];
			frame.mIn = isSamplerCubic ? values[i] : glm::vec3(0);
			frame.mOut = isSamplerCubic ? values[i] : glm::vec3(0);
		}
	}

	void ParseAnimationChannelQuat(const fastgltf::Asset& gltf, const fastgltf::Animation& animation, const fastgltf::AnimationChannel& channel, QuatTrack& result)
	{
		const fastgltf::AnimationSampler& animationSampler = animation.samplers[channel.samplerIndex];

		//@TODO(Serge): change to flat version
		result.interpolation = animationSampler.interpolation;
		bool isSamplerCubic = result.interpolation == fastgltf::AnimationInterpolation::CubicSpline;

		std::vector<float> times;
		//inputAccessor is the times
		fastgltf::iterateAccessorWithIndex<float>(gltf, gltf.accessors[animationSampler.inputAccessor],
			[&](float v, size_t index) {
				times.push_back(v);
			});

		std::vector<glm::quat> values;
		fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[animationSampler.outputAccessor],
			[&](glm::vec4 v, size_t index) {
				values.push_back(glm::quat(v.w, v.x, v.y, v.x));
			});

		auto numFrames = gltf.accessors[animationSampler.inputAccessor].count;
		result.mFrames.resize(numFrames);

		for (size_t i = 0; i < numFrames; ++i)
		{
			QuatFrame& frame = result.mFrames[i];
			frame.mTime = times[i];

			frame.mValue = values[i];
			frame.mIn = isSamplerCubic ? values[i] : glm::quat(1, 0, 0, 0);
			frame.mOut = isSamplerCubic ? values[i] : glm::quat(1, 0, 0, 0);
		}
	}

	void RearrangeClip(Clip& clip, BoneMap& boneMap)
	{
		auto size = clip.mTracks.size();
		for (size_t i = 0; i < size; ++i)
		{
			int joint = clip.mTracks[i].mJointID;
			int newJoint = boneMap[joint];
			clip.mTracks[i].mJointID = newJoint;
		}
	}

	std::vector<Clip> LoadAnimationClips(const fastgltf::Asset& gltf, BoneMap boneMap, unsigned int numberOfSamples)
	{
		const auto numClips = gltf.animations.size();
		std::vector<Clip> result(numClips);

		for (size_t i = 0; i < numClips; ++i)
		{
			const auto& animation = gltf.animations[i];
			result[i].mAnimationName = animation.name;
			
			const auto numChannels = animation.channels.size();

			for (size_t j = 0; j < numChannels; ++j)
			{
				const auto& channel = animation.channels[j];

				if (channel.path == fastgltf::AnimationPath::Translation)
				{
					auto& positionChannel = result[i][channel.nodeIndex].mPosition;
					ParseAnimationChannelVec3(gltf, animation, channel, positionChannel);
					positionChannel.UpdateIndexLookupTable(numberOfSamples);
				}
				else if (channel.path == fastgltf::AnimationPath::Rotation)
				{
					auto& rotationChannel = result[i][channel.nodeIndex].mRotation;
					ParseAnimationChannelQuat(gltf, animation, channel, rotationChannel);
					rotationChannel.UpdateIndexLookupTable(numberOfSamples);
				}
				else if (channel.path == fastgltf::AnimationPath::Scale)
				{
					auto& scaleChannel = result[i][channel.nodeIndex].mScale;
					ParseAnimationChannelVec3(gltf, animation, channel, scaleChannel);
					scaleChannel.UpdateIndexLookupTable(numberOfSamples);
				}
			}

			if (!boneMap.empty())
			{
				RearrangeClip(result[i], boneMap);
			}
			
			result[i].RecalculateDuration();
		}

		return result;
	}

	std::vector<BoneData> RearrangeMesh(std::vector<BoneData>& animVertices, BoneMap& boneMap)
	{
		unsigned int size = animVertices.size();
		std::vector<BoneData> remappedVertices;
		remappedVertices.resize(size);

		for (unsigned int i = 0; i < size; ++i)
		{
			glm::ivec4 newJoint = glm::ivec4(
				boneMap[animVertices[i].boneIDs.x],
				boneMap[animVertices[i].boneIDs.y],
				boneMap[animVertices[i].boneIDs.z],
				boneMap[animVertices[i].boneIDs.w]);
			glm::vec4 weight = glm::vec4(animVertices[i].boneWeights.x, animVertices[i].boneWeights.y, animVertices[i].boneWeights.z, animVertices[i].boneWeights.w);

			remappedVertices[i] = { weight, newJoint };
		}

		return remappedVertices;
	}

	bool ConvertGLTFModel(const std::filesystem::path& inputFilePath,
		const std::filesystem::path& parentOutputDir,
		const std::filesystem::path& binaryMaterialParamPacksManifestFile,
		const std::filesystem::path& rawMaterialsParentDirectory,
		const std::filesystem::path& engineTexturePacksManifestFile,
		const std::filesystem::path& texturePacksManifestFile,
		bool forceRebuild,
		uint32_t numAnimationSamples)
	{
		fastgltf::Parser parser{};

		constexpr auto gltfOptions = 
			fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::AllowDouble |
			fastgltf::Options::LoadGLBBuffers |
			fastgltf::Options::DecomposeNodeMatrices |
			fastgltf::Options::LoadExternalBuffers;

		fastgltf::GltfDataBuffer data;
		data.loadFromFile(inputFilePath);

		fastgltf::Asset gltf;

		auto type = fastgltf::determineGltfFileType(&data);
		if (type == fastgltf::GltfType::glTF)
		{
			auto load = parser.loadGLTF(&data, inputFilePath.parent_path(), gltfOptions);
			if (load)
			{
				gltf = std::move(load.get());
			}
			else
			{
				printf("Failed to load glTF file: %s\n", inputFilePath.string().c_str());
				return false;
			}
		}
		else if (type == fastgltf::GltfType::GLB)
		{
			auto load = parser.loadBinaryGLTF(&data, inputFilePath.parent_path(), gltfOptions);
			if (load)
			{
				gltf = std::move(load.get());
			}
			else
			{
				printf("Failed to load glTF file: %s\n", inputFilePath.string().c_str());
				return false;
			}
		}
		else
		{
			printf("Failed to determine glTF container for path: %s\n", inputFilePath.string().c_str());
			return false;
		}

		Model model;
		model.binaryMaterialPath = binaryMaterialParamPacksManifestFile;
		model.originalPath = inputFilePath.string();
		model.globalInverseTransform = glm::mat4(1.0f); //@TODO(Serge): need to calculate for realz

		char modelAssetName[256];

		std::filesystem::path dstModelPath = inputFilePath;
		dstModelPath.replace_extension(RMDL_EXTENSION);
		r2::asset::MakeAssetNameStringForFilePath(dstModelPath.string().c_str(), modelAssetName, r2::asset::RMODEL);

		model.modelName = modelAssetName;

		std::vector<Sampler> samplers;
		std::vector<std::filesystem::path> images;
		std::vector<Texture> textures;
		std::vector<MaterialData> materialsToBuild;

		//load samplers
		for (size_t i = 0; i < gltf.samplers.size(); ++i)
		{
			const fastgltf::Sampler& sampler = gltf.samplers[i];
			samplers.push_back(ExtractSampler(i, sampler));
		}

		for (size_t i = 0; i < gltf.images.size(); ++i)
		{
			auto uri = std::get<fastgltf::sources::URI>(gltf.images[i].data).uri;
			std::filesystem::path imagePath = uri.fspath();
			images.push_back(imagePath);
		}

		for (size_t i = 0; i < gltf.textures.size(); ++i)
		{
			const fastgltf::Texture& fastgltfTexture = gltf.textures[i];

			Texture newTexture;
			newTexture.imageIndex = fastgltfTexture.imageIndex.value();
			newTexture.samplerIndex = fastgltfTexture.samplerIndex.value();

			newTexture.name = images[newTexture.imageIndex].filename().string();

			textures.push_back(newTexture);
		}

		{
			const char* materialManifestData = LoadAndReadFile(binaryMaterialParamPacksManifestFile);
			const auto* materialManifest = flat::GetMaterialPack(materialManifestData);

			const char* engineTextureManifestData = LoadAndReadFile(engineTexturePacksManifestFile);
			const auto* engineTextureManifest = flat::GetTexturePacksManifest(engineTextureManifestData);

			const char* textureManifestData = LoadAndReadFile(texturePacksManifestFile);
			const auto* textureManifest = flat::GetTexturePacksManifest(textureManifestData);

			for (size_t i = 0; i < gltf.materials.size(); ++i)
			{
				const fastgltf::Material& fastgltfMaterial = gltf.materials[i];

				MaterialData materialData;
				bool isNew = ExtractMaterialDataFromFastGLTF(model.modelName, i, fastgltfMaterial, materialManifest, materialData, textures, engineTextureManifest, textureManifest, forceRebuild);

				if (isNew)
				{
					materialsToBuild.push_back(materialData);
				}

				model.materialNames.push_back(materialData.materialName);
			}

			delete[] textureManifestData;
			delete[] engineTextureManifestData;
			delete[] materialManifestData;
		}

		std::unordered_map<size_t, Transform> meshLocalTransforms;
		std::unordered_map<size_t, size_t> meshToSkin;
		std::unordered_map<size_t, Transform> nodeLocalTransform;

		//go through all of the nodes to setup the correct transformations
		for (size_t i = 0; i < gltf.nodes.size(); ++i)
		{
			fastgltf::Node& node = gltf.nodes[i];
			
			Transform localTransform;

			fastgltf::Node::TRS fastgltfTRS = std::get<fastgltf::Node::TRS>(node.transform);

			glm::vec3 tl(fastgltfTRS.translation[0], fastgltfTRS.translation[1], fastgltfTRS.translation[2]);
			glm::quat rot(fastgltfTRS.rotation[3], fastgltfTRS.rotation[0], fastgltfTRS.rotation[1], fastgltfTRS.rotation[2]);
			glm::vec3 sc(fastgltfTRS.scale[0], fastgltfTRS.scale[1], fastgltfTRS.scale[2]);

			localTransform.position = tl;
			localTransform.rotation = rot;
			localTransform.scale = sc;

			nodeLocalTransform[i] = localTransform;

			if (node.meshIndex.has_value())
			{
				meshLocalTransforms[node.meshIndex.value()] = localTransform;
				meshToSkin[node.meshIndex.value()] = node.skinIndex.value();
			}
		}


		BoneMap boneRemapping;

		if (gltf.skins.size() > 0)
		{
			boneRemapping = LoadSkeleton(gltf, nodeLocalTransform, model.skeleton);
			model.boneInfo.clear();
			model.boneInfo.insert(model.boneInfo.end(), model.skeleton.mInvBindPose.begin(), model.skeleton.mInvBindPose.end());
		}
		
		if (gltf.animations.size() > 0)
		{
			model.clips = LoadAnimationClips(gltf, boneRemapping, numAnimationSamples);
		}

		for (size_t i = 0; i < gltf.meshes.size(); ++i)
		{
			const fastgltf::Mesh& fastgltfMesh = gltf.meshes[i];
			assert(fastgltfMesh.primitives.size() > 0 && "Empty Mesh?");
			
			glm::mat4 localTransformMat = TransformToMat4(meshLocalTransforms[i]);

			const fastgltf::Skin* skinForMesh = nullptr;
			if (gltf.skins.size() > 0)
			{
				skinForMesh = &gltf.skins[meshToSkin[i]];
			}

			for (size_t p = 0; p < fastgltfMesh.primitives.size(); ++p)
			{
				
				const fastgltf::Primitive& primitive = fastgltfMesh.primitives[p];
				assert(primitive.type == fastgltf::PrimitiveType::Triangles && "We only support Triangles at the moment!");

				Mesh nextMesh;

				nextMesh.materialIndex = primitive.materialIndex.value();
				nextMesh.hashName = r2::asset::GetAssetNameForFilePath(fastgltfMesh.name.c_str(), r2::asset::MESH);

				//load all of the indices
				{
					fastgltf::Accessor& indexaccessor = gltf.accessors[primitive.indicesAccessor.value()];
					nextMesh.indices.reserve(nextMesh.indices.size() + indexaccessor.count);

					fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor, [&](std::uint32_t idx)
					{
						nextMesh.indices.push_back(idx);
					});
				}

				//load vertex positions
				{
					fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->second];
					nextMesh.vertices.resize(posAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index)
						{
							Vertex newVertex;
							//We apply the local transform directly to the mesh - not sure if that's an issue but why would it be?
							newVertex.position = localTransformMat * glm::vec4(v, 1.0);
							newVertex.normal = glm::vec3(0, 0, 1);
							newVertex.tangent = glm::vec3(1, 0, 0);
							newVertex.texCoords = glm::vec3(0);

							nextMesh.vertices[index] = newVertex;
						});
				}

				//load vertex normals
				{
					auto normals = primitive.findAttribute("NORMAL");
					if (normals != primitive.attributes.end())
					{
						fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
							[&](glm::vec3 v, size_t index) {
								nextMesh.vertices[index].normal = glm::normalize(v);
							});
					}
					else
					{
						printf("NORMAL attribute not found!\n");
					}
				}

				//load tangents
				{
					auto tangents = primitive.findAttribute("TANGENT");
					if (tangents != primitive.attributes.end())
					{


						auto tangentsCount = gltf.accessors[(*tangents).second].count;
						
						fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangents).second],
							[&](glm::vec4 v, size_t index) {
								nextMesh.vertices[index].tangent = v.w * glm::vec3(v.x, v.y, v.z);
							});
					}
					else
					{
						printf("TANGENT attribute not found!\n");
					}
				}

				//load texture coordinates
				{
					auto uv = primitive.findAttribute("TEXCOORD_0");
					if (uv != primitive.attributes.end())
					{
						fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
							[&](glm::vec2 v, size_t index)
							{
								nextMesh.vertices[index].texCoords = glm::vec3(v.x, v.y, primitive.materialIndex.value());
							});
					}
					else
					{
						printf("TEXCOORD_0 attribute not found!\n");
					}
				}

				//load the weights
				std::vector<glm::vec4> weights;
				{
					auto accWeights = primitive.findAttribute("WEIGHTS_0");
					if (accWeights != primitive.attributes.end())
					{
						fastgltf::Accessor& weightsAccessor = gltf.accessors[primitive.findAttribute("WEIGHTS_0")->second];
						weights.resize(weightsAccessor.count);

						assert(weightsAccessor.componentType == fastgltf::ComponentType::Float);
						assert(weightsAccessor.type == fastgltf::AccessorType::Vec4);

						fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*accWeights).second],
							[&](glm::vec4 w, size_t index)
							{
								weights[index] = w;
							});
					}
					else
					{
						printf("WEIGHTS_0 attribute not found\n");
					}
				}

				std::vector<glm::ivec4> joints;
				//load the joints
				{
					auto accJoints = primitive.findAttribute("JOINTS_0");
					if (accJoints != primitive.attributes.end())
					{
						fastgltf::Accessor& jointAccessor = gltf.accessors[primitive.findAttribute("JOINTS_0")->second];
						joints.resize(jointAccessor.count);

			//			assert(jointAccessor.componentType == fastgltf::ComponentType::UnsignedShort);
						assert(jointAccessor.type == fastgltf::AccessorType::Vec4);

						fastgltf::iterateAccessorWithIndex<glm::uvec4>(gltf, gltf.accessors[(*accJoints).second],
							[&](glm::uvec4 j, size_t index)
							{
								joints[index] = glm::ivec4(j);
							});
					}
					else
					{
						printf("JOINTS_0 attribute not found\n");
					}
				}
				std::vector<BoneData> animVertices;
				assert(weights.size() == joints.size() && "should always be the case");
				for (size_t w = 0; w < weights.size(); ++w)
				{
					BoneData animVertex;

					animVertex.boneWeights = weights[w];

					animVertex.boneIDs = glm::ivec4(
						skinForMesh->joints[joints[w].x],
						skinForMesh->joints[joints[w].y],
						skinForMesh->joints[joints[w].z],
						skinForMesh->joints[joints[w].w]
					);

					animVertices.push_back(animVertex);
				}

				if (skinForMesh)
				{
					animVertices = RearrangeMesh(animVertices, boneRemapping);

					model.boneData.insert(model.boneData.end(), animVertices.begin(), animVertices.end());
				}

				model.meshes.push_back(nextMesh);
			}
		}

		BuildNewMaterials(rawMaterialsParentDirectory, binaryMaterialParamPacksManifestFile, materialsToBuild, samplers);
		
		//assert(false && "@TODO(Serge): Build the new RAnimation + RModel fbs and update how we do the ConvertModelToFlatbuffer for animation data");

		return ConvertModelToFlatbuffer(model, inputFilePath, parentOutputDir, numAnimationSamples);
	}

	bool ConvertModel(
		const std::filesystem::path& inputFilePath,
		const std::filesystem::path& parentOutputDir,
		const std::filesystem::path& binaryMaterialParamPacksManifestFile,
		const std::filesystem::path& rawMaterialsParentDirectory,
		const std::filesystem::path& engineTexturePacksManifestFile,
		const std::filesystem::path& texturePacksManifestFile,
		bool forceRebuild,
		uint32_t numAnimationSamples)
	{
		//mJoints.clear();
		//mBoneNecessityMap.clear();
		//mBoneMap.clear();

//		if (inputFilePath.extension().string() == ".gltf" || inputFilePath.extension().string() == ".glb")
		{
			return ConvertGLTFModel(inputFilePath, parentOutputDir, binaryMaterialParamPacksManifestFile, rawMaterialsParentDirectory, engineTexturePacksManifestFile, texturePacksManifestFile, forceRebuild, numAnimationSamples);
		}

	/*	Assimp::Importer import;

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
			std::string errString = std::string("Failed to load model: ") + inputFilePath.string() + std::string(" with error: ") + std::string( import.GetErrorString() );

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

		char modelAssetName[256];

		std::filesystem::path dstModelPath = inputFilePath;
		dstModelPath.replace_extension(RMDL_EXTENSION);
		r2::asset::MakeAssetNameStringForFilePath(dstModelPath.string().c_str(), modelAssetName, r2::asset::RMODEL);

		model.modelName = modelAssetName;
		printf("modelAssetName: %s\n", modelAssetName);

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

		if (!model.animationParentDirectory.empty())
		{
			for (const auto& dirEntry : std::filesystem::directory_iterator(model.animationParentDirectory))
			{
				Animation nextAnimation;

				LoadAnimationFromFile(dirEntry.path(), nextAnimation);

				model.animations.push_back(nextAnimation);
			}
		}

		return ConvertModelToFlatbuffer(model, inputFilePath, parentOutputDir);*/
	}

	/*void CreateBonesVector(const aiScene* scene, aiNode* node, int parentIndex)
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
	}*/

	//void GetMeshData(aiNode* node, const aiScene* scene, uint64_t& numMeshes, uint64_t& numBones, uint64_t& numVertices)
	//{
	//	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	//	{
	//		GetMeshData(node->mChildren[i], scene, numMeshes, numBones, numVertices);
	//	}

	//	numMeshes += node->mNumMeshes;

	//	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	//	{
	//		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

	//		numVertices += mesh->mNumVertices;

	//		if (mesh->mNumBones > 0)
	//		{
	//			numBones += mesh->mNumBones;
	//		}
	//	}
	//}

	//glm::mat4 GetGlobalTransform(uint32_t i)
	//{
	//	glm::mat4 result = mJoints[i].localTransform;

	//	for (int p = mJoints[i].parentIndex; p >= 0; p = mJoints[p].parentIndex)
	//	{
	//		result = mJoints[p].localTransform * result;
	//	}

	//	return result;
	//}

	//int FindJointIndex(const std::string& jointName)
	//{
	//	for (uint32_t i = 0; i < mJoints.size(); ++i)
	//	{
	//		if (mJoints[i].name == jointName)
	//		{
	//			return (int)i;
	//		}
	//	}

	//	return -1;
	//}

	//void LoadBindPose(Model& model, Skeleton& skeleton)
	//{

	//	//@TODO(Serge): Right now I don't think this is correct. It SEEMS correct since we can put the model into bind pose no problem BUT when it comes to animating the model, this messes up if we first put the model into bind pose which doesn't seems correct.
	//	//				This might be a weird artifact of ASSIMP since the rest pose is frame 1 of the animation. It might be that we can't load an animation as our "base model" as we're doing for the Skeleton Archer and Ellen. Interestingly, this works for the Micro Bat
	//	//				Could also be that our math is messed up somewhere but currently I don't see where....
	//	std::vector<glm::mat4> worldBindPos(mJoints.size());

	//	for (uint32_t i = 0; i < mJoints.size(); ++i)
	//	{
	//		worldBindPos[i] = GetGlobalTransform(i);
	//	}

	//	for (auto iter = mBoneMap.begin(); iter != mBoneMap.end(); iter++)
	//	{
	//		int jointIndex = FindJointIndex(iter->first);

	//		if (jointIndex >= 0)
	//		{
	//			worldBindPos[jointIndex] = glm::inverse(iter->second.invBindPoseMat);
	//		}
	//		else
	//		{
	//			assert(jointIndex != -1 && "Fail....");
	//		}
	//	}

	//	for (uint32_t i = 0; i < mJoints.size(); ++i)
	//	{
	//		glm::mat4 current = worldBindPos[i];

	//		int p = mJoints[i].parentIndex;
	//		if (p >= 0)
	//		{
	//			glm::mat4 parent = worldBindPos[p];
	//			current = glm::inverse(parent) * current;
	//		}

	//		skeleton.mBindPoseTransforms[i] = ToTransform(current);
	//	}
	//}

	//void PrintAllTexturesOfType(aiMaterial* material, aiTextureType type, const std::string& label)
	//{
	//	auto numTexturesOfType = material->GetTextureCount(type);
	//	printf("----------------------------------------------------------------------------\n\n");
	//	printf("%s: %u\n", label.c_str(), numTexturesOfType);
	//	for (unsigned int i = 0; i < numTexturesOfType; ++i)
	//	{
	//		aiString path;
	//		aiTextureMapping textureMapping;
	//		unsigned int uvIndex;
	//		ai_real blend;
	//		aiTextureOp textureOP;
	//		aiTextureMapMode textureMapMode;

	//		material->GetTexture(type, i, &path, &textureMapping, &uvIndex, &blend, &textureOP, &textureMapMode);
	//		printf("Texture index: %u\n", i);
	//		printf("Texture Path: %s\n", path.C_Str());
	//		std::string textureMappingStr = "";
	//		switch (textureMapping)
	//		{
	//		case aiTextureMapping_UV:
	//			textureMappingStr = "UV";
	//			break;
	//		case aiTextureMapping_SPHERE:
	//			textureMappingStr = "Sphere";
	//			break;
	//		case aiTextureMapping_CYLINDER:
	//			textureMappingStr = "Cylinder";
	//			break;
	//		case aiTextureMapping_BOX:
	//			textureMappingStr = "Box";
	//			break;
	//		case aiTextureMapping_PLANE:
	//			textureMappingStr = "Plane";
	//			break;
	//		case aiTextureMapping_OTHER:
	//			textureMappingStr = "Other";
	//			break;
	//		default:
	//			break;
	//		}
	//		printf("Texture Mapping: %s\n", textureMappingStr.c_str());
	//		printf("Texture UV Index: %u\n", uvIndex);
	//		printf("Texture Blend: %f\n", blend);
	//		std::string textureOpStr = "";
	//		switch (textureOP)
	//		{
	//		case aiTextureOp_Multiply:
	//			textureOpStr = "Multiply";
	//			break;
	//		case aiTextureOp_Add:
	//			textureOpStr = "Add";
	//			break;
	//		case aiTextureOp_Subtract:
	//			textureOpStr = "Subtract";
	//			break;
	//		case aiTextureOp_Divide:
	//			textureOpStr = "Divide";
	//			break;
	//		case aiTextureOp_SmoothAdd:
	//			textureOpStr = "Smooth Add";
	//			break;
	//		case aiTextureOp_SignedAdd:
	//			textureOpStr = "Signed Add";
	//			break;
	//		default:
	//			textureOpStr = "Not Specified";
	//			break;
	//		}

	//		printf("Texture Op: %s\n", textureOpStr.c_str());
	//		std::string textureMapModeStr = "";
	//		switch (textureMapMode)
	//		{
	//		case aiTextureMapMode_Wrap:
	//			textureMapModeStr = "Wrap";
	//			break;
	//		case aiTextureMapMode_Clamp:
	//			textureMapModeStr = "Clamp";
	//			break;
	//		case aiTextureMapMode_Decal:
	//			textureMapModeStr = "Decal";
	//			break;
	//		case aiTextureMapMode_Mirror:
	//			textureMapModeStr = "Mirror";
	//			break;
	//		default:
	//			break;

	//		}
	//		printf("Texture Wrap Mode: %s\n\n", textureMapModeStr.c_str());
	//		printf("----------------------------------------------------------------------------\n");
	//	}
	//}

	//void PrintAllAssimpMaterialInformation(aiMaterial* material)
	//{
	//	printf("===========================================================================\n\n");
	//	printf("Material Name: %s\n", material->GetName().C_Str());

	//	PrintAllTexturesOfType(material, aiTextureType_AMBIENT, "Num Ambient Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_AMBIENT_OCCLUSION, "Num Ambient Occlusion Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_DIFFUSE, "Num Diffuse Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_DIFFUSE_ROUGHNESS, "Num Diffuse Roughness Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_DISPLACEMENT, "Num Displacement Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_EMISSION_COLOR, "Num Emission Color Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_EMISSIVE, "Num Emissive Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_HEIGHT, "Num Height Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_LIGHTMAP, "Num Light Map Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_METALNESS, "Num Metalness Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_NORMALS, "Num Normal Map Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_OPACITY, "Num Opacity Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_REFLECTION, "Num Reflection Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_SHININESS, "Num Shininess Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_SPECULAR, "Num Specular Textures");
	//	PrintAllTexturesOfType(material, aiTextureType_UNKNOWN, "Num Unknown Textures");
	//	
	//	

	//	printf("Material Properties: \n");

	//	for (unsigned int p = 0; p < material->mNumProperties; ++p)
	//	{
	//		auto* aiProperty = material->mProperties[p];

	//		//This will skip all of the textures - if semantic != 0 then it's a texture property which we already did above
	//		if (aiProperty->mSemantic != 0)
	//		{
	//			continue;
	//		}

	//		int intValue = -1;
	//		ai_real realValue;
	//		double doubleValue;
	//		aiString stringValue;
	//		aiColor3D color3DValue;
	//		aiColor4D color4DValue;
	//		aiUVTransform uvTransform;

	//		std::string propertyTypeStr = "Completely Unknown";

	//		switch (aiProperty->mType)
	//		{
	//		case aiPTI_Float:
	//		{
	//			propertyTypeStr = "Float";

	//			unsigned int numValues = aiProperty->mDataLength / sizeof(ai_real);
	//			ai_real* values = (ai_real*)aiProperty->mData;

	//			if (numValues == 1)
	//			{
	//				printf("Float property: %s, value: %f\n", aiProperty->mKey.C_Str(), *values);
	//			}
	//			else if (numValues == 2)
	//			{
	//				printf("Vec2 Property: %s, value - x: %f, y: %f\n", aiProperty->mKey.C_Str(), values[0], values[1]);
	//			}
	//			else if (numValues == 3)
	//			{
	//				printf("Color 3D Property: %s, value - r: %f, g: %f, b: %f\n", aiProperty->mKey.C_Str(), values[0], values[1], values[2]);
	//			}
	//			else if (numValues == 4)
	//			{
	//				printf("Color 4D Property: %s, value - r: %f, g: %f, b: %f, a: %f\n", aiProperty->mKey.C_Str(), values[0], values[1], values[2], values[3]);
	//			}
	//		}
	//			break;
	//		case aiPTI_Double:

	//			propertyTypeStr = "Double";
	//			material->Get<double>(aiProperty->mKey.C_Str(), 0, 0, doubleValue);
	//			printf("Float Property: %s, value: %f\n", aiProperty->mKey.C_Str(), doubleValue);

	//			break;
	//		case aiPTI_String: 

	//			propertyTypeStr = "String";
	//			material->Get(aiProperty->mKey.C_Str(), 0, 0, stringValue);
	//			printf("String Property: %s, value: %s\n", aiProperty->mKey.C_Str(), stringValue.C_Str());

	//			break;
	//		case aiPTI_Integer:
	//		{
	//			propertyTypeStr = "Integer";
	//			
	//			unsigned int numValues = aiProperty->mDataLength / sizeof(int);
	//			int* values = (int*)aiProperty->mData;

	//			if (numValues == 1)
	//			{
	//				printf("Int property: %s, value: %i\n", aiProperty->mKey.C_Str(), *values);
	//			}
	//			else if (numValues == 2)
	//			{
	//				printf("Int2 Property: %s, value - x: %i, y: %i\n", aiProperty->mKey.C_Str(), values[0], values[1]);
	//			}
	//			else if (numValues == 3)
	//			{
	//				printf("Int3 Property: %s, value - r: %i, g: %i, b: %i\n", aiProperty->mKey.C_Str(), values[0], values[1], values[2]);
	//			}
	//			else if (numValues == 4)
	//			{
	//				printf("Int4 Property: %s, value - r: %i, g: %i, b: %i, a: %i\n", aiProperty->mKey.C_Str(), values[0], values[1], values[2], values[3]);
	//			}
	//		}
	//			break;
	//		case aiPTI_Buffer:
	//		{
	//			propertyTypeStr = "Buffer";

	//			if (std::string(aiProperty->mKey.C_Str()) == "$mat.twosided")
	//			{
	//				bool twoSided =  (bool)*aiProperty->mData;
	//				printf("Buffer Property type: %s - %i\n", aiProperty->mKey.C_Str(), twoSided);
	//			}
	//		}
	//			break;
	//		default:
	//			printf("Property type: %s - UNKNOWN\n", aiProperty->mKey.C_Str());
	//			break;
	//		}
	//	}

	//	printf("===========================================================================\n\n");
	//}

	//void ProcessMeshForModel(Model& model, aiMesh* mesh, uint32_t meshIndex, const aiNode* node, const aiScene* scene)
	//{
	//	Mesh nextMesh;


	//	//@NOTE: I think this is still wrong - we're not weighting the mesh vertices based on the bone influences (if that matters)

	//	//@NOTE: What this is doing, for the mesh we're looking at we take the local transform up to the first bone we find, then we multiply that transforms by the bind pose of the bone
	//	//		 Not sure. I think the mTransformation matrices for the meshes remain the same (as the bind pose) up until they encounter the bone which is why this works
	//	glm::mat4 localTransform = AssimpMat4ToGLMMat4(node->mTransformation);
	//	{
	//		const aiNode* nextNode = node->mParent;

	//		glm::mat4 bindMat = glm::mat4(1.0f);

	//		aiNode* foundNode = nullptr;
	//		while (nextNode)
	//		{
	//			auto iter = mBoneMap.find(nextNode->mName.C_Str());

	//			if (iter != mBoneMap.end())
	//			{
	//				glm::mat4 temp = iter->second.invBindPoseMat;

	//				bindMat = glm::inverse(temp);

	//				break;
	//			}
	//			else
	//			{
	//				localTransform = AssimpMat4ToGLMMat4(nextNode->mTransformation) * localTransform;
	//			}

	//			nextNode = nextNode->mParent;
	//		}

	//		localTransform = bindMat * localTransform * AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation);
	//	}

	//	nextMesh.vertices.reserve(mesh->mNumVertices);


	//	if (mesh->mNumFaces > 0)
	//	{
	//		nextMesh.indices.reserve(mesh->mNumFaces * 3);
	//	}

	//	nextMesh.hashName = r2::asset::GetAssetNameForFilePath(mesh->mName.C_Str(), r2::asset::MESH);

	//	MaterialName materialName;

	//	int materialIndex = -1;

	//	//@NOTE(Serge): change later 
	//	if (mesh->mMaterialIndex >= 0)
	//	{
	//		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	//		assert(material != nullptr && "Material is null!");

	//		//@TEMPORARY - We're going to look at all the material params and see what gets packaged into them

	//	//	PrintAllAssimpMaterialInformation(material);



	//		if (material)
	//		{

	//		//	ProcessMaterial(mesh->mName.C_Str(), material);

	//			const uint32_t numTextures = aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);

	//			DiskAssetFile diskFile;

	//			bool openResult = diskFile.OpenForRead(model.binaryMaterialPath.string().c_str());

	//			assert(openResult && "Couldn't open the file!");

	//			auto fileSize = diskFile.Size();

	//			char* materialManifestData = new char[fileSize];

	//			auto readSize = diskFile.Read(materialManifestData, fileSize);

	//			assert(readSize == fileSize && "Didn't read whole file");

	//			const auto materialManifest = flat::GetMaterialPack(materialManifestData);//flat::GetMaterialParamsPack(materialManifestData);
	//			
	//			if (numTextures > 0)
	//			{
	//				for (int i = 0; i < numTextures; ++i)
	//				{
	//					aiString diffuseStr;

	//					material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseStr);

	//					fs::path diffuseTexturePath = diffuseStr.C_Str();

	//					diffuseTexturePath.make_preferred();

	//					fs::path diffuseTextureName = diffuseTexturePath.filename();

	//					diffuseTextureName.replace_extension(".rtex");
	//					std::string diffuseTextureNameStr = diffuseTextureName.string();

	//					std::transform(diffuseTextureNameStr.begin(), diffuseTextureNameStr.end(), diffuseTextureNameStr.begin(),
	//						[](unsigned char c) { return std::tolower(c); });

	//					//now look into all the materials and find the material that matches this texture name


	//					//@TODO(Serge): we've put the texture pack name into the material for each texture (we only need the diffuse/albedo texture)
	//					//				thus we need to build the texture name like: texturePackNameStr() / albedo / diffuseTextureName

	//					const auto materials = materialManifest->pack();

	//					const auto numMaterials = materials->size();//materialManifest->pack()->size();
	//					bool found = false;

	//					for (flatbuffers::uoffset_t i = 0; i < numMaterials && !found; ++i)
	//					{
	//					//	const flat::MaterialParams* materialParams = materialManifest->pack()->Get(i);

	//						const flat::Material* material = materials->Get(i);


	//						const auto* textureParams = material->shaderParams()->textureParams();

	//						for (flatbuffers::uoffset_t t = 0; t < textureParams->size() && !found; ++t)
	//						{
	//							//const flat::MaterialTextureParam* texParam = materialParams->textureParams()->Get(t);
	//							const flat::ShaderTextureParam* texParam = textureParams->Get(t);


	//							if (texParam->propertyType() == flat::ShaderPropertyType_ALBEDO)
	//							{
	//								auto packNameStr = texParam->texturePack()->stringName()->str();//texParam->texturePackNameStr()->str();
	//								
	//								std::string textureNameWithParents = packNameStr + "/albedo/" + diffuseTextureNameStr;

	//								auto textureNameID = STRING_ID(textureNameWithParents.c_str());

	//								if (textureNameID == texParam->value()->assetName())
	//								{
	//									materialName.name = material->assetName()->assetName();//materialParams->name();
	//									materialName.materialNameStr = material->assetName()->stringName()->str();
	//									materialName.materialPackName = materialManifest->assetName()->assetName();//materialManifest->name();
	//									materialName.materialPackNameStr = materialManifest->assetName()->stringName()->str();
	//									found = true;
	//									break;
	//								}
	//							}
	//						}
	//					}

	//					assert(found && "Couldn't find it");
	//				}
	//			}
	//			else
	//			{
	//				const char* matName = material->GetName().C_Str();
	//				materialName.name = STRING_ID(matName);
	//				materialName.materialNameStr = matName;
	//				materialName.materialPackName = materialManifest->assetName()->assetName();//materialManifest->name();
	//				materialName.materialPackNameStr = materialManifest->assetName()->stringName()->str();
	//			}


	//			delete[] materialManifestData;
	//			//r2::draw::MaterialHandle materialHandle = {};

	//			//if (numTextures > 0)
	//			//{
	//			//	//find the first one that isn't invalid
	//			//	for (int i = 0; i < numTextures && r2::draw::mat::IsInvalidHandle(materialHandle); ++i)
	//			//	{
	//			//		//@TODO(Serge): check other texture types if we don't find this one
	//			//		//@TODO(Serge): what do we do if we don't have any textures?
	//			//		aiString diffuseStr;

	//			//		material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseStr);


	//			//		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
	//			//		char textureName[r2::fs::FILE_PATH_LENGTH];
	//			//		r2::fs::utils::SanitizeSubPath(diffuseStr.C_Str(), sanitizedPath);

	//			//		r2::fs::utils::CopyFileNameWithExtension(sanitizedPath, textureName);

	//			//		char extType[r2::fs::FILE_PATH_LENGTH];

	//			//		//@TODO(Serge): clean up this MAJOR HACK!
	//			//		if (r2::fs::utils::CopyFileExtension(textureName, extType))
	//			//		{
	//			//			if (strcmp(extType, RTEX_EXT.c_str()) != 0)
	//			//			{
	//			//				r2::fs::utils::SetNewExtension(textureName, RTEX_EXT.c_str());
	//			//			}
	//			//		}

	//			//		r2::draw::tex::Texture tex;
	//			//		materialHandle = r2::draw::matsys::FindMaterialFromTextureName(textureName, tex);
	//			//	}
	//			//}
	//			//else
	//			//{
	//			//	const char* matName = material->GetName().C_Str();

	//			//	auto matNameID = STRING_ID(matName);

	//			//	materialHandle = r2::draw::matsys::FindMaterialHandle(matNameID);
	//			//}

	//			if (materialName.name != 0 && materialName.materialPackName != 0)//materialName != 0 && materialPackName != 0)
	//			{
	//				bool found = false;

	//				auto numMaterialHandles = model.materialNames.size();

	//				for (uint64_t i = 0; i < numMaterialHandles; ++i)
	//				{
	//					auto nextMatHandle = model.materialNames[i];

	//					if (nextMatHandle.name == materialName.name && nextMatHandle.materialPackName == materialName.materialPackName)
	//					{
	//						materialIndex = i;
	//						found = true;
	//						break;
	//					}
	//				}

	//				if (!found)
	//				{
	//					materialIndex = numMaterialHandles;
	//					model.materialNames.push_back(materialName);
	//				}
	//			}
	//			else
	//			{
	//				assert(false && "Failed to load the material for the mesh");
	//			}

	//		}
	//	}

	//	assert(materialIndex != -1 && "We should have a material for the mesh!");

	//	assert(materialName.name != 0 && "We should have a proper material");
	//	assert(materialName.materialPackName != 0 && "We should have a proper material");

	//	nextMesh.materialIndex = materialIndex;

	//	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	//	{
	//		Vertex nextVertex;

	//		nextVertex.position = localTransform * glm::vec4(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), 1.0);

	//		if (mesh->mNormals)
	//		{
	//			nextVertex.normal = glm::mat3(glm::transpose(glm::inverse(localTransform))) * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
	//		}

	//		if (mesh->mTextureCoords)
	//		{
	//			nextVertex.texCoords = glm::vec3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, materialIndex);
	//		}

	//		if (mesh->mTangents)
	//		{
	//			nextVertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
	//			//		printf("nextVertex.tangent - x: %f, y: %f, z: %f\n", nextVertex.tangent.x, nextVertex.tangent.y, nextVertex.tangent.y);
	//		}

	//		//if (mesh->mBitangents)
	//		//{
	//		//	nextVertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
	//		//}
	//		nextMesh.vertices.push_back(nextVertex);
	//	}

	//	//u32 indexOffsetToUse = indexOffset;
	//	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	//	{
	//		aiFace face = mesh->mFaces[i];
	//		for (uint32_t j = 0; j < face.mNumIndices; ++j)
	//		{
	//			uint32_t nextIndex = face.mIndices[j];
	//			//	printf("face.mIndices[j] + indexOffsetToUse: %zu\n", nextIndex);
	//			//r2::sarr::Push(*nextMeshPtr->optrIndices, nextIndex);
	//			nextMesh.indices.push_back(nextIndex);
	//			//if (nextIndex > indexOffset)
	//			//{
	//			//	indexOffset = nextIndex;
	//			//}
	//		}

	//	}

	//	//		indexOffset++;

	//	model.meshes.push_back(nextMesh);
	//	//r2::sarr::Push(*model.optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
	//}

	//void ProcessNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene)
	//{
	//	//process all of the node's meshes
	//	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	//	{
	//		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

	//		ProcessMeshForModel(model, mesh, meshIndex, node, scene);

	//		++meshIndex;
	//	}

	//	//then do the same for each of its children
	//	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	//	{
	//		ProcessNode(model, node->mChildren[i], meshIndex, scene);
	//	}
	//}

	//void ProcessBones(Model& model, uint32_t baseVertex, const aiMesh* mesh, const aiNode* node, const aiScene* scene)
	//{
	//	const aiMesh* meshToUse = mesh;
	//	//printf("Num bones: %zu\n", meshToUse->mNumBones);

	//	for (uint32_t i = 0; i < meshToUse->mNumBones; ++i)
	//	{
	//		int boneIndex = -1;
	//		std::string boneNameStr = std::string(meshToUse->mBones[i]->mName.data);
	//		//		printf("boneNameStr name: %s\n", boneNameStr.c_str());

	//		uint64_t boneName = STRING_ID(meshToUse->mBones[i]->mName.C_Str());

	//		//int theDefault = -1;
	//		//s32 result = r2::shashmap::Get(*model.boneMapping, boneName, theDefault);


	//		auto result = model.boneMapping.find(boneName);

	//		if (result == model.boneMapping.end())
	//		{
	//			boneIndex = model.boneInfo.size();//(u32)r2::sarr::Size(*model.boneInfo);
	//			BoneInfo info;
	//			info.offsetTransform = AssimpMat4ToGLMMat4(meshToUse->mBones[i]->mOffsetMatrix);
	//			model.boneInfo.push_back(info);

	//			model.boneMapping[boneName] = boneIndex;
	//		}
	//		else
	//		{
	//			boneIndex = result->second;
	//		}

	//		//	printf("boneIndex: %i\n", boneIndex);

	//		size_t numWeights = meshToUse->mBones[i]->mNumWeights;

	//		for (uint32_t j = 0; j < numWeights; ++j)
	//		{
	//			uint32_t vertexID = baseVertex + meshToUse->mBones[i]->mWeights[j].mVertexId;

	//			float currentWeightTotal = 0.0f;
	//			for (uint32_t k = 0; k < MAX_BONE_WEIGHTS; ++k)
	//			{
	//				currentWeightTotal += model.boneData[vertexID].boneWeights[k];

	//				if (!NearEq(currentWeightTotal, 1.0f) && NearEq(model.boneData[vertexID].boneWeights[k], 0.0f))
	//				{
	//					model.boneData[vertexID].boneIDs[k] = boneIndex;
	//					model.boneData[vertexID].boneWeights[k] = meshToUse->mBones[i]->mWeights[j].mWeight;
	//					break;
	//				}
	//			}
	//		}
	//	}

	//	if (meshToUse->mNumBones == 0)
	//	{
	//		//manually find the bone that goes with this guy
	//		const aiNode* nodeToUse = node->mParent;

	//		while (nodeToUse)
	//		{
	//			uint64_t boneName = STRING_ID(nodeToUse->mName.C_Str());
	//			
	//			//s32 boneIndex = r2::shashmap::Get(*model.boneMapping, boneName, theDefault);

	//			auto boneIter = model.boneMapping.find(boneName);

	//			if (boneIter != model.boneMapping.end())
	//			{
	//				for (uint64_t i = 0; i < meshToUse->mNumVertices; ++i)
	//				{
	//					uint32_t vertexID = baseVertex + i;

	//					model.boneData[vertexID].boneIDs[0] = boneIter->second;
	//					model.boneData[vertexID].boneWeights[0] = 1.0;
	//				}

	//				break;
	//			}
	//			else
	//			{
	//				nodeToUse = nodeToUse->mParent;
	//			}
	//		}
	//	}
	//}

	//void ProcessAnimNode(Model& model, aiNode* node, uint32_t& meshIndex, const aiScene* scene, Skeleton& skeleton, int parentDebugBone, uint32_t& numVertices)
	//{
	//	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	//	{
	//		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

	//		ProcessMeshForModel(model, mesh, meshIndex, node, scene);

	//		ProcessBones(model, numVertices, mesh, node, scene);

	//		numVertices += mesh->mNumVertices;

	//		++meshIndex;
	//	}

	//	auto iter = mBoneNecessityMap.find(node->mName.C_Str());
	//	if (iter != mBoneNecessityMap.end())
	//	{
	//		skeleton.mJointNames.push_back(STRING_ID(node->mName.C_Str()));
	//		skeleton.mRestPoseTransforms.push_back(AssimpMat4ToTransform(node->mTransformation));

	//		int jointIndex = (int)skeleton.mJointNames.size() - 1;

	//		const Joint& joint = mJoints[jointIndex];

	//		skeleton.mParents.push_back(joint.parentIndex);

	//		//#ifdef R2_DEBUG
	//		//			skeleton.mDebugBoneNames.push_back(node->mName.C_Str());
	//		//#endif // R2_DEBUG

	//		auto realBoneIter = mBoneMap.find(node->mName.C_Str());
	//		if (realBoneIter != mBoneMap.end())
	//		{
	//		//	r2::sarr::At(*skeleton.mRealParentBones, jointIndex) = parentDebugBone;

	//			skeleton.mRealParentBones[jointIndex] = parentDebugBone;

	//			parentDebugBone = jointIndex;
	//		}
	//	}

	//	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	//	{
	//		ProcessAnimNode(model, node->mChildren[i], meshIndex, scene, model.skeleton, parentDebugBone, numVertices);
	//	}
	//}

	flat::InterpolationType GetInterpolationType(fastgltf::AnimationInterpolation fastgltfInterpolationType)
	{
		switch (fastgltfInterpolationType)
		{
		case fastgltf::AnimationInterpolation::Linear:
			return flat::InterpolationType_LINEAR;

		case fastgltf::AnimationInterpolation::Step:
			return flat::InterpolationType_CONSTANT;

		case fastgltf::AnimationInterpolation::CubicSpline:
			return flat::InterpolationType_CUBIC;

		default:
			return flat::InterpolationType_LINEAR;
		}
	}

	bool ConvertModelToFlatbuffer(Model& model, const fs::path& inputFilePath, const fs::path& outputPath, uint32_t numberOfSamples )
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

		//Make the animation meta data here
		std::vector<flatbuffers::Offset<flat::RAnimationMetaData>> animationsMetaData;
		animationsMetaData.reserve(model.clips.size());

		char animationName[256];

		for (const auto& clip : model.clips)
		{
			std::vector<flatbuffers::Offset<flat::RChannelMetaData>> channelMetaDatas;
			channelMetaDatas.reserve(clip.mTracks.size());
			for (const auto& track : clip.mTracks)
			{
				channelMetaDatas.push_back(flat::CreateRChannelMetaData(
					builder,
					track.mPosition.mFrames.size(),
					track.mScale.mFrames.size(),
					track.mRotation.mFrames.size(),
					track.mPosition.mSampledFrames.size(),
					track.mScale.mSampledFrames.size(),
					track.mRotation.mSampledFrames.size(),
					numberOfSamples));
			}

			asset::MakeAssetNameStringForFilePath(clip.mAnimationName.c_str(), animationName, r2::asset::RANIMATION);

			auto metaData = flat::CreateRAnimationMetaData(
				builder,
				STRING_ID(animationName),
				clip.mEndTime - clip.mStartTime, //this is now duration in seconds
				0, //Ticks per second - This doesn't exist anymore - just 0
				builder.CreateVector(channelMetaDatas),
				builder.CreateString(animationName));

			animationsMetaData.push_back(metaData);
		}

		//@TODO(Serge): UUID
		auto modelAssetName = flat::CreateAssetName(builder, 0, r2::asset::GetAssetNameForFilePath(model.modelName.c_str(), r2::asset::RMODEL), builder.CreateString(model.modelName));

		auto modelMetaDataOffset = flat::CreateRModelMetaData(
			builder,
			modelAssetName,
			builder.CreateVector(meshInfos),
			&flatBounds,
			numMeshes,
			model.materialNames.size(),
			totalVertices,
			totalIndices,
			model.boneData.size() > 0,
			flat::CreateBoneMetaData(builder, model.boneData.size(), model.boneInfo.size()),
			flat::CreateSkeletonMetaData(builder, model.skeleton.mJointNameStrings.size(), model.skeleton.mBindPose.parents.size(), model.skeleton.mRestPose.joints.size(), model.skeleton.mBindPose.joints.size()),
			builder.CreateString(model.originalPath),
			builder.CreateVector(animationsMetaData)); 

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
			//@TODO(Serge): UUID
			auto materialAssetName = flat::CreateAssetName(dataBuilder, 0, model.materialNames[i].name, dataBuilder.CreateString(model.materialNames[i].materialNameStr));
			auto materialPackAssetName = flat::CreateAssetName(dataBuilder, 0, model.materialNames[i].materialPackName, dataBuilder.CreateString(model.materialNames[i].materialPackNameStr));

			flatMaterialNames.push_back(flat::CreateMaterialName(dataBuilder, materialAssetName,materialPackAssetName));
		}

		flat::Matrix4 flatGlobalInverseTransform = ToFlatMatrix4(model.globalInverseTransform);

		flatbuffers::Offset<flat::AnimationData> animationData = 0;

		if (model.boneInfo.size() > 0)
		{
			std::vector<flat::BoneData> flatBoneDatas;
			std::vector<flat::BoneInfo> flatBoneInfos;
			std::vector<int32_t> flatParents;
			std::vector<flat::Transform> flatRestPoseTransforms;
			std::vector<flat::Transform> flatBindPoseTransforms;
			std::vector<uint64_t> flatJointNames;
			std::vector<flatbuffers::Offset<flatbuffers::String>> flatJointNameStrings;
			
			assert(model.skeleton.mBindPose.parents.size() == model.skeleton.mRestPose.parents.size() && "Should always be the case");

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
				flatBoneInfo.mutable_offsetTransform() = ToFlatMatrix4(model.boneInfo[i]);

				flatBoneInfos.push_back(flatBoneInfo);
			}

			for (size_t i = 0; i < model.skeleton.mRestPose.joints.size(); ++i)
			{
				const auto& restPoseTransform = model.skeleton.mRestPose.joints[i];

				flatRestPoseTransforms.push_back(ToFlatTransform(restPoseTransform));
			}

			for (size_t i = 0; i < model.skeleton.mBindPose.joints.size(); ++i)
			{
				const auto& bindPoseTransform = model.skeleton.mBindPose.joints[i];

				flatBindPoseTransforms.push_back(ToFlatTransform(bindPoseTransform));
			}

			for (size_t i = 0; i < model.skeleton.mJointNameStrings.size(); ++i)
			{
				flatJointNames.push_back(STRING_ID( model.skeleton.mJointNameStrings[i].c_str()));
			}

			for (size_t i = 0; i < model.skeleton.mJointNameStrings.size(); ++i)
			{
				flatJointNameStrings.push_back(dataBuilder.CreateString(model.skeleton.mJointNameStrings[i]));
			}

			for (size_t i = 0; i < model.skeleton.mBindPose.parents.size(); ++i)
			{
				flatParents.push_back(model.skeleton.mBindPose.parents[i]);
			}

			animationData = flat::CreateAnimationData(
				dataBuilder,
				dataBuilder.CreateVectorOfStructs(flatBoneDatas),
				dataBuilder.CreateVectorOfStructs(flatBoneInfos),
				dataBuilder.CreateVector(flatParents),
				dataBuilder.CreateVectorOfStructs(flatRestPoseTransforms),
				dataBuilder.CreateVectorOfStructs(flatBindPoseTransforms),
				dataBuilder.CreateVector(flatJointNames),
				dataBuilder.CreateVector(flatJointNameStrings));
		}

		std::vector<flatbuffers::Offset<flat::RAnimation>> flatAnimations;

		for (const auto& clip : model.clips)
		{
			std::vector<flatbuffers::Offset<flat::TransformTrack>> transformTracks;

			for (const auto& track : clip.mTracks)
			{
				std::vector<flat::VectorKey> flatPositionKeys;
				std::vector<flat::VectorKey> flatScaleKeys;
				std::vector<flat::RotationKey> flatRotationKeys;

				for (const auto& frame : track.mPosition.mFrames)
				{
					flat::VectorKey pKey;
					pKey.mutate_time(frame.mTime);
					pKey.mutable_value() = flat::Vertex3(frame.mValue.x, frame.mValue.y, frame.mValue.z);
					pKey.mutable_in() = flat::Vertex3(frame.mIn.x, frame.mIn.y, frame.mIn.z);
					pKey.mutable_out() = flat::Vertex3(frame.mOut.x, frame.mOut.y, frame.mOut.z);

					flatPositionKeys.push_back(pKey);
				}

				for (const auto& frame : track.mScale.mFrames)
				{
					flat::VectorKey pKey;
					pKey.mutate_time(frame.mTime);
					pKey.mutable_value() = flat::Vertex3(frame.mValue.x, frame.mValue.y, frame.mValue.z);
					pKey.mutable_in() = flat::Vertex3(frame.mIn.x, frame.mIn.y, frame.mIn.z);
					pKey.mutable_out() = flat::Vertex3(frame.mOut.x, frame.mOut.y, frame.mOut.z);

					flatScaleKeys.push_back(pKey);
				}

				for (const auto& frame : track.mRotation.mFrames)
				{
					flat::RotationKey rKey;
					rKey.mutate_time(frame.mTime);
					rKey.mutable_value() = flat::Quaternion(frame.mValue.x, frame.mValue.y, frame.mValue.z, frame.mValue.w);
					rKey.mutable_in() = flat::Quaternion(frame.mIn.x, frame.mIn.y, frame.mIn.z, frame.mIn.w);
					rKey.mutable_out() = flat::Quaternion(frame.mOut.x, frame.mOut.y, frame.mOut.z, frame.mOut.w);

					flatRotationKeys.push_back(rKey);
				}

				auto positionTrackInfo = flat::CreateTrackInfo(dataBuilder, GetInterpolationType(track.mPosition.interpolation), track.mPosition.mNumberOfSamples, dataBuilder.CreateVector(track.mPosition.mSampledFrames));
				auto positionTrack = flat::CreateVectorTrack(dataBuilder, dataBuilder.CreateVectorOfStructs(flatPositionKeys), positionTrackInfo);

				auto scaleTrackInfo = flat::CreateTrackInfo(dataBuilder, GetInterpolationType(track.mScale.interpolation), track.mScale.mNumberOfSamples, dataBuilder.CreateVector(track.mScale.mSampledFrames));
				auto scaleTrack = flat::CreateVectorTrack(dataBuilder, dataBuilder.CreateVectorOfStructs(flatScaleKeys), scaleTrackInfo);

				auto rotationTrackInfo = flat::CreateTrackInfo(dataBuilder, GetInterpolationType(track.mRotation.interpolation), track.mRotation.mNumberOfSamples, dataBuilder.CreateVector(track.mRotation.mSampledFrames));
				auto rotationTrack = flat::CreateQuaternionTrack(dataBuilder, dataBuilder.CreateVectorOfStructs(flatRotationKeys), rotationTrackInfo);

				auto flatTransformTrack = flat::CreateTransformTrack(dataBuilder, track.mJointID, positionTrack, rotationTrack, scaleTrack, track.GetStartTime(), track.GetEndTime());
				
				transformTracks.push_back(flatTransformTrack);
			}

			asset::MakeAssetNameStringForFilePath(clip.mAnimationName.c_str(), animationName, r2::asset::RANIMATION);

			//@TODO(Serge): UUID
			auto animationAssetName = flat::CreateAssetName(dataBuilder, 0, STRING_ID(animationName), dataBuilder.CreateString(animationName));
			auto flatAnimationData = flat::CreateRAnimation(dataBuilder, animationAssetName, clip.mStartTime, clip.mEndTime, dataBuilder.CreateVector(transformTracks));

			flatAnimations.push_back(flatAnimationData);
		}

		//@TODO(Serge): UUID
		auto modelAssetName2 = flat::CreateAssetName(builder, 0, r2::asset::GetAssetNameForFilePath(model.modelName.c_str(), r2::asset::RMODEL), builder.CreateString(model.modelName));

		const auto rmodel = flat::CreateRModel(
			dataBuilder,
			modelAssetName2,
			&flatGlobalInverseTransform,
			dataBuilder.CreateVector(flatMaterialNames),
			dataBuilder.CreateVector(flatMeshes),
			animationData,
			dataBuilder.CreateVector(flatAnimations));

		dataBuilder.Finish(rmodel, "rmdl");

		uint8_t* modelData = dataBuilder.GetBufferPointer();
		size_t modelDataSize = dataBuilder.GetSize();

		DiskAssetFile assetFile;
		assetFile.SetFreeDataBlob(false);
		pack_model(assetFile, metaDataBuf, metaDataSize, modelData, modelDataSize);

		if (!std::filesystem::exists(outputPath))
		{
			std::filesystem::create_directories(outputPath);
		}

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


	//void ProcessMaterialTextureForType(const aiMaterial* assimpMaterial, aiTextureType textureType, std::vector<std::string>& textureNames)
	//{
	//	const uint32_t numDiffuseTextures = aiGetMaterialTextureCount(assimpMaterial, textureType);

	//	for (int i = 0; i < numDiffuseTextures; ++i)
	//	{
	//		aiString diffuseStr;

	//		assimpMaterial->GetTexture(textureType, i, &diffuseStr);

	//		fs::path diffuseTexturePath = diffuseStr.C_Str();

	//		diffuseTexturePath.make_preferred();

	//		//fs::path diffuseTextureName = diffuseTexturePath.filename();

	//		diffuseTexturePath.replace_extension(".rtex");
	//		std::string diffuseTextureNameStr = diffuseTexturePath.string();

	//		std::transform(diffuseTextureNameStr.begin(), diffuseTextureNameStr.end(), diffuseTextureNameStr.begin(),
	//			[](unsigned char c) { return std::tolower(c); });

	//		textureNames.push_back(diffuseTextureNameStr);
	//	}
	//}


	//void PrintTextureNames(const std::string& modelName, const std::string& name, const std::vector<std::string>& textures)
	//{
	//	printf("========================================\n");
	//	printf("%s - %s: \n", modelName.c_str(), name.c_str());
	//	printf("========================================\n");
	//	for (const auto& texture : textures)
	//	{
	//		printf("%s\n", texture.c_str());
	//	}
	//	printf("========================================\n\n");
	//}

	//void ProcessMaterial(const std::string& modelName, aiMaterial* assimpMaterial)
	//{
	//	//printf("Model Name: %s\n\n", modelName.c_str());
	//	//printf("----------------------------------\n");

	//	std::vector<std::string> diffuseTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_DIFFUSE, diffuseTextures);
	//	PrintTextureNames(modelName, "Diffuse Textures", diffuseTextures);

	//	std::vector<std::string> normalTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_NORMALS, normalTextures);
	//	PrintTextureNames(modelName, "Normal Textures", normalTextures);

	//	std::vector<std::string> emissiveTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_EMISSIVE, emissiveTextures);
	//	PrintTextureNames(modelName, "Emissive Textures", emissiveTextures);

	//	std::vector<std::string> heightTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_HEIGHT, heightTextures);
	//	PrintTextureNames(modelName, "Height Textures", heightTextures);

	//	std::vector<std::string> ambientOcclusionTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_AMBIENT_OCCLUSION, ambientOcclusionTextures);
	//	PrintTextureNames(modelName, "Ambient Occlusion Textures", ambientOcclusionTextures);

	//	std::vector<std::string> metallicTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_METALNESS, metallicTextures);
	//	PrintTextureNames(modelName, "Metallic Textures", metallicTextures);

	//	std::vector<std::string> roughnessTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS, roughnessTextures);
	//	PrintTextureNames(modelName, "Roughness Textures", roughnessTextures);
	//	
	//	std::vector<std::string> specularTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_NORMAL_CAMERA, specularTextures);
	//	PrintTextureNames(modelName, "Normal Camera Textures", specularTextures);


	//	std::vector<std::string> unknownTextures;
	//	ProcessMaterialTextureForType(assimpMaterial, aiTextureType_UNKNOWN, unknownTextures);
	//	PrintTextureNames(modelName, "Unknown Textures", unknownTextures);

	//	/*
	//	enum TextureType
	//	{
	//		Diffuse = 0,
	//		Emissive,
	//		Normal,
	//		Metallic,
	//		Height,
	//		Roughness,
	//		Occlusion,
	//		Anisotropy,
	//		Detail,
	//		ClearCoat,
	//		ClearCoatRoughness,
	//		ClearCoatNormal,
	//		Cubemap,
	//		NUM_TEXTURE_TYPES
	//	};
	//	*/
	//}
}
