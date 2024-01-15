#include "r2pch.h"
#include "r2/Render/Animation/Animation.h"

//namespace r2::draw
//{
//	
//
//	//void UpdateFrameIndexSamplesForVectorKeys(r2::SArray<VectorKey>* vectorKeys, r2::SArray<s32>* frameIndexSamples)
//	//{
//	//	if (!vectorKeys || !frameIndexSamples)
//	//	{
//	//		R2_CHECK(false, "We don't have any vector keys or frame index samples!");
//	//		return;
//	//	}
//
//	//	const s32 numKeys = (s32)r2::sarr::Size(*vectorKeys);
//	//	if (numKeys <= 1)
//	//		return;
//
//
//	//	float startTime = r2::sarr::First(*vectorKeys).time;
//	//	float duration = r2::sarr::Last(*vectorKeys).time - startTime;
//
//	//	u32 numSamples = duration * r2::draw::chnl::NUM_SAMPLES;
//
//	//	R2_CHECK(r2::sarr::Capacity(*frameIndexSamples) >= numSamples, "We don't have enough space for the samples!");
//
//	//	frameIndexSamples->mSize = numSamples;
//
//
//	//	for (u32 i = 0; i < numSamples; ++i)
//	//	{
//	//		float t = (float)i / (float)(numSamples - 1);
//	//		float time = t * duration + startTime;
//
//	//		u32 frameIndex = 0;
//
//	//		for (s32 j = numKeys - 1; j >= 0; --j)
//	//		{
//	//			if (time >= r2::sarr::At(*vectorKeys, j).time)
//	//			{
//	//				frameIndex = (u32)j;
//	//				if ((s32)frameIndex >= numKeys - 2)
//	//				{
//	//					frameIndex = numKeys - 2;
//	//				}
//	//				break;
//	//			}
//	//		}
//
//	//		r2::sarr::At(*frameIndexSamples, i) = frameIndex;
//	//	}
//	//}
//
//	//void UpdateFrameIndexSamplesForRotationKeys(r2::SArray<RotationKey>* rotationKeys, r2::SArray<s32>* frameIndexSamples)
//	//{
//	//	if (!rotationKeys || !frameIndexSamples)
//	//	{
//	//		R2_CHECK(false, "We don't have any vector keys or frame index samples!");
//	//		return;
//	//	}
//
//	//	const s32 numKeys = (s32)r2::sarr::Size(*rotationKeys);
//	//	if (numKeys <= 1)
//	//		return;
//
//	//	float startTime = r2::sarr::First(*rotationKeys).time;
//	//	float duration = r2::sarr::Last(*rotationKeys).time - startTime;
//
//	//	u32 numSamples = duration * r2::draw::chnl::NUM_SAMPLES;
//
//	//	R2_CHECK(r2::sarr::Capacity(*frameIndexSamples) >= numSamples, "We don't have enough space for the samples!");
//
//	//	frameIndexSamples->mSize = numSamples;
//
//	//	for (u32 i = 0; i < numSamples; ++i)
//	//	{
//	//		float t = (float)i / (float)(numSamples - 1);
//	//		float time = t * duration + startTime;
//
//	//		u32 frameIndex = 0;
//
//	//		for (s32 j = numKeys - 1; j >= 0; --j)
//	//		{
//	//			if (time >= r2::sarr::At(*rotationKeys, j).time)
//	//			{
//	//				frameIndex = (u32)j;
//	//				if ((s32)frameIndex >= numKeys - 2)
//	//				{
//	//					frameIndex = numKeys - 2;
//	//				}
//	//				break;
//	//			}
//	//		}
//
//	//		r2::sarr::At(*frameIndexSamples, i) = frameIndex;
//	//	}
//	//}
//
//	//s32 FindVectorKey(r2::SArray<VectorKey>* vectorKeys, r2::SArray<s32>* sampleIndexes, float time, bool loop)
//	//{
//	//	if (!vectorKeys || !sampleIndexes)
//	//	{
//	//		R2_CHECK(false, "Either the vector keys are nullptr or the sampleIndexes are");
//	//		return -1;
//	//	}
//	//	
//	//	const u64 numKeys = r2::sarr::Size(*vectorKeys);
//
//	//	if (numKeys <= 1)
//	//	{
//	//		return -1;
//	//	}
//
//	//	if (loop)
//	//	{
//	//		float startTime = r2::sarr::At(*vectorKeys, 0).time;
//	//		float endTime = r2::sarr::At(*vectorKeys, numKeys - 1).time;
//
//	//		float duration = endTime - startTime;
//
//	//		time = fmodf(time - startTime, duration);
//
//	//		if (time < 0.0f)
//	//		{
//	//			time += duration;
//	//		}
//	//		time = time + startTime;
//	//	}
//	//	else
//	//	{
//	//		if (time <= r2::sarr::At(*vectorKeys, 0).time)
//	//		{
//	//			return 0;
//	//		}
//
//	//		if (time >= r2::sarr::At(*vectorKeys, numKeys - 2).time)
//	//		{
//	//			return (s32)numKeys - 2;
//	//		}
//	//	}
//	//	float startTime = r2::sarr::At(*vectorKeys, 0).time;
//	//	float endTime = r2::sarr::At(*vectorKeys, numKeys - 1).time;
//
//	//	float duration = endTime - startTime;
//	//	float t = time / duration;
//
//	//	u32 numSamples = duration * r2::draw::chnl::NUM_SAMPLES;
//	//	u32 index = (t * (float)numSamples);
//	//	if (index >= r2::sarr::Size(*sampleIndexes))
//	//	{
//	//		return -1;
//	//	}
//
//	//	return (s32)r2::sarr::At(*sampleIndexes, index);
//	//}
//
//	//s32 FindRotationKey(r2::SArray<RotationKey>* rotationKeys, r2::SArray<s32>* sampleIndexes, float time, bool loop)
//	//{
//	//	if (!rotationKeys || !sampleIndexes)
//	//	{
//	//		R2_CHECK(false, "Either the vector keys are nullptr or the sampleIndexes are");
//	//		return -1;
//	//	}
//
//	//	const u64 numKeys = r2::sarr::Size(*rotationKeys);
//
//	//	if (numKeys <= 1)
//	//	{
//	//		return -1;
//	//	}
//
//	//	if (loop)
//	//	{
//	//		float startTime = r2::sarr::At(*rotationKeys, 0).time;
//	//		float endTime = r2::sarr::At(*rotationKeys, numKeys - 1).time;
//
//	//		float duration = endTime - startTime;
//
//	//		time = fmodf(time - startTime, duration);
//
//	//		if (time < 0.0f)
//	//		{
//	//			time += duration;
//	//		}
//	//		time = time + startTime;
//	//	}
//	//	else
//	//	{
//	//		if (time <= r2::sarr::At(*rotationKeys, 0).time)
//	//		{
//	//			return 0;
//	//		}
//
//	//		if (time >= r2::sarr::At(*rotationKeys, numKeys - 2).time)
//	//		{
//	//			return (s32)numKeys - 2;
//	//		}
//	//	}
//	//	float startTime = r2::sarr::At(*rotationKeys, 0).time;
//	//	float endTime = r2::sarr::At(*rotationKeys, numKeys - 1).time;
//
//	//	float duration = endTime - startTime;
//	//	float t = time / duration;
//
//	//	u32 numSamples = duration * r2::draw::chnl::NUM_SAMPLES;
//	//	u32 index = (t * (float)numSamples);
//	//	if (index >= r2::sarr::Size(*sampleIndexes))
//	//	{
//	//		return -1;
//	//	}
//
//	//	return (s32)r2::sarr::At(*sampleIndexes, index);
//	//}
//
//
//	u64 AnimationChannel::MemorySizeNoData(u64 numPositionKeys, u64 numScaleKeys, u64 numRotationKeys, float duration, u64 alignment, u32 headerSize, u32 boundsChecking)
//	{
//		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VectorKey>::MemorySize(numPositionKeys), alignment, headerSize, boundsChecking) +
//			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VectorKey>::MemorySize(numScaleKeys), alignment, headerSize, boundsChecking) +
//			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RotationKey>::MemorySize(numRotationKeys), alignment, headerSize, boundsChecking);
//			/*r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(duration * r2::draw::chnl::NUM_SAMPLES), alignment, headerSize, boundsChecking) +
//			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(duration * r2::draw::chnl::NUM_SAMPLES), alignment, headerSize, boundsChecking) +
//			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(duration * r2::draw::chnl::NUM_SAMPLES), alignment, headerSize, boundsChecking);*/
//	}
//
//	//namespace chnl
//	//{
//	//	void UpdateFrameIndexSamples(AnimationChannel& channel)
//	//	{
//	//		UpdateFrameIndexSamplesForVectorKeys(channel.positionKeys, channel.positionKeyFrameSamples);
//	//		UpdateFrameIndexSamplesForVectorKeys(channel.scaleKeys, channel.scaleKeyFrameSamples);
//	//		UpdateFrameIndexSamplesForRotationKeys(channel.rotationKeys, channel.rotationkeyFrameSamples);
//	//	}
//	//	
//	//	s32  FindPositionFrameIndex(const AnimationChannel& channel, float time, bool loop)
//	//	{
//	//		return FindVectorKey(channel.positionKeys, channel.positionKeyFrameSamples, time, loop);
//	//	}
//
//	//	s32  FindScaleFrameIndex(const AnimationChannel& channel, float time, bool loop)
//	//	{
//	//		return FindVectorKey(channel.scaleKeys, channel.scaleKeyFrameSamples, time, loop);
//	//	}
//
//	//	s32  FindRotationFrameIndex(const AnimationChannel& channel, float time, bool loop)
//	//	{
//	//		return FindRotationKey(channel.rotationKeys, channel.rotationkeyFrameSamples, time, loop);
//	//	}
//	//}
//
//	u64 Animation::MemorySizeNoData(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking)
//	{
//		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<AnimationChannel>::MemorySize(numChannels * r2::SHashMap<AnimationChannel>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking);
//	}
//
//	u64 Animation::MemorySize(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking)
//	{
//		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Animation), alignment, headerSize, boundsChecking) +
//			MemorySizeNoData(numChannels, alignment, headerSize, boundsChecking);
//	}
//}