//
//  AnimationPlayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//
#include "r2pch.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "AnimationPlayer.h"
#include "r2/Render/Animation/Animation.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Utils/Timer.h"

namespace
{
    std::vector<f64> s_CalcBoneRuns;

    void CalculateStaticDebugBones(const r2::draw::Model& model, r2::SArray<r2::math::Transform>& outTransforms, r2::SArray<r2::draw::DebugBone>* outDebugBones, u64 offset);

    void CalculateBoneTransforms(
        f64 animationTime,
        const r2::draw::Animation& animation,
        const r2::draw::Model& model,
        r2::SArray<r2::draw::ShaderBoneTransform>& outTransforms,
        r2::SArray<r2::draw::DebugBone>* outDebugBones,
        u64 offset);
    
    r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName);
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::vec3& refScale);
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::quat& refRotation);
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::vec3& refPosition);
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
}

namespace r2::draw
{
	//void PlayAnimationForSkinnedModel(
	//	u32 timeInMilliseconds,
	//	const AnimModel& model,
	//	const AnimationHandle& animationHandle,
	//	AnimationCache& animationCache,
	//	r2::SArray<ShaderBoneTransform>& outBoneTransforms,
 //       r2::SArray<DebugBone>& outDebugBones)
 //   {

 //       if (r2::sarr::Capacity(outBoneTransforms) < r2::sarr::Size(*model.boneInfo))
 //       {
 //           R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
 //           return;
 //       }


 //       const Animation* anim = animcache::GetAnimation(animationCache, animationHandle);

	//	if (anim == nullptr)
	//	{
	//		outBoneTransforms.mSize = model.boneInfo->mSize;
	//		for (u32 i = 0; i < model.boneInfo->mSize; ++i)
	//		{
	//			r2::sarr::At(outBoneTransforms, i).globalInv = glm::mat4(1.0f);
 //               r2::sarr::At(outBoneTransforms, i).transform = glm::mat4(1.0f);
 //               r2::sarr::At(outBoneTransforms, i).invBindPose = glm::mat4(1.0f);
	//		}

 //           outDebugBones.mSize = model.boneData->mSize;
 //         //  CalculateStaticDebugBones(model, outDebugBones, 0);

	//		return;
	//	}

 //       double ticksPerSecond = anim->ticksPerSeconds != 0 ? anim->ticksPerSeconds : 25.0;
 //       double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
 //       double animationTime = fmod(timeInTicks, anim->duration);

 //       R2_CHECK(animationTime < anim->duration, "Hmmm");
 //       
 //       outBoneTransforms.mSize = model.boneInfo->mSize;
 //       outDebugBones.mSize = model.boneInfo->mSize;
	//	for (u32 i = 0; i < model.boneInfo->mSize; ++i)
	//	{
	//		r2::sarr::At(outBoneTransforms, i).globalInv = glm::mat4(1.0f);
 //           r2::sarr::At(outBoneTransforms, i).transform = glm::mat4(1.0f);
 //           r2::sarr::At(outBoneTransforms, i).invBindPose = glm::mat4(1.0f);
	//	}

 //       CalculateBoneTransforms(animationTime, *anim, model, outBoneTransforms, outDebugBones, 0);
 //   }

