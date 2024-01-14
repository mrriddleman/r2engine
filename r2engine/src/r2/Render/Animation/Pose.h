#ifndef __POSE_H__
#define __POSE_H__

#include <glm/glm.hpp>
#include "R2/Core/Memory/Memory.h"
#include "r2/Utils/Utils.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Core/Containers/SArray.h"
#include "flatbuffers/flatbuffers.h"


namespace flat
{
	struct Transform;
}

namespace r2::anim
{
	struct Pose
	{
	public:
		r2::SArray<math::Transform>* mJointTransforms = nullptr;
		r2::SArray<s32>* mParents = nullptr;

		static u32 MemorySize(u32 numJoints, r2::mem::utils::MemoryProperties& memProperties);
	};

	namespace pose
	{
		u32 Size(const Pose& pose);

		s32 GetParent(const Pose& pose, u32 index);

		math::Transform GetLocalTransform(const Pose& pose, u32 index);
		math::Transform GetGlobalTransform(const Pose& pose, u32 index);

		void GetMatrixPalette(const Pose& pose, r2::SArray<glm::mat4>* out, u32 offset);

		bool IsEqual(const Pose& p1, const Pose& p2);


		Pose* Load(void** memoryPointer, const flatbuffers::Vector<const flat::Transform*>* joints, const flatbuffers::Vector<s32>* parents);

		template<class ARENA>
		Pose Copy(ARENA& arena, const Pose& other, const char* file, s32 line, const char* description)
		{
			Pose newPose;
			const auto numJoints = other.mJointTransforms->mSize;

			newPose.mJointTransforms = MAKE_SARRAY_VERBOSE(arena, math::Transform, numJoints, file, line, description);
			newPose.mParents = MAKE_SARRAY_VERBOSE(arena, s32, numJoints, file, line, description);

			r2::sarr::Copy(*newPose.mJointTransforms, *other.mJointTransforms);
			r2::sarr::Copy(*newPose.mParents, *other.mParents);

			return newPose;
		}
	}
}


#endif // 
