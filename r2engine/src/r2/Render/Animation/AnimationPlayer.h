//
//  AnimationPlayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-14.
//

#ifndef AnimationPlayer_h
#define AnimationPlayer_h

namespace r2::anim
{
	struct AnimationClip;
	struct Pose;

	float PlayAnimationClip(const AnimationClip& clip, Pose& animatedPose, float time, bool loop);
}

#endif /* AnimationPlayer_hpp */