	u32 PlayAnimationForAnimModel(
		u32 timeInMilliseconds,
		u32 startTime,
		bool loop,
		const Model& model,
		const Animation* animation,
		r2::SArray<ShaderBoneTransform>& outBoneTransforms,
        r2::SArray<DebugBone>* outDebugBones,
		u64 offset) 
    {
		if (r2::sarr::Capacity(outBoneTransforms) < r2::sarr::Size(*model.optrBoneInfo))
		{
			R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
			return 0;
		}

        if (outBoneTransforms.mSize + model.optrBoneInfo->mSize > r2::sarr::Capacity(outBoneTransforms))
        {
            R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
            return 0;
        }

        if (animation == nullptr)
        {
            const u64 numJoints = r2::sarr::Size(*model.skeleton.mJointNames);
            r2::SArray<r2::math::Transform>* tempTransforms = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::math::Transform, numJoints);

			outBoneTransforms.mSize += model.optrBoneInfo->mSize;
            if (outDebugBones && outDebugBones->mSize + model.optrAnimations->mSize <= outDebugBones->mCapacity)
            {
                outDebugBones->mSize += model.optrBoneInfo->mSize;
            }

			CalculateStaticDebugBones(model, *tempTransforms, outDebugBones, offset);

            for (u64 j = 0; j < numJoints; ++j)
            {
                u64 hashName = r2::sarr::At(*model.skeleton.mJointNames, j);
				s32 theDefault = -1;
				s32 boneMapResult = r2::shashmap::Get(*model.optrBoneMapping, hashName, theDefault);

                if (boneMapResult != theDefault)
                {
                    u32 boneIndex = boneMapResult;
					r2::sarr::At(outBoneTransforms, boneIndex + offset).globalInv = model.globalInverseTransform;
					r2::sarr::At(outBoneTransforms, boneIndex + offset).transform = r2::math::ToMatrix(r2::sarr::At(*tempTransforms, j));
					r2::sarr::At(outBoneTransforms, boneIndex + offset).invBindPose = r2::sarr::At(*model.optrBoneInfo, boneIndex).offsetTransform;
                }
            }

            FREE(tempTransforms, *MEM_ENG_SCRATCH_PTR);

            return outBoneTransforms.mSize;
        }

		double ticksPerSecond = animation->ticksPerSeconds != 0 ? animation->ticksPerSeconds : 30.0;
		double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double realStartTime = r2::util::MillisecondsToSeconds(startTime) * ticksPerSecond;
        double duration = animation->duration;

        double animationTime = 0.0;

        animationTime = (timeInTicks - realStartTime) ;

        if (animationTime < 0.0)
        {
            animationTime = 0.0;
        }

        if (loop)
        {
            animationTime = fmod(animationTime, duration);
        }
        else
        {
            if (animationTime > duration)
            {
                animationTime = duration;
            }

            if (animationTime < 0.0)
            {
                animationTime = 0.0;
            }
        }

//		R2_CHECK(animationTime <= animation->duration, "Hmmm");

        outBoneTransforms.mSize += model.optrBoneInfo->mSize;


        if (outDebugBones && outDebugBones->mSize + model.optrAnimations->mSize <= outDebugBones->mCapacity)
        {
            outDebugBones->mSize += model.optrBoneInfo->mSize;
        }
        
		//for (u32 i = 0; i < model.boneInfo->mSize; ++i)
		//{
		//	r2::sarr::At(outBoneTransforms, i + offset).globalInv = glm::mat4(1.0f);
  //          r2::sarr::At(outBoneTransforms, i + offset).transform = glm::mat4(1.0f);
  //          r2::sarr::At(outBoneTransforms, i + offset).invBindPose = glm::mat4(1.0f);
		//}

        CalculateBoneTransforms(animationTime, *animation, model, outBoneTransforms, outDebugBones, offset);



		return outBoneTransforms.mSize;
    }
}

namespace
{
    void CalculateStaticDebugBones(const r2::draw::Model& model, r2::SArray<r2::math::Transform>& outTransforms, r2::SArray<r2::draw::DebugBone>* outDebugBones, u64 offset)
    {
		const u64 numJoints = r2::sarr::Size(*model.skeleton.mJointNames);
		r2::SArray<r2::math::Transform>* tempGlobalTransforms = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::math::Transform, numJoints);
		tempGlobalTransforms->mSize = numJoints;
        r2::sarr::At(*tempGlobalTransforms, 0) = r2::math::Transform{};

