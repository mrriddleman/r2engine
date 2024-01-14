#ifndef __POSE_H__
#define __POSE_H__

#include <glm/glm.hpp>
#include "R2/Core/Memory/Memory.h"
#include "r2/Utils/Utils.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Core/Containers/SArray.h"
#include "flatbuffers/flatbuffers.h"

namespace r2::draw
{
	struct ShaderBoneTransform;
	struct DebugBone;
}

namespace flat
{
	struct Transform;

}

namespace r2::anim
{

	struct Skeleton;

	struct Pose
	{
	public:
		r2::SArray<math::Transform>* mJointTransforms = nullptr;
		r2::SArray<s32>* mParents = nullptr;

		static u32 MemorySize(u32 numJoints, const r2::mem::utils::MemoryProperties& memProperties);
	};

	namespace pose
	{
		u32 Size(const Pose& pose);

		s32 GetParent(const Pose& pose, u32 index);

		math::Transform GetLocalTransform(const Pose& pose, u32 index);
		math::Transform GetGlobalTransform(const Pose& pose, u32 index);
		void SetLocalTransform(Pose& pose, u32 index, const math::Transform& transform);

		void GetMatrixPalette(const Pose& pose, const Skeleton& skeleton, r2::SArray<r2::draw::ShaderBoneTransform>* out, u32 offset);

#if defined( R2_DEBUG ) || defined(R2_EDITOR)
		void GetDebugBones(const Pose& pose, std::vector<r2::draw::DebugBone>& outDebugBones);
#endif

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
