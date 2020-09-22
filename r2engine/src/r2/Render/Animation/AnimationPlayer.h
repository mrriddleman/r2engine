//
//  AnimationPlayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//

#ifndef AnimationPlayer_h
#define AnimationPlayer_h

#include "glm/glm.hpp"
#include "r2/Render/Animation/AnimationCache.h"

namespace r2::draw
{
    struct AnimModel;

    void PlayAnimationForSkinnedModel(
        u32 timeInMilliseconds,
        const AnimModel& model,
        const AnimationHandle& animationHandle,
        AnimationCache& animationCache,
        r2::SArray<glm::mat4>& outBoneTransforms);

    u32 PlayAnimationForAnimModel(
        u32 timeInMilliseconds,
        const AnimModel& model,
		const AnimationHandle& animationHandle,
		AnimationCache& animationCache,
        r2::SArray<glm::mat4>& outBoneTransforms,
        u64 offset);
}

#endif /* AnimationPlayer_hpp */
