//
//  AnimationPlayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//
#include "r2pch.h"
#include "r2/Render/Animation/AnimationClip.h"

namespace r2::anim
{
    float PlayAnimationClip(const AnimationClip& clip, Pose& animatedPose, float time, bool loop)
    {
        return clip.Sample(animatedPose, time, loop);
    }
}