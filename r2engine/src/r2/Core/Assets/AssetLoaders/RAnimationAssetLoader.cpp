#include "r2pch.h"

#include "r2/Core/Assets/AssetLoaders/RAnimationAssetLoader.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"
#include "assetlib/AnimationAsset.h"
#include "assetlib/RAnimationMetaData_generated.h"
#include "assetlib/RAnimation_generated.h"
#include "r2/Render/Animation/Animation.h"

namespace r2::asset
{
	RAnimationAssetLoader::RAnimationAssetLoader()
	{

	}

	const char* RAnimationAssetLoader::GetPattern()
	{
		return flat::RAnimationExtension();
	}

	r2::asset::AssetType RAnimationAssetLoader::GetType() const
	{
		return RANIMATION;
	}

	bool RAnimationAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 RAnimationAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		MemoryAssetFile memoryAssetFile{ (void*)rawBuffer, size };

		r2::assets::assetlib::load_binaryfile(filePath, memoryAssetFile);

		const auto metaData = r2::assets::assetlib::read_animation_meta_data(memoryAssetFile);
		const auto numChannels = metaData->channelsMetaData()->size();

		u64 bytes = r2::draw::Animation::MemorySize(numChannels, alignment, header, boundsChecking);

		for (u32 i = 0; i < numChannels; ++i)
		{
			const auto channelMetaData = metaData->channelsMetaData()->Get(i);
			bytes += r2::draw::AnimationChannel::MemorySizeNoData(
				channelMetaData->numPositionKeys(),
				channelMetaData->numScaleKeys(),
				channelMetaData->numRotationKeys(),
				metaData->durationInTicks(),
				alignment, header, boundsChecking);
		}

		return bytes;
	}

	bool RAnimationAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		void* dataPtr = assetBuffer.MutableData();

		void* startOfArrayPtr = nullptr;

		MemoryAssetFile memoryAssetFile{ (void*)rawBuffer, rawSize };

		r2::assets::assetlib::load_binaryfile(filePath, memoryAssetFile);

		const auto metaData = r2::assets::assetlib::read_animation_meta_data(memoryAssetFile);

		R2_CHECK(metaData != nullptr, "metaData is null!");

		const auto modelData = r2::assets::assetlib::read_animation_data(metaData, memoryAssetFile);

		R2_CHECK(modelData != nullptr, "modelData is null!");

		r2::draw::Animation* animation = new (dataPtr) r2::draw::Animation();

		startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Animation));

		const auto hashMapSize = modelData->channels()->size() * r2::SHashMap<r2::draw::AnimationChannel>::LoadFactorMultiplier();

		if (modelData->channels()->size() > 0)
		{
			animation->channels = MAKE_SHASHMAP_IN_PLACE(r2::draw::AnimationChannel, startOfArrayPtr, hashMapSize);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<r2::draw::AnimationChannel>::MemorySize(hashMapSize));
		}

		animation->hashName = modelData->animationName();
		animation->duration = modelData->durationInTicks();
		animation->ticksPerSeconds = modelData->ticksPerSeconds();

		for (u32 i = 0; i < modelData->channels()->size(); ++i)
		{
			const flat::Channel* flatChannel = modelData->channels()->Get(i);

			r2::draw::AnimationChannel channel;

			channel.hashName = flatChannel->channelName();

			if (flatChannel->positionKeys()->size() > 0)
			{
				channel.positionKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::VectorKey, flatChannel->positionKeys()->size());

				startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(flatChannel->positionKeys()->size()));
			}

			for (u32 pKey = 0; pKey < flatChannel->positionKeys()->size(); ++pKey)
			{
				const auto flatPositionKey = flatChannel->positionKeys()->Get(pKey);
				r2::sarr::Push(*channel.positionKeys, { flatPositionKey->time(), glm::vec3(flatPositionKey->value().x(), flatPositionKey->value().y(), flatPositionKey->value().z())});
			}

			if (flatChannel->scaleKeys()->size() > 0)
			{
				channel.scaleKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::VectorKey, flatChannel->scaleKeys()->size());

				startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(flatChannel->scaleKeys()->size()));
			}

			for (u32 sKey = 0; sKey < flatChannel->scaleKeys()->size(); ++sKey)
			{
				const auto flatScaleKey = flatChannel->scaleKeys()->Get(sKey);
				r2::sarr::Push(*channel.scaleKeys, { flatScaleKey->time(), glm::vec3(flatScaleKey->value().x(), flatScaleKey->value().y(), flatScaleKey->value().z()) });
			}

			if (flatChannel->rotationKeys()->size() > 0)
			{
				channel.rotationKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::RotationKey, flatChannel->rotationKeys()->size());

				startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::RotationKey>::MemorySize(flatChannel->rotationKeys()->size()));
			}

			for (u32 rKey = 0; rKey < flatChannel->rotationKeys()->size(); ++rKey)
			{
				const auto flatRotationKey = flatChannel->rotationKeys()->Get(rKey);
				r2::sarr::Push(*channel.rotationKeys, { flatRotationKey->time(), glm::quat(flatRotationKey->value().w(), flatRotationKey->value().x(), flatRotationKey->value().y(), flatRotationKey->value().z()) });
			}

			r2::shashmap::Set(*animation->channels, channel.hashName, channel);
		}

		return true;
	}

	
}