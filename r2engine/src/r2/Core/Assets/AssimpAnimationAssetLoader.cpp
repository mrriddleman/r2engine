#include "r2pch.h"

#include "r2/Core/Assets/AssimpAnimationAssetLoader.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssimpHelpers.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Animation/Animation.h"

#include "r2/Utils/Hash.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace
{
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

		return bytes + sizeof(r2::draw::Animation) * scene->mNumAnimations;
	}

	void ProcessAnimations(r2::draw::Animation* animation, void** dataPtr, const aiNode* node, const aiScene* scene)
	{
		if (animation && scene->HasAnimations())
		{
			R2_CHECK(scene->mNumAnimations == 1, "We should only have 1 animation!");

			aiAnimation* anim = scene->mAnimations[0];
				
			animation->duration = anim->mDuration;
			animation->ticksPerSeconds = anim->mTicksPerSecond;
			animation->hashName = STRING_ID(anim->mName.C_Str());


		//	printf("Channel name: %s\n", anim->mName.C_Str());

			if (anim->mNumChannels > 0)
			{
				animation->channels = EMPLACE_SARRAY(*dataPtr, r2::draw::AnimationChannel, anim->mNumChannels);
				*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::AnimationChannel>::MemorySize(anim->mNumChannels));
			}

			for (u32 j = 0; j < anim->mNumChannels; ++j)
			{
				aiNodeAnim* animChannel = anim->mChannels[j];
				r2::draw::AnimationChannel channel;
				r2::draw::AnimationChannel* channelToUse = &channel;
				std::string channelName = std::string(animChannel->mNodeName.data);

		//		printf("channel name: %s\n", channelName.c_str());
				
				channel.hashName = STRING_ID(channelName.c_str());
		
				if (animChannel->mNumPositionKeys > 0 )
				{
					channelToUse->positionKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::VectorKey, animChannel->mNumPositionKeys);
					*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(animChannel->mNumPositionKeys));
				}

				for (u32 pKey = 0; pKey < animChannel->mNumPositionKeys; ++pKey)
				{
					r2::sarr::Push(*channelToUse->positionKeys, { animChannel->mPositionKeys[pKey].mTime, AssimpVec3ToGLMVec3(animChannel->mPositionKeys[pKey].mValue) });
				}

				if (animChannel->mNumScalingKeys > 0 )
				{
					channelToUse->scaleKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::VectorKey, animChannel->mNumScalingKeys );
					*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(animChannel->mNumScalingKeys ));
				}


				for (u32 sKey = 0; sKey < animChannel->mNumScalingKeys; ++sKey)
				{
					r2::sarr::Push(*channelToUse->scaleKeys, { animChannel->mScalingKeys[sKey].mTime, AssimpVec3ToGLMVec3(animChannel->mScalingKeys[sKey].mValue) });
				}
				
				
				if (animChannel->mNumRotationKeys > 0 )
				{
					channelToUse->rotationKeys = EMPLACE_SARRAY(*dataPtr, r2::draw::RotationKey, animChannel->mNumRotationKeys);
					*dataPtr = r2::mem::utils::PointerAdd(*dataPtr, r2::SArray<r2::draw::RotationKey>::MemorySize(animChannel->mNumRotationKeys));
				}

				for (u32 rKey = 0; rKey < animChannel->mNumRotationKeys; ++rKey)
				{
					r2::sarr::Push(*channelToUse->rotationKeys, { animChannel->mRotationKeys[rKey].mTime, AssimpQuatToGLMQuat(animChannel->mRotationKeys[rKey].mValue.Normalize()) });
				}
				

				if (channelToUse == &channel)
				{
					r2::sarr::Push(*animation->channels, channel);
				}
				
			}
		}
	}
}


namespace r2::asset
{
	const char* AssimpAnimationLoader::GetPattern()
	{
		return "*";
	}

	AssetType AssimpAnimationLoader::GetType() const
	{
		return ASSIMP_ANIMATION;
	}

	bool AssimpAnimationLoader::ShouldProcess()
	{
		return true;
	}

	u64 AssimpAnimationLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		Assimp::Importer import;

		//import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
		const aiScene* scene = import.ReadFileFromMemory(rawBuffer, size, 0);

		if (!scene || !scene->mRootNode)
		{
			R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
			return 0;
		}

		u64 totalSizeInBytes = 0;
		
		if (scene->HasAnimations())
		{
			//figure out how many animations
			totalSizeInBytes += GetTotalAnimationBytes(scene->mRootNode, scene, alignment, header, boundsChecking);
		}
		import.FreeScene();

		return totalSizeInBytes;
	}

	bool AssimpAnimationLoader::LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		Assimp::Importer import;
		
		//import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = import.ReadFileFromMemory(rawBuffer, rawSize, 0);

		if (!scene || !scene->mRootNode)
		{
			R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
			return 0;
		}

		void* dataPtr = assetBuffer.MutableData();

		void* startOfArrayPtr = nullptr;


		if (scene->HasAnimations())
		{
			r2::draw::Animation* anim = new (dataPtr) r2::draw::Animation();
			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Animation));
			ProcessAnimations(anim, &startOfArrayPtr, scene->mRootNode, scene);
		}

		import.FreeScene();

		return true;
	}
}