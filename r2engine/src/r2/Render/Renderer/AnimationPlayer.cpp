//
//  AnimationPlayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//
#include "r2pch.h"
#include "AnimationPlayer.h"
#include "r2/Render/Renderer/SkinnedModel.h"

#include "r2/Core/Math/MathUtils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace
{
    void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation, r2::draw::SkinnedModel& model, const r2::draw::SkeletonPart& skeletonPart, const glm::mat4& parentTransform);
    
    const r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, const std::string& name);
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel);
}

namespace r2::draw
{
    std::vector<glm::mat4> PlayAnimationForSkinnedModel(u32 timeInMilliseconds,  SkinnedModel& model, u32 animationId)
    {
        R2_CHECK(animationId < model.animations.size(), "Invalid animation id given: %u but only have %zu animations", animationId, model.animations.size());
        
        std::vector<glm::mat4> boneTransforms{};

        const Animation& anim = model.animations[animationId];

        double ticksPerSecond = anim.ticksPerSeconds != 0 ? anim.ticksPerSeconds : 25.0;
        double timeInTicks = r2::util::MillisecondsToSeconds(timeInMilliseconds) * ticksPerSecond;
        double animationTime = fmod(timeInTicks, anim.duration);

        R2_CHECK(animationTime < anim.duration, "Hmmm");
        
        CalculateBoneTransforms(animationTime, anim, model, model.skeleton, glm::mat4(1.0f));

        boneTransforms.resize(model.boneInfos.size(), glm::mat4(1.0f));
        for (u32 i = 0; i < model.boneInfos.size(); ++i)
        {
            boneTransforms[i] = model.boneInfos[i].finalTransform;
        }
        
        return boneTransforms;
    }
}

namespace
{
    void CalculateBoneTransforms(f64 animationTime, const r2::draw::Animation& animation,  r2::draw::SkinnedModel& model, const r2::draw::SkeletonPart& skeletonPart, const glm::mat4& parentTransform)
    {
        glm::mat4 transform = skeletonPart.transform;

        const r2::draw::AnimationChannel* channel = FindChannel(animation, skeletonPart.name);
        
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
        
        glm::mat4 globalTransform =  parentTransform * transform ;
        
        auto boneMapResult = model.boneMapping.find(skeletonPart.name);
        if (boneMapResult != model.boneMapping.end())
        {
            u32 boneIndex = boneMapResult->second;
            
            model.boneInfos[boneIndex].finalTransform = model.globalInverseTransform* globalTransform * model.boneInfos[boneIndex].offsetTransform;
        }
        
        for (u32 i = 0; i < skeletonPart.children.size(); ++i)
        {
            CalculateBoneTransforms(animationTime, animation, model, skeletonPart.children[i], globalTransform);
        }
    }
    
    const r2::draw::AnimationChannel* FindChannel(const r2::draw::Animation& animation, const std::string& name)
    {
        for (u32 i = 0; i < animation.channels.size(); ++i)
        {
            if (animation.channels[i].name == name)
            {
                return &animation.channels[i];
            }
        }
        
        return nullptr;
    }
    
    glm::vec3 CalculateScaling(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        if (channel.numScalingKeys == 1)
        {
            return channel.scaleKeys[0].value;
        }
        
        u32 curScalingIndex = FindScalingIndex(animationTime, totalTime, channel);
        u32 nextScalingIndex = (curScalingIndex + 1) % channel.numScalingKeys;
        
        R2_CHECK(nextScalingIndex < channel.numScalingKeys, "nextScalingIndex: %u is larger or equal to the number of scaling keys: %u", nextScalingIndex, channel.numScalingKeys);
        
        double dt = fabs(channel.scaleKeys[nextScalingIndex].time - channel.scaleKeys[curScalingIndex].time);
        
        if (nextScalingIndex < curScalingIndex)
        {
            dt = totalTime - channel.scaleKeys[curScalingIndex].time;
        }
        
        float factor = (float)glm::clamp(animationTime - channel.scaleKeys[curScalingIndex].time / dt, 0.0, 1.0) ;

        glm::vec3 start = channel.scaleKeys[curScalingIndex].value;
        glm::vec3 end = channel.scaleKeys[nextScalingIndex].value;
        
        return r2::math::Lerp(start, end, factor);
    }
    
