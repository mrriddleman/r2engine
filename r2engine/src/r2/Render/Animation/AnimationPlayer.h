//
//  AnimationPlayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//

#ifndef AnimationPlayer_h
#define AnimationPlayer_h

#include "glm/glm.hpp"
#include "r2/Render/Model/Model.h"

namespace r2::draw
{
	u32 PlayAnimationForAnimModel(
		u32 timeInMilliseconds,
		u32 startTime,
		bool loop,
		const Model& model,
		const Animation* animation,
		r2::SArray<ShaderBoneTransform>& outBoneTransforms,
		r2::SArray<DebugBone>* outDebugBones,
		u64 offset);
}

#endif /* AnimationPlayer_hpp */