        for (u64 j = 0; j < numJoints; ++j)
        {
            u64 hashName = r2::sarr::At(*model.skeleton.mJointNames, j);
            r2::math::Transform transform = r2::sarr::At(*model.skeleton.mBindPoseTransforms, j);

			s32 parent = r2::sarr::At(*model.skeleton.mParents, j);

			r2::sarr::At(*tempGlobalTransforms, j) = transform;
			if (parent >= 0)
			{
                r2::sarr::At(*tempGlobalTransforms, j) = r2::math::Combine(r2::sarr::At(*tempGlobalTransforms, parent), transform);
			}

			s32 theDefault = -1;
			s32 boneMapResult = r2::shashmap::Get(*model.optrBoneMapping, hashName, theDefault);

            if (boneMapResult != theDefault)
            {
                u32 boneIndex = boneMapResult;
                s32 realBoneParentIndex = r2::sarr::At(*model.skeleton.mRealParentBones, j);

                if (realBoneParentIndex >= 0)
                {
                    const r2::math::Transform& globalTransform = r2::sarr::At(*tempGlobalTransforms, j);
                    const r2::math::Transform& parentTransform = r2::sarr::At(*tempGlobalTransforms, realBoneParentIndex);

                    if (outDebugBones && boneIndex + offset < r2::sarr::Capacity(*outDebugBones))
                    {
						r2::draw::DebugBone& debugBone = r2::sarr::At(*outDebugBones, boneIndex + offset);
						debugBone.p0 = parentTransform.position;
						debugBone.p1 = globalTransform.position;
                    }
                }
            }
        }

        r2::sarr::Copy(outTransforms, *tempGlobalTransforms);
        
