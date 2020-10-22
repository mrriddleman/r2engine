//
//  AnimationPlayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//
#include "r2pch.h"
#include "AnimationPlayer.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Animation/Animation.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Utils/Hash.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "r2/Utils/Timer.h"

namespace
{
    std::vector<f64> s_CalcBoneRuns;

    void CalculateStaticDebugBones(const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, glm::mat4 lastParentBoneTransform, r2::SArray<r2::draw::DebugBone>& outDebugBones, u64 offset);

    void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, glm::mat4 lastBoneParentTransform, r2::SArray<glm::mat4>& outTransforms, r2::SArray<r2::draw::DebugBone>& outDebugBones, u64 offset);
    
    r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName);
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel);
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel);
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel);
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex);
}

namespace r2::draw
{
	void PlayAnimationForSkinnedModel(
		u32 timeInMilliseconds,
		const AnimModel& model,
		const AnimationHandle& animationHandle,
		AnimationCache& animationCache,
		r2::SArray<glm::mat4>& outBoneTransforms,
        r2::SArray<DebugBone>& outDebugBones)
    {

        if (r2::sarr::Capacity(outBoneTransforms) < r2::sarr::Size(*model.boneInfo))
        {
            R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
            return;
        }


        const Animation* anim = animcache::GetAnimation(animationCache, animationHandle);

		if (anim == nullptr)
		{
			outBoneTransforms.mSize = model.boneInfo->mSize;
			for (u32 i = 0; i < model.boneInfo->mSize; ++i)
			{
				r2::sarr::At(outBoneTransforms, i) = glm::mat4(1.0f);
			}

            outDebugBones.mSize = model.boneData->mSize;
            CalculateStaticDebugBones(model, model.skeleton, glm::mat4(1.0f), glm::mat4(1.0f), outDebugBones, 0);

			return;
		}

        double ticksPerSecond = anim->ticksPerSeconds != 0 ? anim->ticksPerSeconds : 25.0;
        double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double animationTime = fmod(timeInTicks, anim->duration);

        R2_CHECK(animationTime < anim->duration, "Hmmm");
        
        outBoneTransforms.mSize = model.boneInfo->mSize;
        outDebugBones.mSize = model.boneInfo->mSize;
		for (u32 i = 0; i < model.boneInfo->mSize; ++i)
		{
			r2::sarr::At(outBoneTransforms, i) = glm::mat4(1.0f);
		}
       // PROFILE_SCOPE("CalculateBoneTransforms")
        CalculateBoneTransforms(animationTime, *anim, model, model.skeleton, glm::mat4(1.0f), glm::mat4(1.0f), outBoneTransforms, outDebugBones, 0);
    }

	u32 PlayAnimationForAnimModel(
		u32 timeInMilliseconds,
		const AnimModel& model,
		const AnimationHandle& animationHandle,
		AnimationCache& animationCache,
		r2::SArray<glm::mat4>& outBoneTransforms,
        r2::SArray<DebugBone>& outDebugBones,
		u64 offset) 
    {
      //  PROFILE_SCOPE_FN

		if (r2::sarr::Capacity(outBoneTransforms) < r2::sarr::Size(*model.boneInfo))
		{
			R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
			return 0;
		}

        if (outBoneTransforms.mSize + model.boneInfo->mSize > r2::sarr::Capacity(outBoneTransforms))
        {
            R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
            return 0;
        }

		const Animation* anim = animcache::GetAnimation(animationCache, animationHandle);

        if (anim == nullptr)
        {
			outBoneTransforms.mSize += model.boneInfo->mSize;
            for (u32 i = 0; i < model.boneInfo->mSize; ++i)
            {
                r2::sarr::At(outBoneTransforms, i + offset) = glm::mat4(1.0f);
            }

            outDebugBones.mSize += model.boneInfo->mSize;

        //    PROFILE_SCOPE("CalculateStaticDebugBones")
            CalculateStaticDebugBones(model, model.skeleton, glm::mat4(1.0f), glm::mat4(1.0f), outDebugBones, offset);

            return outBoneTransforms.mSize;
        }

		double ticksPerSecond = anim->ticksPerSeconds != 0 ? anim->ticksPerSeconds : 25.0;
		double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double duration = anim->duration;

		double animationTime = fmod(timeInTicks, duration);

		R2_CHECK(animationTime < anim->duration, "Hmmm");

        outBoneTransforms.mSize += model.boneInfo->mSize;
        outDebugBones.mSize += model.boneInfo->mSize;

		for (u32 i = 0; i < model.boneInfo->mSize; ++i)
		{
			r2::sarr::At(outBoneTransforms, i + offset) = glm::mat4(1.0f);
		}

      //  PROFILE_SCOPE("CalculateBoneTransforms")
        s_CalcBoneRuns.clear();
        CalculateBoneTransforms(animationTime, *anim, model, model.skeleton, glm::mat4(1.0f), glm::mat4(1.0f), outBoneTransforms, outDebugBones, offset);

		return outBoneTransforms.mSize;
    }
}

