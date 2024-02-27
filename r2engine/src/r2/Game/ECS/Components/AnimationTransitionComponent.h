#ifndef __ANIMATION_TRANSITION_COMPONENT_H__
#define __ANIMATION_TRANSITION_COMPONENT_H__

namespace r2::ecs
{
	struct AnimationTransitionComponent
	{
		s32 currentAnimationIndex;
		s32 nextAnimationIndex;
		b32 loop;

		f32 timeToTransition; //how much time is needed to transition to the new animation
		f32 currentTransitionTime; //current time in the transition
		
	};
}

#endif