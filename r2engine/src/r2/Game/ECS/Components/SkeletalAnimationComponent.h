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
		u64 animModelAssetName;
		u32 startingAnimationIndex;
		u32 startTime;
		b32 shouldLoop;
		b32 shouldUseSameTransformsForAllInstances;

		//These shouldn't be edited directly in the editor just set when the above is set
		u32 currentAnimationIndex;
		const r2::draw::Model* animModel;
		r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
	};
}


#endif // __SKELETAL_ANIMATION_COMPONENT_H__
