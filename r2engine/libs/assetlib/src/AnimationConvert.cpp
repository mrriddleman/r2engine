#include "assetlib/AnimationConvert.h"

#include "assetlib/AnimationAsset.h"
#include "assetlib/DiskAssetFile.h"

#include "Hash.h"

#include <cassert>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
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
	const std::string RANM_EXTENSION = ".ranm";

	struct VectorKey
	{
		double time;
		glm::vec3 value;
	};

	struct RotationKey
	{
		double time;
		glm::quat quat;
	};

	struct Channel
	{
		uint64_t channelName;

		std::vector<VectorKey> positionKeys;
		std::vector<VectorKey> scaleKeys;
		std::vector<RotationKey> rotationKeys;
	};

	struct Animation
	{
		uint64_t animationName = 0;
		double duration = 0; //in ticks
		double ticksPerSecond = 0; 
		std::vector<Channel> channels;
		std::string originalPath;
	};

	glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D)
	{
		return glm::vec3(vec3D.x, vec3D.y, vec3D.z);
	}

	glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat)
	{
		return glm::quat(quat.w, quat.x, quat.y, quat.z);
	}

	void ProcessAnimation(Animation& animation, const aiNode* node, const aiScene* scene);
	bool ConvertAnimationToFlatbuffer(Animation& animation, const fs::path& inputFilePath, const fs::path& outputPath);

	bool ConvertAnimation(const std::filesystem::path& inputFilePath, const std::filesystem::path& parentOutputDir, const std::string& extension)
	{
		Assimp::Importer import;

		import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
		import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);

		const aiScene* scene = import.ReadFile(inputFilePath.string(),
			aiProcess_ImproveCacheLocality |
			aiProcess_FindDegenerates);

		if (!scene || !scene->mRootNode)
		{
			std::string errString = std::string("Failed to load animation: ") + inputFilePath.string() + std::string(" with error: ") + std::string(import.GetErrorString());

			assert(false && errString.c_str());

			return 0;
		}

		Animation animation;
		animation.originalPath = inputFilePath.string();
		//@TODO(Serge): use the path to generate a proper name with the appropriate amount of parent directories
		char animationAssetName[256];

		std::filesystem::path dstAnimationPath = inputFilePath;
		dstAnimationPath.replace_extension(RANM_EXTENSION);
		r2::asset::MakeAssetNameStringForFilePath(dstAnimationPath.string().c_str(), animationAssetName, r2::asset::RANIMATION);

		animation.animationName = r2::asset::GetAssetNameForFilePath(dstAnimationPath.string().c_str(), r2::asset::RANIMATION);

		ProcessAnimation(animation, scene->mRootNode, scene);

		import.FreeScene();

		return ConvertAnimationToFlatbuffer(animation, inputFilePath, parentOutputDir);
	}

	void ProcessAnimation(Animation& animation, const aiNode* node, const aiScene* scene)
	{
		if (scene->HasAnimations())
		{
			assert(scene->mNumAnimations == 1 && "We should only have 1 animation!");

			aiAnimation* anim = scene->mAnimations[0];
			
			animation.duration = anim->mDuration;
			animation.ticksPerSecond = anim->mTicksPerSecond;
			

			if (anim->mNumChannels > 0)
			{
				animation.channels.reserve(anim->mNumChannels);
			}

			for (uint32_t j = 0; j < anim->mNumChannels; ++j)
			{
				aiNodeAnim* animChannel = anim->mChannels[j];
				Channel channel;
				
				std::string channelName = std::string(animChannel->mNodeName.data);

				channel.channelName = STRING_ID(channelName.c_str());

				if (animChannel->mNumPositionKeys > 0)
				{
					channel.positionKeys.reserve(animChannel->mNumPositionKeys);
				}

				for (uint32_t pKey = 0; pKey < animChannel->mNumPositionKeys; ++pKey)
				{
					VectorKey positionKey;
					positionKey.time = animChannel->mPositionKeys[pKey].mTime;
					positionKey.value = AssimpVec3ToGLMVec3(animChannel->mPositionKeys[pKey].mValue);

					channel.positionKeys.push_back(positionKey);
				}

				if (animChannel->mNumScalingKeys > 0)
				{
					channel.scaleKeys.reserve(animChannel->mNumScalingKeys);
				}

				for (uint32_t sKey = 0; sKey < animChannel->mNumScalingKeys; ++sKey)
				{
					VectorKey scaleKey;
					scaleKey.time = animChannel->mScalingKeys[sKey].mTime;
					scaleKey.value = AssimpVec3ToGLMVec3(animChannel->mScalingKeys[sKey].mValue);

					channel.scaleKeys.push_back(scaleKey);
				}

				if (animChannel->mNumRotationKeys > 0)
				{
					channel.rotationKeys.reserve(animChannel->mNumRotationKeys);
				}

				for (uint32_t rKey = 0; rKey < animChannel->mNumRotationKeys; ++rKey)
				{
					RotationKey rotKey;
					rotKey.quat = AssimpQuatToGLMQuat(animChannel->mRotationKeys[rKey].mValue.Normalize());
					rotKey.time = animChannel->mRotationKeys[rKey].mTime;

					channel.rotationKeys.push_back(rotKey);
				}

				animation.channels.push_back(channel);
			}
		}
	}

	bool ConvertAnimationToFlatbuffer(Animation& animation, const fs::path& inputFilePath, const fs::path& outputPath)
	{
		//meta data stuff
		flatbuffers::FlatBufferBuilder metaDataBuilder;
		std::vector<flatbuffers::Offset<flat::RChannelMetaData>> channelMetaDatas;
		
		for (const auto& channel : animation.channels)
		{
			channelMetaDatas.push_back(flat::CreateRChannelMetaData(metaDataBuilder, channel.positionKeys.size(), channel.scaleKeys.size(), channel.rotationKeys.size()));
		}

		auto metaData = flat::CreateRAnimationMetaData(
			metaDataBuilder,
			animation.animationName,
			animation.duration,
			animation.ticksPerSecond,
			metaDataBuilder.CreateVector(channelMetaDatas),
			metaDataBuilder.CreateString(animation.originalPath));

		metaDataBuilder.Finish(metaData, "rman");

		auto* metaDataData = metaDataBuilder.GetBufferPointer();
		const auto metaDataSize = metaDataBuilder.GetSize();

		auto flatAnimationMetaData = flat::GetMutableRAnimationMetaData(metaDataData);

		flatbuffers::FlatBufferBuilder dataBuilder;

		std::vector<flatbuffers::Offset<flat::Channel>> channels;

		for (const auto& channel : animation.channels)
		{
			std::vector<flat::VectorKey> flatPositionKeys;
			std::vector<flat::VectorKey> flatScaleKeys;
			std::vector<flat::RotationKey> flatRotationKeys;

			for (const auto& positionKey : channel.positionKeys)
			{
				flat::VectorKey pKey(positionKey.time, flat::Vertex3(positionKey.value.x, positionKey.value.y, positionKey.value.z));
				flatPositionKeys.push_back(pKey);
			}

			for (const auto& scaleKey : channel.scaleKeys)
			{
				flat::VectorKey sKey(scaleKey.time, flat::Vertex3(scaleKey.value.x, scaleKey.value.y, scaleKey.value.z));
				flatScaleKeys.push_back(sKey);
			}

			for (const auto& rotationKey : channel.rotationKeys)
			{
				flat::RotationKey rKey(rotationKey.time, flat::Quaternion(rotationKey.quat.x, rotationKey.quat.y, rotationKey.quat.z, rotationKey.quat.w));
				flatRotationKeys .push_back(rKey);
			}
			
			channels.push_back(
				flat::CreateChannel(
					dataBuilder,
					channel.channelName,
					dataBuilder.CreateVectorOfStructs(flatPositionKeys),
					dataBuilder.CreateVectorOfStructs(flatScaleKeys),
					dataBuilder.CreateVectorOfStructs(flatRotationKeys)));
		}

		auto animationData = flat::CreateRAnimation(dataBuilder, animation.animationName, animation.duration, animation.ticksPerSecond, dataBuilder.CreateVector(channels));

		dataBuilder.Finish(animationData, "ranm");

		auto* flatAnimationDataBuf = dataBuilder.GetBufferPointer();
		const auto flatAnimationDataSize = dataBuilder.GetSize();

		DiskAssetFile assetFile;
		assetFile.SetFreeDataBlob(false);

		pack_animation(assetFile, metaDataData, metaDataSize, flatAnimationDataBuf, flatAnimationDataSize);

		fs::path filenamePath = inputFilePath.filename();

		fs::path outputFilePath = outputPath / filenamePath.replace_extension(RANM_EXTENSION);

		bool result = save_binaryfile(outputFilePath.string().c_str(), assetFile);

		if (!result)
		{
			printf("Failed to output file: %s\n", outputFilePath.string().c_str());
			return false;
		}

		return result;
	}
}