        FREE(tempGlobalTransforms, *MEM_ENG_SCRATCH_PTR);
    }

	void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, const r2::draw::Model& model, r2::SArray<r2::draw::ShaderBoneTransform>& outTransforms, r2::SArray<r2::draw::DebugBone>* outDebugBones, u64 offset)
	{
        const u64 numJoints = r2::sarr::Size(*model.skeleton.mJointNames);
        r2::SArray<r2::math::Transform>* tempGlobalTransforms = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::math::Transform, numJoints);
        tempGlobalTransforms->mSize = numJoints;
        r2::sarr::At(*tempGlobalTransforms, 0) = r2::math::Transform{};
        
        for (u64 j = 0; j < numJoints; ++j)
        {
            u64 hashName = r2::sarr::At(*model.skeleton.mJointNames, j);

            //@Optimization
            //@TODO(Serge): Dunno how to optimize this yet, tried to make the channels follow the joints directly but didn't work with the models we're using at the moment. Could try this again in the future. For now leaving it as is.
            r2::draw::AnimationChannel* channel = FindChannel(animation, hashName);

            r2::math::Transform transform = r2::sarr::At(*model.skeleton.mRestPoseTransforms, j);
            if (channel)
            {
				//interpolate using the channel
                //@Optimization
                //@TODO(Serge): I dunno how to optimize these yet but they could use an additional pass
                transform.scale = CalculateScaling(animationTime, animation.duration, *channel, transform.scale);
                transform.rotation = CalculateRotation(animationTime, animation.duration, *channel, transform.rotation);
                transform.position = CalculateTranslation(animationTime, animation.duration, *channel, transform.position);
            }

            s32 parent = r2::sarr::At(*model.skeleton.mParents, j);

            if (parent > (s32)j)
            {
                R2_CHECK(false, "uh oh - our parent index is greater than our index - we rely on the fact that child bone come after parent bones");
            }

            r2::sarr::At(*tempGlobalTransforms, j) = transform;

            if (parent >= 0)
            {
                //get the parent's global transform and combine with yours
                //@Optimization
                //@TODO(Serge): slow
                r2::math::Combine(r2::sarr::At(*tempGlobalTransforms, parent), transform, r2::sarr::At(*tempGlobalTransforms, j));

//                 = r2::math::Combine(r2::sarr::At(*tempGlobalTransforms, parent), transform);
            }

			s32 theDefault = -1;

			s32 boneMapResult = r2::shashmap::Get(*model.optrBoneMapping, hashName, theDefault);

            if (boneMapResult != theDefault)
            {
                u32 boneIndex = boneMapResult;

                const r2::math::Transform& globalTransform = r2::sarr::At(*tempGlobalTransforms, j);

                r2::sarr::At(outTransforms, boneIndex + offset).globalInv = model.globalInverseTransform;
                //@Optimization
                //@TODO(Serge): slow
                r2::sarr::At(outTransforms, boneIndex + offset).transform = r2::math::ToMatrix(globalTransform);
                r2::sarr::At(outTransforms, boneIndex + offset).invBindPose = r2::sarr::At(*model.optrBoneInfo, boneIndex).offsetTransform;

                s32 realParentJointIndex = r2::sarr::At(*model.skeleton.mRealParentBones, j);

                if (realParentJointIndex >= 0)
                {
                    const r2::math::Transform& parentBoneGlobalTransform = r2::sarr::At(*tempGlobalTransforms, realParentJointIndex);

                    if (outDebugBones)
                    {
						r2::draw::DebugBone& debugBone = r2::sarr::At(*outDebugBones, boneIndex + offset);

						debugBone.p0 = parentBoneGlobalTransform.position;
						debugBone.p1 = globalTransform.position;
                    }
                }
            }
        }


        FREE(tempGlobalTransforms, *MEM_ENG_SCRATCH_PTR);
    }
    
    r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName)
    {
        r2::draw::AnimationChannel defaultChannel;

        r2::draw::AnimationChannel& result = r2::shashmap::Get(*animation.channels, hashName, defaultChannel);

        if (result.positionKeys != nullptr)
        {
            return &result;
        }

        return nullptr;
    }
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::vec3& refScale)
    {
        u64 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        if (numScalingKeys == 1)
        {
            return refScale;
        }
        
		if (channel.state.mCurScaleTime > animationTime)
		{
			channel.state.mCurScaleIndex = 0;
		}

        channel.state.mCurScaleIndex = FindScalingIndex(animationTime, totalTime, channel, channel.state.mCurScaleIndex);
		u32 nextScalingIndex = (channel.state.mCurScaleIndex + 1) % numScalingKeys;

		channel.state.mCurScaleTime = r2::sarr::At(*channel.scaleKeys, channel.state.mCurScaleIndex).time;

        if (nextScalingIndex >= numScalingKeys)
        {
            R2_CHECK(false, "nextScalingIndex: %u is larger or equal to the number of scaling keys: %u", nextScalingIndex, numScalingKeys);
            return glm::vec3(1.0f);
        }

        double nextScaleTime = r2::sarr::At(*channel.scaleKeys, nextScalingIndex).time;

        double dt = fabs(nextScaleTime - channel.state.mCurScaleTime);
        
        if (nextScalingIndex < channel.state.mCurScaleIndex)
        {
            dt = totalTime - channel.state.mCurScaleTime;
        }
        
        float factor = (float)glm::clamp((animationTime - channel.state.mCurScaleTime)/ dt, 0.0, 1.0) ;

        const glm::vec3& start = r2::sarr::At(*channel.scaleKeys, channel.state.mCurScaleIndex).value;
        const glm::vec3& end = r2::sarr::At(*channel.scaleKeys, nextScalingIndex).value;
        
        return r2::math::Interpolate(start, end, factor);
    }
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::quat& refRotation)
    {
        u64 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        if (numRotationKeys == 1)
        {
            return refRotation;
        }

		if (channel.state.mCurRotationTime > animationTime)
		{
			channel.state.mCurRotationIndex = 0;
		}

        channel.state.mCurRotationIndex = FindRotationIndex(animationTime, totalTime, channel, channel.state.mCurRotationIndex);
		u32 nextRotationIndex = (channel.state.mCurRotationIndex + 1) % numRotationKeys;

        channel.state.mCurRotationTime = r2::sarr::At(*channel.rotationKeys, channel.state.mCurRotationIndex).time;

		if (nextRotationIndex >= numRotationKeys)
		{
			R2_CHECK(false, "nextRotationIndex: %u is larger or equal to the number of rotation keys: %u", nextRotationIndex, numRotationKeys);
			return glm::vec3(1.0f);
		}

        double nextRotationTime = r2::sarr::At(*channel.rotationKeys, nextRotationIndex).time;

        double dt = fabs(nextRotationTime - channel.state.mCurRotationTime);
        
        if (nextRotationIndex < channel.state.mCurRotationIndex)
        {
            dt = totalTime - channel.state.mCurRotationTime;
        }
        
        float factor = (float)glm::clamp((animationTime - channel.state.mCurRotationTime)/ dt, 0.0, 1.0) ;
		const glm::quat& start = r2::sarr::At(*channel.rotationKeys, channel.state.mCurRotationIndex).quat;
		const glm::quat& end = r2::sarr::At(*channel.rotationKeys, nextRotationIndex).quat;

        return r2::math::Interpolate(start, end, factor);
    }
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel, const glm::vec3& refPosition)
    {
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        if (numPositionKeys == 1)
        {
            return refPosition;
        }

        if (channel.state.mCurTranslationTime > animationTime)
        {
            channel.state.mCurTranslationIndex = 0;
        }

		channel.state.mCurTranslationIndex = FindPositionIndex(animationTime, totalTime, channel, channel.state.mCurTranslationIndex);

        u32 nextTranslationIndex = 0;
        
        {
            nextTranslationIndex = (channel.state.mCurTranslationIndex + 1) % numPositionKeys;
        }

        channel.state.mCurTranslationTime = r2::sarr::At(*channel.positionKeys, channel.state.mCurTranslationIndex).time;
        
		if (nextTranslationIndex >= numPositionKeys)
		{
			R2_CHECK(false, "nextTranslationIndex: %u is larger or equal to the number of position keys: %u", nextTranslationIndex, numPositionKeys);
			return glm::vec3(1.0f);
		}

        double nextPosTime = r2::sarr::At(*channel.positionKeys, nextTranslationIndex).time;

        double dt = fabs(nextPosTime - channel.state.mCurTranslationTime);
        
        if (nextTranslationIndex < channel.state.mCurTranslationIndex)
        {
            dt = totalTime - channel.state.mCurTranslationTime;
        }
        
        float factor = (float)glm::clamp((animationTime - channel.state.mCurTranslationTime)/ dt, 0.0, 1.0) ;

        const glm::vec3& start = r2::sarr::At(*channel.positionKeys, channel.state.mCurTranslationIndex).value;
        const glm::vec3& end = r2::sarr::At(*channel.positionKeys, nextTranslationIndex).value;

        glm::vec3 r = r2::math::Interpolate(start, end, factor);


        return r;
    }
    
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
        u32 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        R2_CHECK(numScalingKeys > 0, "We don't have any scaling keys");
        
        for (u32 i = startingIndex; i < numScalingKeys -1; ++i)
        {
            const auto time = r2::sarr::At(*channel.scaleKeys, i + 1).time;
            if (animationTime < time)
            {
                return i;
            }
        }

        if (animationTime >= totalTime)
        {
            return numScalingKeys - 2;
        }
        
        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
        u32 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        R2_CHECK(numRotationKeys > 0, "We don't have any rotation keys");
        
        for (u32 i = startingIndex; i < numRotationKeys -1; ++i)
        {
            if (animationTime < r2::sarr::At(*channel.rotationKeys, i+1).time)
            {
                return i;
            }
        }

		if (animationTime >= totalTime)
		{
			return numRotationKeys - 2;
		}


        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        R2_CHECK(numPositionKeys > 0, "We don't have any position keys");
        

        for (u32 i = startingIndex; i < numPositionKeys-1; ++i)
        {
			if (animationTime < r2::sarr::At(*channel.positionKeys, i + 1).time)
			{
				return i;
			}
        }

		if (animationTime >= totalTime)
		{
			return numPositionKeys - 2;
		}


        R2_CHECK(false, "Failed to find a proper position key for time: %f", animationTime);
        
        return 0;
    }
}
