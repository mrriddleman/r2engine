#ifndef __DEBUG_BONE_COMPONENT_H__
#define __DEBUG_BONE_COMPONENT_H__

#ifdef R2_DEBUG

#include "r2/Render/Model/Model.h"

namespace r2::ecs
{
	struct DebugBoneComponent
	{
		glm::vec4 color;
		r2::SArray<r2::draw::DebugBone>* debugBones;
	};
}

#endif

#endif // __DEBUG_BONE_COMPONENT_H__
