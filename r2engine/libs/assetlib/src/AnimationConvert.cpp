#include "assetlib/AnimationConvert.h"

#include "assetlib/AnimationAsset.h"
#include "assetlib/DiskAssetFile.h"

#include "Hash.h"

#include <cassert>
#include <string>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <unordered_map>
#include <vector>

#include "flatbuffers/flatbuffers.h"

#include "assetlib/RAnimation_generated.h"
#include "assetlib/RAnimationMetaData_generated.h"
#include "assetlib/AssetUtils.h"

namespace fs = std::filesystem;

namespace r2::assets::assetlib
{
	//const std::string RANM_EXTENSION = ".ranm";

	//glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D)
	//{
	//	return glm::vec3(vec3D.x, vec3D.y, vec3D.z);
	//}

	//glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat)
	//{
	//	return glm::quat(quat.w, quat.x, quat.y, quat.z);
	//}

	//void ProcessAnimation(Animation& animation, const aiNode* node, const aiScene* scene);

	//bool LoadAnimationFromFile(const std::filesystem::path& inputFilePath, Animation& animation)
	//{
	//	Assimp::Importer import;

	//	import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	//	import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);

	//	const aiScene* scene = import.ReadFile(inputFilePath.string(),
	//		aiProcess_ImproveCacheLocality |
	//		aiProcess_FindDegenerates);

	//	if (!scene || !scene->mRootNode)
	//	{
	//		std::string errString = std::string("Failed to load animation: ") + inputFilePath.string() + std::string(" with error: ") + std::string(import.GetErrorString());

	//		assert(false && errString.c_str());

	//		return false;
	//	}

	//	animation.originalPath = inputFilePath.string();
	//	std::filesystem::path dstAnimationPath = inputFilePath;
	//	dstAnimationPath.replace_extension(RANM_EXTENSION);
	//	animation.animationName = r2::asset::GetAssetNameForFilePath(dstAnimationPath.string().c_str(), r2::asset::RANIMATION);

	//	ProcessAnimation(animation, scene->mRootNode, scene);

	//	import.FreeScene();

	//	return true;
	//}

	//void ProcessAnimation(Animation& animation, const aiNode* node, const aiScene* scene)
	//{
	//	if (scene->HasAnimations())
	//	{
	//		assert(scene->mNumAnimations == 1 && "We should only have 1 animation!");

	//		aiAnimation* anim = scene->mAnimations[0];
	//		
	//		animation.duration = anim->mDuration;
	//		animation.ticksPerSecond = anim->mTicksPerSecond;
	//		

	//		if (anim->mNumChannels > 0)
	//		{
	//			animation.channels.reserve(anim->mNumChannels);
	//		}

	//		for (uint32_t j = 0; j < anim->mNumChannels; ++j)
	//		{
	//			aiNodeAnim* animChannel = anim->mChannels[j];
	//			Channel channel;
	//			
	//			std::string channelName = std::string(animChannel->mNodeName.data);

	//			channel.channelName = STRING_ID(channelName.c_str());

	//			if (animChannel->mNumPositionKeys > 0)
	//			{
	//				channel.positionKeys.reserve(animChannel->mNumPositionKeys);
	//			}

	//			for (uint32_t pKey = 0; pKey < animChannel->mNumPositionKeys; ++pKey)
	//			{
	//				VectorKey positionKey;
	//				positionKey.time = animChannel->mPositionKeys[pKey].mTime;
	//				positionKey.value = AssimpVec3ToGLMVec3(animChannel->mPositionKeys[pKey].mValue);

	//				channel.positionKeys.push_back(positionKey);
	//			}

	//			if (animChannel->mNumScalingKeys > 0)
	//			{
	//				channel.scaleKeys.reserve(animChannel->mNumScalingKeys);
	//			}

	//			for (uint32_t sKey = 0; sKey < animChannel->mNumScalingKeys; ++sKey)
	//			{
	//				VectorKey scaleKey;
	//				scaleKey.time = animChannel->mScalingKeys[sKey].mTime;
	//				scaleKey.value = AssimpVec3ToGLMVec3(animChannel->mScalingKeys[sKey].mValue);

	//				channel.scaleKeys.push_back(scaleKey);
	//			}

	//			if (animChannel->mNumRotationKeys > 0)
	//			{
	//				channel.rotationKeys.reserve(animChannel->mNumRotationKeys);
	//			}

	//			for (uint32_t rKey = 0; rKey < animChannel->mNumRotationKeys; ++rKey)
	//			{
	//				RotationKey rotKey;
	//				rotKey.quat = AssimpQuatToGLMQuat(animChannel->mRotationKeys[rKey].mValue.Normalize());
	//				rotKey.time = animChannel->mRotationKeys[rKey].mTime;

	//				channel.rotationKeys.push_back(rotKey);
	//			}

	//			animation.channels.push_back(channel);
	//		}
	//	}
	//}
}