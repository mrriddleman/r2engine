#ifndef __SKELETAL_ANIMATION_COMPONENT_H__
#define __SKELETAL_ANIMATION_COMPONENT_H__

#include "r2/Core/Containers/SArray.h"

#include "r2/Render/Animation/Animation.h"
#include "r2/Render/Model/Model.h"
#include "r2/Utils/Utils.h"

namespace r2::ecs
{
	struct SkeletalAnimationComponent
	{
		b32 shouldUseSameTransformsForAllInstances;
		const r2::draw::AnimModel* animModel;

		r2::SArray<u32>* startTimePerInstance;
		r2::SArray<b32>* loopPerInstance;
		r2::SArray<r2::draw::Animation*>* animationsPerInstance;

		r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
		r2::SArray<r2::draw::DebugBone>* debugBones;
	};
}


#endif // __SKELETAL_ANIMATION_COMPONENT_H__
