#ifndef __HEIRARCHY_COMPONENT_SERIALIZATION_H__
#define __HEIRARCHY_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/HeirarchyComponent.h"
#include "r2/Game/ECS/Serialization/HeirarchyComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
/*
	struct HeirarchyComponent
	{
		Entity parent = INVALID_ENTITY;
	};
*/
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<HeirarchyComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::HeirarchyComponentData>>* heirarchyComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::HeirarchyComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const HeirarchyComponent& heirarchyComponent = r2::sarr::At(components, i);

			flat::HeirarchyComponentDataBuilder heirarchyComponentBuilder(fbb);

			heirarchyComponentBuilder.add_parent(heirarchyComponent.parent);

			r2::sarr::Push(*heirarchyComponents, heirarchyComponentBuilder.Finish());
		}

		auto vec = fbb.CreateVector(heirarchyComponents->mData, heirarchyComponents->mSize);
		
		flat::HeirarchyComponentArrayDataBuilder heirarchyComponentArrayDataBuilder(fbb);

		heirarchyComponentArrayDataBuilder.add_heirarchyComponentArray(vec);

		fbb.Finish(heirarchyComponentArrayDataBuilder.Finish());

		FREE(heirarchyComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<HeirarchyComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::HeirarchyComponentArrayData* heirarchyComponentArrayData = flatbuffers::GetRoot<flat::HeirarchyComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = heirarchyComponentArrayData->heirarchyComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::HeirarchyComponentData* flatHeirarchyComponent = componentVector->Get(i);

			//We can't just map the entity directly since we don't guarantee the same entity values
			s32 entityIndex = -1;
			for (u32 j= 0; j < r2::sarr::Size(*refEntities); ++j)
			{
				if (r2::sarr::At(*refEntities, j)->entityID() == flatHeirarchyComponent->parent())
				{
					entityIndex = j;
					break;
				}
			}

			Entity parent = 0;
			if (entityIndex != -1)
			{
				parent = r2::sarr::At(*entities, entityIndex);
			}

			HeirarchyComponent heirarchyComponent;
			heirarchyComponent.parent = parent;

			r2::sarr::Push(components, heirarchyComponent);
		}
	}
}

#endif