namespace
{
    void CalculateStaticDebugBones(const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, glm::mat4 lastParentBoneTransform, r2::SArray<r2::draw::DebugBone>& outDebugBones, u64 offset)
    {
        
        glm::mat4 transform = skeletonPart.transform;

		glm::mat4 globalTransform = parentTransform * transform;

		s32 theDefault = -1;
		s32 boneMapResult = r2::shashmap::Get(*model.boneMapping, skeletonPart.hashName, theDefault);

        if (boneMapResult != theDefault)
        {
            u32 boneIndex = boneMapResult;

			if (skeletonPart.parent)
			{
				r2::draw::DebugBone& debugBone = r2::sarr::At(outDebugBones, boneIndex + offset);
				debugBone.p0 = lastParentBoneTransform[3];
				debugBone.p1 = globalTransform[3];

                lastParentBoneTransform = globalTransform;
			}
        }

		u32 numChildren = 0;
		if (skeletonPart.children != nullptr)
		{
			numChildren = r2::sarr::Size(*skeletonPart.children);
		}

        for (u32 i = 0; i < numChildren; ++i)
        {
            CalculateStaticDebugBones(model, r2::sarr::At(*skeletonPart.children, i), globalTransform, lastParentBoneTransform, outDebugBones, offset);
        }
    }

