#ifndef __DEBUG_BONE_COMPONENT_SERIALIZATION_H__
#define __DEBUG_BONE_COMPONENT_SERIALIZATION_H__

#ifdef R2_DEBUG
#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<DebugBoneComponent>& components)
	{
		//don't save anything
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<InstanceComponentT< DebugBoneComponent> >& components)
	{
		//don't save anything
	}
}

#endif
#endif