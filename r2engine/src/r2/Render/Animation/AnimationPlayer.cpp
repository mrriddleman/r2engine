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


namespace
{
    void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, r2::SArray<glm::mat4>& outTransforms, u64 offset);
    
    const r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName);
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
}

namespace r2::draw
{
	void PlayAnimationForSkinnedModel(
		u32 timeInMilliseconds,
		const AnimModel& model,
		const AnimationHandle& animationHandle,
		AnimationCache& animationCache,
		r2::SArray<glm::mat4>& outBoneTransforms)
    {

        if (r2::sarr::Capacity(outBoneTransforms) < r2::sarr::Size(*model.boneInfo))
        {
            R2_CHECK(false, "We must have enough space to put the final bone transforms in!");
            return;
        }


        const Animation* anim = animcache::GetAnimation(animationCache, animationHandle);

		if (anim == nullptr)
		{
			outBoneTransforms.mSize += model.boneInfo->mSize;
			for (u32 i = 0; i < model.boneInfo->mSize; ++i)
			{
				r2::sarr::At(outBoneTransforms, i) = glm::mat4(1.0f);
			}

			return;
		}

 
        double ticksPerSecond = anim->ticksPerSeconds != 0 ? anim->ticksPerSeconds : 25.0;
        double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double animationTime = fmod(timeInTicks, anim->duration);

        R2_CHECK(animationTime < anim->duration, "Hmmm");
        
        outBoneTransforms.mSize = model.boneInfo->mSize;
		for (u32 i = 0; i < model.boneInfo->mSize; ++i)
		{
			r2::sarr::At(outBoneTransforms, i) = glm::mat4(1.0f);
		}

        CalculateBoneTransforms(animationTime, *anim, model, model.skeleton, glm::mat4(1.0f), outBoneTransforms, 0);
    }

	u32 PlayAnimationForAnimModel(
		u32 timeInMilliseconds,
		const AnimModel& model,
		const AnimationHandle& animationHandle,
		AnimationCache& animationCache,
		r2::SArray<glm::mat4>& outBoneTransforms,
		u64 offset) 
    {

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

            return outBoneTransforms.mSize;
        }

		double ticksPerSecond = anim->ticksPerSeconds != 0 ? anim->ticksPerSeconds : 25.0;
		double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double duration = anim->duration;

		double animationTime = fmod(timeInTicks, duration);

		R2_CHECK(animationTime < anim->duration, "Hmmm");

        outBoneTransforms.mSize += model.boneInfo->mSize;

		for (u32 i = 0; i < model.boneInfo->mSize; ++i)
		{
			r2::sarr::At(outBoneTransforms, i + offset) = glm::mat4(1.0f);
		}

        CalculateBoneTransforms(animationTime, *anim, model, model.skeleton, glm::mat4(1.0f), outBoneTransforms, offset);

		return outBoneTransforms.mSize;
    }
}

