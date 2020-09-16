//
//  AnimationPlayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//

#ifndef AnimationPlayer_h
#define AnimationPlayer_h

#include "glm/glm.hpp"

namespace r2::draw
{
    struct AnimModel;
    void PlayAnimationForSkinnedModel(u32 timeInMilliseconds, AnimModel& model, u32 animationId, r2::SArray<glm::mat4>& outBoneTransforms);
    u32 PlayAnimationForAnimModel(u32 timeInMilliseconds, AnimModel& model, u32 animationId, r2::SArray<glm::mat4>& outBoneTransforms, u64 offset);
}

#endif /* AnimationPlayer_hpp */