    glm::quat CalculateRotation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        if (channel.numRotationKeys == 1)
        {
            return channel.rotationKeys[0].quat;
        }
        
        u32 curRotIndex = FindRotationIndex(animationTime, totalTime,  channel);
        u32 nextRotIndex = (curRotIndex + 1) % channel.numRotationKeys;
        
        R2_CHECK(nextRotIndex < channel.numRotationKeys, "curRotIndex: %u is larger or equal to the number of rotation keys: %u", curRotIndex, channel.numRotationKeys);
        
        double dt = fabs(channel.rotationKeys[nextRotIndex].time - channel.rotationKeys[curRotIndex].time);
        
        if (nextRotIndex < curRotIndex)
        {
            dt = totalTime - channel.rotationKeys[curRotIndex].time;
        }
        
        float factor = (float)glm::clamp(animationTime - channel.rotationKeys[curRotIndex].time / dt, 0.0, 1.0) ;
        
        return r2::math::Slerp(channel.rotationKeys[curRotIndex].quat, channel.rotationKeys[nextRotIndex].quat, factor);
    }
    
    glm::vec3 CalculateTranslation(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        if (channel.numPositionKeys == 1)
        {
            return channel.positionKeys[0].value;
        }
        
        u32 curPositionIndex = FindPositionIndex(animationTime, totalTime, channel);
        u32 nextPositionIndex = (curPositionIndex + 1) % channel.numPositionKeys;
        
        R2_CHECK(nextPositionIndex < channel.numPositionKeys, "nextPositionIndex: %u is larger or equal to the number of position keys: %u", nextPositionIndex, channel.numPositionKeys);
        
        double dt = fabs(channel.positionKeys[nextPositionIndex].time - channel.positionKeys[curPositionIndex].time);
        
        if (nextPositionIndex < curPositionIndex)
        {
            dt = totalTime - channel.positionKeys[curPositionIndex].time;
        }
        
        float factor = (float)glm::clamp(animationTime - channel.positionKeys[curPositionIndex].time / dt, 0.0, 1.0) ;
        
        glm::vec3 start = channel.positionKeys[curPositionIndex].value;
        glm::vec3 end = channel.positionKeys[nextPositionIndex].value;

        return r2::math::Lerp(start, end, factor);
    }
    
    u32 FindScalingIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        R2_CHECK(channel.numScalingKeys > 0, "We don't have any scaling keys");
        
        for (u32 i = 0; i < channel.numScalingKeys-1; ++i)
        {
            if (animationTime < (float)channel.scaleKeys[i+1].time)
            {
                return i;
            }
        }
        
        if (animationTime < totalTime)
        {
            return channel.numScalingKeys - 1;
        }
        
        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindRotationIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        R2_CHECK(channel.numRotationKeys > 0, "We don't have any rotation keys");
        
        for (u32 i = 0; i < channel.numRotationKeys-1; ++i)
        {
            if (animationTime < (float)channel.rotationKeys[i+1].time)
            {
                return i;
            }
        }
        
        if (animationTime < totalTime)
        {
            return channel.numRotationKeys - 1;
        }
        
        R2_CHECK(false, "Failed to find a proper scaling key for time: %f", animationTime);
        
        return 0;
    }
    
    u32 FindPositionIndex(f64 animationTime, f64 totalTime, const r2::draw::AnimationChannel& channel)
    {
        R2_CHECK(channel.numPositionKeys > 0, "We don't have any position keys");
        
        for (u32 i = 0; i < channel.numPositionKeys-1; ++i)
        {
            if (animationTime < (float)channel.positionKeys[i+1].time)
            {
                return i;
            }
        }
        
        if (animationTime < totalTime)
        {
            return channel.numPositionKeys - 1;
        }
        
        R2_CHECK(false, "Failed to find a proper position key for time: %f", animationTime);
        
        return 0;
    }
}
