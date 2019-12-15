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
    struct SkinnedModel;
    std::vector<glm::mat4> PlayAnimationForSkinnedModel(u32 timeInMilliseconds, SkinnedModel& model, u32 animationId);
}

#endif /* AnimationPlayer_hpp */
