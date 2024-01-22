#include "r2pch.h"

#include "r2/Render/Animation/Pose.h"
#include "assetlib/RModel_generated.h"
#include "r2/Render/Model/Model.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

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

		void GetMatrixPalette(const Pose& pose, const Skeleton& skeleton, r2::SArray<r2::draw::ShaderBoneTransform>* out, u32 offset)
		{
			s32 size = (s32)Size(pose);
			R2_CHECK((s32)r2::sarr::Capacity(*out) >= size, "The matrix palette is too small. We need: %i and have: %i", size, r2::sarr::Capacity(*out));
			R2_CHECK(static_cast<s32>(r2::sarr::Capacity(*out)) - offset >= size, "The matrix palette is too small. We have %i slots left and we need %i!", static_cast<s32>(r2::sarr::Capacity(*out)) - offset, size);

			r2::SArray<math::Transform>* tempTransforms =MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, math::Transform, pose.mJointTransforms->mSize);
			r2::sarr::Fill(*tempTransforms, math::Transform{});

			for (s32 i = 0; i < size; ++i)
			{
				s32 parent = r2::sarr::At(*pose.mParents, i);
				if (parent > i) { break; }
				
				math::Transform nextTransform = r2::sarr::At(*pose.mJointTransforms, i);

				if (parent >= 0)
				{
					nextTransform = math::Combine(tempTransforms->mData[parent], nextTransform);
				}

				tempTransforms->mData[i] = nextTransform;
				out->mData[i + offset].invBindPose = r2::sarr::At(*skeleton.mInvBindPose, i);

			}

			for (s32 j = 0; j < size; ++j)
			{
				out->mData[j + offset].transform = math::ToMatrix(tempTransforms->mData[j]);
			}

			out->mSize += size;
			
			FREE(tempTransforms, *MEM_ENG_SCRATCH_PTR);
		}

#if defined( R2_DEBUG ) || defined(R2_EDITOR)
		void GetDebugBones(const Pose& pose, r2::SArray<r2::draw::DebugBone>* outDebugBones)
		{
			u32 size = Size(pose);
			R2_CHECK(r2::sarr::Capacity(*outDebugBones) >= size, "We don't have enough space in our outDebugBones");

			std::vector<math::Transform> jointGlobalTransforms;

			jointGlobalTransforms.resize(size);

			int i = 0;

			for (; i < size; ++i)
			{
				int parent = pose.mParents->mData[i];
				if (parent > i)
				{
					break;
				}

				glm::vec3 parentPos = glm::vec3(0);
				math::Transform global = pose.mJointTransforms->mData[i];
				if (parent >= 0)
				{
					math::Transform parentTransform = jointGlobalTransforms[parent];
					global = math::Combine(parentTransform, global);

					parentPos = parentTransform.position;
				}

				r2::draw::DebugBone debugBone;
				debugBone.p0 = parentPos;
				debugBone.p1 = global.position;

				r2::sarr::Push(*outDebugBones, debugBone);

				jointGlobalTransforms[i] = global;
			}

			

			for (; i < size; ++i)
			{
				R2_CHECK(false, "If we do things right, this should never happen");
				//math::Transform t = GetGlobalTransform(pose, i);
				glm::vec3 parentPos = glm::vec3(0);

				auto parentIndex = pose.mParents->mData[i];

				math::Transform global = pose.mJointTransforms->mData[i];

				if (parentIndex >= 0)
				{
					math::Transform p = GetGlobalTransform(pose, parentIndex);

					parentPos = p.position;

					global = math::Combine(p, global);
				}

				r2::draw::DebugBone debugBone;
				debugBone.p0 = parentPos;
				debugBone.p1 = global.position;

				r2::sarr::Push(*outDebugBones, debugBone);


			}

		

		}
#endif

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

			for (flatbuffers::uoffset_t i = 0; i < joints->size(); ++i)
			{
				const auto flatJoint = joints->Get(i);
				const auto flatPosition = flatJoint->position();
				const auto flatScale = flatJoint->scale();
				const auto flatRotation = flatJoint->rotation();

				math::Transform t;
				t.position = glm::vec3(flatPosition->Get(0), flatPosition->Get(1), flatPosition->Get(2));
				t.scale = glm::vec3(flatScale->Get(0), flatScale->Get(1), flatScale->Get(2));
				t.rotation = glm::quat(flatRotation->Get(3), flatRotation->Get(0), flatRotation->Get(1), flatRotation->Get(2));

				r2::sarr::Push(*newPose->mJointTransforms, t);
				r2::sarr::Push(*newPose->mParents, parents->Get(i));
			}

			//memcpy(newPose->mJointTransforms->mData, joints->data(), sizeof(math::Transform) * numJoints);
			//newPose->mJointTransforms->mSize = numJoints;
			//memcpy(newPose->mParents->mData, parents->data(), sizeof(s32) * numJoints);
			//newPose->mParents->mSize = numJoints;

			return newPose;
		}

		void SetLocalTransform(Pose& pose, u32 index, const math::Transform& transform)
		{
			pose.mJointTransforms->mData[index] = transform;
		}

		void Copy(Pose& dstPose, const Pose& srcPose)
		{
			const auto numJoints = srcPose.mJointTransforms->mSize;

			R2_CHECK(dstPose.mJointTransforms->mCapacity >= numJoints, "Not enough space in our destination pose. We have: %u and need: %u", dstPose.mJointTransforms->mCapacity, numJoints);
			R2_CHECK(dstPose.mParents->mCapacity >= numJoints, "Not enough space in our destination pose. We have: %u and need: %u", dstPose.mParents->mCapacity, numJoints);

			r2::sarr::Clear(*dstPose.mJointTransforms);
			r2::sarr::Clear(*dstPose.mParents);

			r2::sarr::Copy(*dstPose.mJointTransforms, *srcPose.mJointTransforms);
			r2::sarr::Copy(*dstPose.mParents, *srcPose.mParents);
		}

	}

	u32 Pose::MemorySize(u32 numJoints, const r2::mem::utils::MemoryProperties& memProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Pose), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<math::Transform>::MemorySize(numJoints), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(numJoints), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
	}

}