	void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, glm::mat4 lastBoneParentTransform, r2::SArray<glm::mat4>& outTransforms, r2::SArray<r2::draw::DebugBone>& outDebugBones, u64 offset)
	{
     //   r2::util::Timer calcBonesTimer("CalculateBoneTransforms", false);
        //PROFILE_SCOPE_FN   
        glm::mat4 transform = skeletonPart.transform;
        
        r2::draw::AnimationChannel* channel = FindChannel(animation, skeletonPart.hashName);
        

        if (channel)
        {

            //interpolate using the channel
            glm::vec3 scale = CalculateScaling(animationTime, animation.duration, *channel);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
 
            glm::quat rotQ = CalculateRotation(animationTime, animation.duration, *channel);

            glm::vec3 translate = CalculateTranslation(animationTime, animation.duration, *channel);
            glm::mat4 transMat = glm::translate(glm::mat4(1.0f), translate);

            transform = transMat * glm::mat4_cast(rotQ) * scaleMat;
        }
        else
        {
            //For FBX animations, there might be some messed up pivot point nonsense to take care of
            //we probably want to attempt to find the <boneName>_$AssimpFbx$_Rotation and <boneName>_$AssimpFbx$_Scaling
            //animation channels
    //        printf("skeletonPart.boneName: %s\n", skeletonPart.boneName.c_str());

			u64 rotationChannelName = STRING_ID(std::string(skeletonPart.boneName + "_$AssimpFbx$_Rotation").c_str());
			u64 scalingChannelName = STRING_ID(std::string(skeletonPart.boneName + "_$AssimpFbx$_Scaling").c_str());

			r2::draw::AnimationChannel* rotationChannel = FindChannel(animation, rotationChannelName);
			r2::draw::AnimationChannel* scalingChannel = FindChannel(animation, scalingChannelName);

			if (rotationChannel && scalingChannel)
			{
				glm::vec3 scale = CalculateScaling(animationTime, animation.duration, *scalingChannel);
				glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

				glm::quat rotQ = CalculateRotation(animationTime, animation.duration, *rotationChannel);
				transform = glm::mat4_cast(rotQ) * scaleMat;
			}

        }
        
        glm::mat4 globalTransform = parentTransform * transform;
      
        s32 theDefault = -1;
       // auto time = CENG.GetTicks();
        s32 boneMapResult = r2::shashmap::Get(*model.boneMapping, skeletonPart.hashName, theDefault);
      //  printf("Calc time: %f\n", CENG.GetTicks() - time);

        if (boneMapResult != theDefault)
        {
            u32 boneIndex = boneMapResult;
            r2::sarr::At(outTransforms, boneIndex + offset) = model.globalInverseTransform * globalTransform * r2::sarr::At(*model.boneInfo, boneIndex).offsetTransform;
        
            if (skeletonPart.parent)
            {
				r2::draw::DebugBone& debugBone = r2::sarr::At(outDebugBones, boneIndex + offset);

				debugBone.p0 = lastBoneParentTransform[3];
				debugBone.p1 = globalTransform[3];

				lastBoneParentTransform = globalTransform;
            }
        }

        u32 numChildren = 0;
        if (skeletonPart.children != nullptr)
        {
            numChildren = r2::sarr::Size(*skeletonPart.children);
        }
        
     //   
        for (u32 i = 0; i < numChildren; ++i)
        {
            CalculateBoneTransforms(animationTime, animation, model, r2::sarr::At(*skeletonPart.children, i), globalTransform, lastBoneParentTransform, outTransforms, outDebugBones, offset);
        }

      //  s_CalcBoneRuns.push_back(calcBonesTimer.Stop());
    }
    
    r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName)
    {
     //   PROFILE_SCOPE_FN
        u32 numChannels = r2::sarr::Size(*animation.channels);
        for (u32 i = 0; i < numChannels; ++i)
        {
            if (r2::sarr::At(*animation.channels, i).hashName == hashName)
            {
                return &r2::sarr::At(*animation.channels, i);
            }
        }
        
        return nullptr;
    }
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel)
    {
     //   PROFILE_SCOPE_FN
        u64 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        if (numScalingKeys == 1)
        {
            return r2::sarr::At(*channel.scaleKeys, 0).value;
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
      //  double curScaleTime = r2::sarr::At(*channel.scaleKeys, channel.state.mCurScaleIndex).time;
        
        double dt = fabs(nextScaleTime - channel.state.mCurScaleTime);
        
        if (nextScalingIndex < channel.state.mCurScaleIndex)
        {
            dt = totalTime - channel.state.mCurScaleTime;
        }
        
        float factor = (float)glm::clamp((animationTime - channel.state.mCurScaleTime)/ dt, 0.0, 1.0) ;

        glm::vec3 start = r2::sarr::At(*channel.scaleKeys, channel.state.mCurScaleIndex).value;
        glm::vec3 end = r2::sarr::At(*channel.scaleKeys, nextScalingIndex).value;
        
        return r2::math::Lerp(start, end, factor);
    }
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel)
    {
    //    PROFILE_SCOPE_FN
        u64 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        if (numRotationKeys == 1)
        {
            return r2::sarr::At(*channel.rotationKeys, 0).quat;
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
      //  double curRotationTime = r2::sarr::At(*channel.rotationKeys, channel.state.mCurRotationIndex).time;

        double dt = fabs(nextRotationTime - channel.state.mCurRotationTime);
        
        if (nextRotationIndex < channel.state.mCurRotationIndex)
        {
            dt = totalTime - channel.state.mCurRotationTime;
        }
        
        float factor = (float)glm::clamp((animationTime - channel.state.mCurRotationTime)/ dt, 0.0, 1.0) ;
    //    printf("Rotation Factor: %f\n", factor);
		glm::quat start = r2::sarr::At(*channel.rotationKeys, channel.state.mCurRotationIndex).quat;
		glm::quat end = r2::sarr::At(*channel.rotationKeys, nextRotationIndex).quat;

        return r2::math::Slerp(start, end, factor);
    }
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, r2::draw::AnimationChannel& channel)
    {
     //   PROFILE_SCOPE_FN
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        if (numPositionKeys == 1)
        {
            return r2::sarr::At(*channel.positionKeys, 0).value;
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

        glm::vec3 start = r2::sarr::At(*channel.positionKeys, channel.state.mCurTranslationIndex).value;
        glm::vec3 end = r2::sarr::At(*channel.positionKeys, nextTranslationIndex).value;

        
       // r2::util::Timer timer("", false);
        glm::vec3 r = r2::math::Lerp(start, end, factor);


        return r;
    }
    
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
   //     PROFILE_SCOPE_FN
        u32 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        R2_CHECK(numScalingKeys > 0, "We don't have any scaling keys");
        
        for (u32 i = startingIndex; i < numScalingKeys -1; ++i)
        {
            if (animationTime < r2::sarr::At(*channel.scaleKeys, i+1).time)
            {
                return i;
            }
        }

		if (animationTime < totalTime)
		{
			return numScalingKeys - 1;
		}
        
        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
     //   PROFILE_SCOPE_FN
        u32 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        R2_CHECK(numRotationKeys > 0, "We don't have any rotation keys");
        
        for (u32 i = startingIndex; i < numRotationKeys -1; ++i)
        {
            if (animationTime < r2::sarr::At(*channel.rotationKeys, i+1).time)
            {
                return i;
            }
        }

		if (animationTime < totalTime)
		{
			return numRotationKeys - 1;
		}
        
        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel, u32 startingIndex)
    {
     //   PROFILE_SCOPE_FN
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        R2_CHECK(numPositionKeys > 0, "We don't have any position keys");
        

        for (u32 i = startingIndex; i < numPositionKeys-1; ++i)
        {
			if (animationTime < r2::sarr::At(*channel.positionKeys, i + 1).time)
			{
				return i;
			}
        }

		if (animationTime < totalTime)
		{
			return numPositionKeys - 1;
		}

        R2_CHECK(false, "Failed to find a proper position key for time: %f", animationTime);
        
        return 0;
    }
}
