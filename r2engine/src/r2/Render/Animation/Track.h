#ifndef __TRACK_H__
#define __TRACK_H__

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "r2/Render/Animation/Frame.h"


namespace flat
{
	struct VectorTrack;
	struct QuaternionTrack;
}

namespace r2::anim
{

	enum InterpolationType : u8
	{
		CONSTANT = 0,
		LINEAR,
		CUBIC
	};

	template<typename T, unsigned int N>
	struct Track
	{
		r2::SArray<Frame<N>>* mFrames = nullptr;
		r2::SArray<u32>* mSampledFrames = nullptr;
		InterpolationType mInterpolationType = LINEAR;

		Frame<N>& operator[](unsigned int index);

		float GetStartTime() const;
		float GetEndTime() const;
		u32 Size() const;
		T Sample(float time, bool looping) const;

		static u64 MemorySize(u32 numFrames, u32 numSampledFrames, const r2::mem::utils::MemoryProperties& memoryProperties);
	private:

		T SampleConstant(float time, bool looping) const;
		T SampleLinear(float time, bool looping) const;
		T SampleCubic(float time, bool looping) const;

		T Hermite(float time, const T& p1, const T& s1, const T& p2, const T& s2) const;

		s32 FrameIndex(float time, bool looping) const;
		float AdjustTimeToFitTrack(float t, bool looping) const;
		T Cast(float* value) const;
	};

	typedef Track<float, 1u> ScalarTrack;
	typedef Track<glm::vec3, 3u> VectorTrack;
	typedef Track<glm::quat, 4u> QuatTrack;

	VectorTrack* LoadVectorTrack(void** memoryPointer, const flat::VectorTrack* flatVectorTrack);
	QuatTrack* LoadQuatTrack(void** memoryPointer, const flat::QuaternionTrack* flatQuatTrack);
}

#endif