namespace
{
	void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, const r2::draw::AnimModel& model, const r2::draw::Skeleton& skeletonPart, const glm::mat4& parentTransform, r2::SArray<glm::mat4>& outTransforms, u64 offset)
	{
        glm::mat4 transform = skeletonPart.transform;

        const r2::draw::AnimationChannel* channel = FindChannel(animation, skeletonPart.hashName);
        
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

            u64 rotationChannelName = STRING_ID(std::string(skeletonPart.boneName + "_$AssimpFbx$_Rotation").c_str());
            u64 scalingChannelName = STRING_ID(std::string(skeletonPart.boneName + "_$AssimpFbx$_Scaling").c_str());

            const r2::draw::AnimationChannel* rotationChannel = FindChannel(animation, rotationChannelName);
            const r2::draw::AnimationChannel* scalingChannel = FindChannel(animation, scalingChannelName);

            if (rotationChannel && scalingChannel)
            {
				glm::vec3 scale = CalculateScaling(animationTime, animation.duration, *scalingChannel);
				glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

                glm::quat rotQ = CalculateRotation(animationTime, animation.duration, *rotationChannel);
                transform = glm::mat4_cast(rotQ) * scaleMat;
            }

            
        }
        
        glm::mat4 globalTransform =  parentTransform * transform ;
      
        s32 theDefault = -1;
        s32 boneMapResult = r2::shashmap::Get(*model.boneMapping, skeletonPart.hashName, theDefault);

        if (boneMapResult != theDefault)
        {
            u32 boneIndex = boneMapResult;
            r2::sarr::At(outTransforms, boneIndex + offset) = model.globalInverseTransform * globalTransform * r2::sarr::At(*model.boneInfo, boneIndex).offsetTransform;
        }

        u32 numChildren = 0;
        if (skeletonPart.children != nullptr)
        {
            numChildren = r2::sarr::Size(*skeletonPart.children);
        }

        for (u32 i = 0; i < numChildren; ++i)
        {
            CalculateBoneTransforms(animationTime, animation, model, r2::sarr::At(*skeletonPart.children, i), globalTransform, outTransforms, offset);
        }
    }
    
    const r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, u64 hashName)
    {
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
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u64 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        if (numScalingKeys == 1)
        {
            return r2::sarr::At(*channel.scaleKeys, 0).value;//channel.scaleKeys[0].value;
        }
        
        u32 curScalingIndex = FindScalingIndex(animationTime, totalTime, channel);
        u32 nextScalingIndex = (curScalingIndex + 1) % numScalingKeys;
        
        R2_CHECK(nextScalingIndex < numScalingKeys, "nextScalingIndex: %u is larger or equal to the number of scaling keys: %u", nextScalingIndex, numScalingKeys);
        

        double nextScaleTime = r2::sarr::At(*channel.scaleKeys, nextScalingIndex).time;
        double curScaleTime = r2::sarr::At(*channel.scaleKeys, curScalingIndex).time;

        double dt = fabs(nextScaleTime - curScaleTime);
        
        if (nextScalingIndex < curScalingIndex)
        {
            dt = totalTime - curScaleTime;
        }
        
        float factor = (float)glm::clamp((animationTime - curScaleTime )/ dt, 0.0, 1.0) ;
     //   printf("Scale Factor: %f\n", factor);
        glm::vec3 start = r2::sarr::At(*channel.scaleKeys, curScalingIndex).value;
        glm::vec3 end = r2::sarr::At(*channel.scaleKeys, nextScalingIndex).value;
        
        return r2::math::Lerp(start, end, factor);
    }
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u64 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        if (numRotationKeys == 1)
        {
            return r2::sarr::At(*channel.rotationKeys, 0).quat;
        }
        
        u32 curRotIndex = FindRotationIndex(animationTime, totalTime,  channel);
        u32 nextRotIndex = (curRotIndex + 1) % numRotationKeys;
        
        R2_CHECK(nextRotIndex < numRotationKeys, "curRotIndex: %u is larger or equal to the number of rotation keys: %u", curRotIndex, numRotationKeys);
        
		double nextRotationTime = r2::sarr::At(*channel.rotationKeys, nextRotIndex).time;
		double curRotationTime = r2::sarr::At(*channel.rotationKeys, curRotIndex).time;

        double dt = fabs(nextRotationTime - curRotationTime);
        
        if (nextRotIndex < curRotIndex)
        {
            dt = totalTime - curRotationTime;
        }
        
        float factor = (float)glm::clamp((animationTime - curRotationTime )/ dt, 0.0, 1.0) ;
    //    printf("Rotation Factor: %f\n", factor);
		glm::quat start = r2::sarr::At(*channel.rotationKeys, curRotIndex).quat;
		glm::quat end = r2::sarr::At(*channel.rotationKeys, nextRotIndex).quat;

        return r2::math::Slerp(start, end, factor);
    }
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        if (numPositionKeys == 1)
        {
            return r2::sarr::At(*channel.positionKeys, 0).value;
        }
        
        u32 curPositionIndex = FindPositionIndex(animationTime, totalTime, channel);
        u32 nextPositionIndex = (curPositionIndex + 1) % numPositionKeys;
        
        R2_CHECK(nextPositionIndex < numPositionKeys, "nextPositionIndex: %u is larger or equal to the number of position keys: %u", nextPositionIndex, numPositionKeys);
        
        double nextPosTime = r2::sarr::At(*channel.positionKeys, nextPositionIndex).time;
        double curPosTime = r2::sarr::At(*channel.positionKeys, curPositionIndex).time;

        double dt = fabs(nextPosTime - curPosTime);
        
        if (nextPositionIndex < curPositionIndex)
        {
            dt = totalTime - curPosTime;
        }
        
        float factor = (float)glm::clamp((animationTime - curPosTime )/ dt, 0.0, 1.0) ;
     //   printf("Translation Factor: %f\n", factor);
        glm::vec3 start = r2::sarr::At(*channel.positionKeys, curPositionIndex).value;
        glm::vec3 end = r2::sarr::At(*channel.positionKeys, nextPositionIndex).value;

        return r2::math::Lerp(start, end, factor);
    }
    
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u32 numScalingKeys = r2::sarr::Size(*channel.scaleKeys);
        R2_CHECK(numScalingKeys > 0, "We don't have any scaling keys");
        
        for (u32 i = 0; i < numScalingKeys -1; ++i)
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
    
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u32 numRotationKeys = r2::sarr::Size(*channel.rotationKeys);
        R2_CHECK(numRotationKeys > 0, "We don't have any rotation keys");
        
        for (u32 i = 0; i < numRotationKeys -1; ++i)
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
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        u32 numPositionKeys = r2::sarr::Size(*channel.positionKeys);
        R2_CHECK(numPositionKeys > 0, "We don't have any position keys");
        
        for (u32 i = 0; i < numPositionKeys-1; ++i)
        {
            if (animationTime < r2::sarr::At(*channel.positionKeys, i+1).time)
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