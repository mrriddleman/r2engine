#ifndef __DEBUG_RENDER_COMPONENT_SERIALIZATION_H__
#define __DEBUG_RENDER_COMPONENT_SERIALIZATION_H__

#ifdef R2_DEBUG
#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<DebugRenderComponent>& components)
	{
		//don't save anything
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<InstanceComponentT< DebugRenderComponent> >& components)
	{
		//don't save anything
	}
}
#endif
#endif