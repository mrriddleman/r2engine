#include "r2pch.h"

#include "r2/Render/Animation/Pose.h"

namespace r2::anim
{
	namespace pose
	{
		u32 Size(const Pose& pose)
		{
			return r2::sarr::Size(*pose.mJointTransforms);
		}

		s32 GetParent(const Pose& pose, u32 index)
		{
			return r2::sarr::At(*pose.mParents, index);
		}

		r2::math::Transform GetLocalTransform(const Pose& pose, u32 index)
		{
			return r2::sarr::At(*pose.mJointTransforms, index);
		}

		r2::math::Transform GetGlobalTransform(const Pose& pose, u32 index)
		{
			math::Transform result = GetLocalTransform(pose, index);

			for (s32 p = GetParent(pose, index); p >= 0;  p = GetParent(pose, p))
			{
				result = math::Combine(GetLocalTransform(pose, p), result);
			}

			return result;
		}

		void GetMatrixPalette(const Pose& pose, r2::SArray<glm::mat4>* out, u32 offset)
		{
			s32 size = (s32)Size(pose);
			R2_CHECK((s32)r2::sarr::Capacity(*out) >= size, "The matrix palette is too small. We need: %i and have: %i", size, r2::sarr::Capacity(*out));
			R2_CHECK(static_cast<s32>(r2::sarr::Capacity(*out)) - offset >= size, "The matrix palette is too small. We have %i slots left and we need %i!", static_cast<s32>(r2::sarr::Capacity(*out)) - offset, size);

			s32 i = 0;

			for (; i < size; ++i)
			{
				s32 parent = r2::sarr::At(*pose.mParents, i);
				if (parent > i) { break; }
				
				glm::mat4 global = math::ToMatrix(r2::sarr::At(*pose.mJointTransforms, i));
				if (parent >= 0)
				{
					global = out->mData[parent + offset] * global; //@TODO(Serge): investigate if this is slower than using Combine...
				}
				out->mData[i + offset] = global;
			}
			
			for (; i < size; ++i)
			{
				R2_CHECK(false, "If we do things right, this should never happen");
				math::Transform t = GetGlobalTransform(pose, i);
				out->mData[i] = math::ToMatrix(t);
			}

		}

		bool IsEqual(const Pose& p1, const Pose& p2)
		{
			if (p1.mJointTransforms->mSize != p2.mJointTransforms->mSize)
			{
				return false;
			}

			if (p1.mParents->mSize != p2.mParents->mSize)
			{
				return false;
			}

			u32 size = r2::sarr::Size(*p1.mJointTransforms);
			for (u32 i = 0; i < size; ++i)
			{
				const math::Transform& p1Local = p1.mJointTransforms->mData[i];
				const math::Transform& p2Local = p2.mJointTransforms->mData[i];

				s32 p1Parent = p1.mParents->mData[i];
				s32 p2Parent = p2.mParents->mData[i];
				if (p1Parent != p2Parent)
				{
					return false;
				}

				if (p1Local.position != p2Local.position)
				{
					return false;
				}

				if (p1Local.scale != p2Local.scale)
				{
					return false;
				}

				if (p1Local.rotation != p2Local.rotation)
				{
					return false;
				}
			}

			return true;
		}

		Pose* Load(void** memoryPointer, const flatbuffers::Vector<const flat::Transform*>* joints, const flatbuffers::Vector<s32>* parents)
		{	
			const auto numJoints = joints->size();
			Pose* newPose;

			newPose = new (*memoryPointer) Pose();
			*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(Pose));

			newPose->mJointTransforms = EMPLACE_SARRAY(*memoryPointer, math::Transform, numJoints)
			*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<math::Transform>::MemorySize(numJoints));
			
			newPose->mParents = EMPLACE_SARRAY(*memoryPointer, s32, numJoints);
			*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<s32>::MemorySize(numJoints));

			////@TODO(Serge): check to see if this is okay
			memcpy(newPose->mJointTransforms->mData, joints->data(), sizeof(math::Transform) * numJoints);
			newPose->mJointTransforms->mSize = numJoints;
			memcpy(newPose->mParents->mData, parents->data(), sizeof(s32) * numJoints);
			newPose->mParents->mSize = numJoints;

			return newPose;
		}
	}

	u32 Pose::MemorySize(u32 numJoints, r2::mem::utils::MemoryProperties& memProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Pose), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<math::Transform>::MemorySize(numJoints), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(numJoints), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
	}

}