#include "r2pch.h"

#include "r2/Render/Animation/Track.h"
#include "r2/Core/Math/MathUtils.h"
#include "assetlib/RAnimation_generated.h"

namespace r2::anim
{
	template Track<float, 1>;
	template Track<glm::vec3, 3>;
	template Track<glm::quat, 4>;

	template<typename T, unsigned int N>
	u64 r2::anim::Track<T, N>::MemorySize(u32 numFrames, u32 numSampledFrames, const r2::mem::utils::MemoryProperties& memProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Track<T, N>), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Frame<N>>::MemorySize(numFrames), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(numSampledFrames), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
	}

	template<typename T, unsigned int N>
	r2::anim::Frame<N>& r2::anim::Track<T, N>::operator[](unsigned int index)
	{
		return r2::sarr::At(*mFrames, index);
	}

	template<typename T, unsigned int N>
	T r2::anim::Track<T, N>::Sample(float time, bool looping) const
	{
		if (mInterpolationType == LINEAR)
		{
			return SampleLinear(time, looping);
		}
		else if (mInterpolationType == CONSTANT)
		{
			return SampleConstant(time, looping);
		}

		return SampleCubic(time, looping);
	}

	template<typename T, unsigned int N>
	u32 r2::anim::Track<T, N>::Size() const
	{
		return r2::sarr::Size(*mFrames);
	}

	template<typename T, unsigned int N>
	float r2::anim::Track<T, N>::GetEndTime() const
	{
		return mFrames->mData[mFrames->mSize - 1].mTime;
	}

	template<typename T, unsigned int N>
	float r2::anim::Track<T, N>::GetStartTime() const
	{
		return mFrames->mData[0].mTime;
	}

	template<typename T, unsigned int N>
	float r2::anim::Track<T, N>::AdjustTimeToFitTrack(float time, bool looping) const
	{
		u32 size = (u32)r2::sarr::Size(*mFrames);
		if (size <= 1)
		{
			return 0.0f;
		}

		float startTime = GetStartTime();
		float endTime = GetEndTime();

		float duration = endTime - startTime;
		if (duration <= 0.0f)
		{
			return 0.0f;
		}

		if (looping)
		{
			time = fmodf(time - startTime, duration);
			if (time < 0.0f)
			{
				time += duration;
			}

			time += startTime;
		}
		else
		{
			if (time < startTime)
			{
				time = startTime;
			}
			if (time >= endTime)
			{
				time = endTime;
			}
		}

		return time;
	}

	template<typename T, unsigned int N>
	s32 r2::anim::Track<T, N>::FrameIndex(float time, bool looping) const
	{
		u32 size = r2::sarr::Size(*mFrames);
		if (size <= 1)
		{
			return -1;
		}
		
		float startTime = GetStartTime();
		float endTime = GetEndTime();
		float duration = endTime - startTime;
		
		if (looping)
		{
			time = fmodf(time - startTime, duration);
			if (time < 0.0f)
			{
				time += duration;
			}
			time += startTime;
		}
		else
		{
			if (time <= startTime)
			{
				return 0;
			}
			if (time >= mFrames->mData[size - 2].mTime)
			{
				return (int)size - 2;
			}
		}

		u32 numSampledFrames = r2::sarr::Size(*mSampledFrames);

		float t = time / duration;
		u32 numSamples = (duration * static_cast<float>(numSampledFrames));
		u32 index = (t * (float)numSamples);
		if (index >= numSampledFrames)
		{
			return -1;
		}

		return (int)mSampledFrames->mData[index];
	}

	template<typename T, unsigned int N>
	T r2::anim::Track<T, N>::Hermite(float time, const T& p1, const T& s1, const T& _p2, const T& s2) const
	{
		float tt = time * time;
		float ttt = tt * time;
		T p2 = _p2;
		math::Neighborhood(p1, p2);
		float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
		float h2 = -2.0f * ttt + 3.0f * tt;
		float h3 = ttt - 2.0f * tt + time;
		float h4 = ttt - tt;
		T result = p1 * h1 + p2 * h2 + s1 * h3 + s2 * h4;
		return math::AdjustHermiteResult(result);
	}

	template<typename T, unsigned int N>
	T r2::anim::Track<T, N>::SampleCubic(float time, bool looping) const
	{
		int thisFrame = FrameIndex(time, looping);
		if (thisFrame < 0 || thisFrame >= (int)r2::sarr::Size(*mFrames) - 1)
		{
			return T();
		}

		int nextFrame = thisFrame + 1;
		float trackTime = AdjustTimeToFitTrack(time, looping);
		float thisTime = mFrames->mData[thisFrame].mTime;
		float frameDelta = mFrames->mData[nextFrame].mTime - thisTime;
		if (frameDelta <= 0.0f)
		{
			return T();
		}

		float t = (trackTime - thisTime) / frameDelta;
		t = glm::clamp(t, 0.0f, 1.0f);
		size_t floatSize = sizeof(float);
		T point1 = Cast(&mFrames->mData[thisFrame].mValue[0]);
		T slope1;
		//we use memcpy here istead of Cast() because we don't want
		//quaternions to be normalized - and slopes are not meant to be quaternions
		memcpy(&slope1, mFrames->mData[thisFrame].mOut, N * floatSize);
		slope1 = slope1 * frameDelta;
		T point2 = Cast(&mFrames->mData[nextFrame].mValue[0]);
		T slope2;
		//we use memcpy here istead of Cast() because we don't want
		//quaternions to be normalized - and slopes are not meant to be quaternions	
		memcpy(&slope2, mFrames->mData[nextFrame].mIn, N * floatSize);
		slope2 = slope2 * frameDelta;
		return Hermite(t, point1, slope1, point2, slope2);
	}

	template<typename T, unsigned int N>
	T r2::anim::Track<T, N>::SampleLinear(float time, bool looping) const
	{
		int thisFrame = FrameIndex(time, looping);
		if (thisFrame < 0 || thisFrame >= (int)r2::sarr::Size(*mFrames) - 1)
		{
			return T();
		}

		int nextFrame = thisFrame + 1;

		float trackTime = AdjustTimeToFitTrack(time, looping);
		float thisTime = mFrames->mData[thisFrame].mTime;
		float frameDelta = mFrames->mData[nextFrame].mTime - thisTime;
		if (frameDelta <= 0.0f)
		{
			return T();
		}

		float t = (trackTime - thisTime) / frameDelta;
		t = glm::clamp(t, 0.0f, 1.0f);
		T start = Cast(&mFrames->mData[thisFrame].mValue[0]);
		T end = Cast(&mFrames->mData[nextFrame].mValue[0]);

		return math::Interpolate(start, end, t);
	}

	template<typename T, unsigned int N>
	T r2::anim::Track<T, N>::SampleConstant(float time, bool looping) const
	{
		int frame = FrameIndex(time, looping);
		if (frame < 0 || frame >= (int)r2::sarr::Size(*mFrames))
		{
			return T();
		}

		return Cast(&mFrames->mData[frame].mValue[0]);
	}

	template<> float Track<float, 1>::Cast(float* value) const
	{
		return value[0];
	}

	template<> glm::vec3 Track<glm::vec3, 3>::Cast(float* value) const
	{
		return glm::vec3(value[0], value[1], value[2]);
	}

	template<> glm::quat Track<glm::quat, 4>::Cast(float* value) const 
	{
		glm::quat r = glm::quat(value[3], value[0], value[1], value[2]);
		return glm::normalize(r);
	}

	VectorTrack* LoadVectorTrack(void** memoryPointer, const flat::VectorTrack* flatVectorTrack)
	{
		VectorTrack* newVectorTrack = nullptr;

		const auto numFrames = flatVectorTrack->keys()->size();
		const auto numSampledFrames = flatVectorTrack->trackInfo()->numberOfSamples();

		newVectorTrack = new (*memoryPointer) VectorTrack();
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(VectorTrack));

		newVectorTrack->mFrames = EMPLACE_SARRAY(*memoryPointer, VectorFrame, numFrames);
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray< VectorFrame>::MemorySize(numFrames));

		newVectorTrack->mSampledFrames = EMPLACE_SARRAY(*memoryPointer, u32, numSampledFrames);
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<u32>::MemorySize(numSampledFrames));

		memcpy(newVectorTrack->mFrames->mData, flatVectorTrack->keys()->data(), sizeof(VectorFrame) * numFrames);
		memcpy(newVectorTrack->mSampledFrames->mData, flatVectorTrack->trackInfo()->sampledKeys()->data(), sizeof(u32) * numSampledFrames);

		newVectorTrack->mInterpolationType = static_cast<InterpolationType>(flatVectorTrack->trackInfo()->interpolation());

		return newVectorTrack;
	}

	QuatTrack* LoadQuatTrack(void** memoryPointer, const flat::QuaternionTrack* flatQuatTrack)
	{
		QuatTrack* newQuatTrack = nullptr;

		const auto numFrames = flatQuatTrack->keys()->size();
		const auto numSampledFrames = flatQuatTrack->trackInfo()->numberOfSamples();

		newQuatTrack = new (*memoryPointer) QuatTrack();
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(QuatTrack));

		newQuatTrack->mFrames = EMPLACE_SARRAY(*memoryPointer, QuatFrame, numFrames);
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray< QuatFrame>::MemorySize(numFrames));

		newQuatTrack->mSampledFrames = EMPLACE_SARRAY(*memoryPointer, u32, numSampledFrames);
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<u32>::MemorySize(numSampledFrames));

		memcpy(newQuatTrack->mFrames->mData, flatQuatTrack->keys()->data(), sizeof(QuatFrame) * numFrames);
		memcpy(newQuatTrack->mSampledFrames->mData, flatQuatTrack->trackInfo()->sampledKeys()->data(), sizeof(u32) * numSampledFrames);

		newQuatTrack->mInterpolationType = static_cast<InterpolationType>(flatQuatTrack->trackInfo()->interpolation());

		return newQuatTrack;
	}
}