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

	template<>
	inline void DeSerializeComponentArray(r2::SArray<HeirarchyComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			u32 savedEntity = componentVector[i].AsUInt32();


			//We can't just map the entity directly since we don't guarantee the same entity values
			s32 entityIndex = -1;
			for (u32 j= 0; j < r2::sarr::Size(*refEntities); ++j)
			{
				if (r2::sarr::At(*refEntities, j)->entityID() == savedEntity)
				{
					entityIndex = j;
					break;
				}
			}

			R2_CHECK(entityIndex != -1, "Should never be -1");

			HeirarchyComponent heirarchyComponent;
			heirarchyComponent.parent = r2::sarr::At(*entities, entityIndex);

			r2::sarr::Push(components, heirarchyComponent);
		}
	}
}

#endif
