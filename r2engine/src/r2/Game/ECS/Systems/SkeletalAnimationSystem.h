#ifndef __SKELETAL_ANIMATION_SYSTEM_H__
#define __SKELETAL_ANIMATION_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	class SkeletalAnimationSystem : public System
	{
	public:
		SkeletalAnimationSystem();
		~SkeletalAnimationSystem();

		void Update();
	};
}

#endif
