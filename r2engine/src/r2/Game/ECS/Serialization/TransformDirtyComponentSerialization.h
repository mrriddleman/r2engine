#ifndef __TRANSFORM_DIRTY_COMPONENT_SERIALIZATION_H__
#define __TRANSFORM_DIRTY_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<TransformDirtyComponent>& components)
	{
		//don't save anything
	}
}

#endif