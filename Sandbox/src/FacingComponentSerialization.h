#ifndef __FACING_COMPONENT_SERIALIZATION_H__
#define __FACING_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/ComponentArrayData_generated.h"
#include "FacingComponent.h"
#include "FacingComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::ecs
{
	template<>
	void SerializeComponentArray<FacingComponent>(flatbuffers::FlatBufferBuilder& builder, const r2::SArray<FacingComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::FacingComponentData>>* facingComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::FacingComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const FacingComponent& facingComponent = r2::sarr::At(components, i);

			flat::FacingComponentDataBuilder facingComponentBuilder(builder);

			facingComponentBuilder.add_x(facingComponent.facing.x);
			facingComponentBuilder.add_y(facingComponent.facing.y);
			facingComponentBuilder.add_z(facingComponent.facing.z);

			r2::sarr::Push(*facingComponents, facingComponentBuilder.Finish());
		}

		auto vec = builder.CreateVector(facingComponents->mData, facingComponents->mSize);

		flat::FacingComponentArrayDataBuilder facingComponentArrayDataBuilder(builder);

		facingComponentArrayDataBuilder.add_facingComponentArray(vec);

		builder.Finish(facingComponentArrayDataBuilder.Finish());

		FREE(facingComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<FacingComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::FacingComponentArrayData* facingComponentArrayData = flatbuffers::GetRoot<flat::FacingComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = facingComponentArrayData->facingComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::FacingComponentData* flatFacingComponent = componentVector->Get(i);

			FacingComponent facingComponent;
			facingComponent.facing = glm::vec3(flatFacingComponent->x(), flatFacingComponent->y(), flatFacingComponent->z());

			r2::sarr::Push(components, facingComponent);
		}
	}
}


#endif // __FACING_COMPONENT_SERIALIZATIOn
