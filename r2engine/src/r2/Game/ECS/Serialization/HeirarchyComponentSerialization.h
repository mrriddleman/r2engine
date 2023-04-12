#ifndef __HEIRARCHY_COMPONENT_SERIALIZATION_H__
#define __HEIRARCHY_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/HeirarchyComponent.h"

namespace r2::ecs
{
/*
	struct HeirarchyComponent
	{
		Entity parent = INVALID_ENTITY;
	};
*/
	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<HeirarchyComponent>& components)
	{
		auto start = builder.StartVector();
		const auto numComponents = r2::sarr::Size(components);
		for (u32 i = 0; i < numComponents; ++i)
		{
			builder.UInt(static_cast<u32>(r2::sarr::At(components, i).parent));
		}
		builder.EndVector(start, false, false);
	}
}

#endif
