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
		u32 startTime;
		b32 loop;
		const r2::draw::AnimModel* animModel;
		const r2::draw::Animation* animation;
		r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
		r2::SArray<r2::draw::DebugBone>* debugBones;
	};
}


#endif // __SKELETAL_ANIMATION_COMPONENT_H__
