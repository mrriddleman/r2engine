#ifndef __GRID_POSITION_COMPONENT_SERIALIZATION_H__
#define __GRID_POSITION_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "InstancedGridPositionComponentArrayData_generated.h"
#include "r2/Game/ECS/ComponentArrayData_generated.h"
#include "GridPosition_generated.h"
#include "GridPositionComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Core/Memory/Memory.h"
#include "GridPositionComponent.h"

#include <glm/glm.hpp>

namespace r2::ecs
{
	template<>
	void SerializeComponentArray<GridPositionComponent>(flatbuffers::FlatBufferBuilder& builder, const r2::SArray<GridPositionComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::GridPositionComponentData>>* gridPositionComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::GridPositionComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const GridPositionComponent& gridPositionComponent = r2::sarr::At(components, i);

			auto flatLocalGridPosition = flat::CreateGridPosition(builder, gridPositionComponent.localGridPosition.x, gridPositionComponent.localGridPosition.y, gridPositionComponent.localGridPosition.z);
			auto flatGlobalGridPosition = flat::CreateGridPosition(builder, gridPositionComponent.globalGridPosition.x, gridPositionComponent.globalGridPosition.y, gridPositionComponent.globalGridPosition.z);

			flat::GridPositionComponentDataBuilder gridPositionComponentBuilder(builder);

			gridPositionComponentBuilder.add_localGridPosition(flatLocalGridPosition);
			gridPositionComponentBuilder.add_globalGridPosition(flatGlobalGridPosition);

			r2::sarr::Push(*gridPositionComponents, gridPositionComponentBuilder.Finish());
		}

		auto vec = builder.CreateVector(gridPositionComponents->mData, gridPositionComponents->mSize);

		flat::GridPositionComponentArrayDataBuilder gridPositionComponentArrayDataBuilder(builder);

		gridPositionComponentArrayDataBuilder.add_gridPositionComponentArray(vec);

		builder.Finish(gridPositionComponentArrayDataBuilder.Finish());

		FREE(gridPositionComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<GridPositionComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::GridPositionComponentArrayData* gridPositionComponentArrayData = flatbuffers::GetRoot<flat::GridPositionComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = gridPositionComponentArrayData->gridPositionComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::GridPositionComponentData* flatGridPositionComponent = componentVector->Get(i);


			const auto flatLocalGridPosition = flatGridPositionComponent->localGridPosition();
			const auto flatGlobalGridPosition = flatGridPositionComponent->globalGridPosition();

			GridPositionComponent gridPositionComponent;
			gridPositionComponent.localGridPosition = glm::ivec3(flatLocalGridPosition->x(), flatLocalGridPosition->y(), flatLocalGridPosition->z());
			gridPositionComponent.globalGridPosition = glm::ivec3(flatGlobalGridPosition->x(), flatGlobalGridPosition->y(), flatGlobalGridPosition->z());


			r2::sarr::Push(components, gridPositionComponent);
		}
	}

	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<InstanceComponentT<GridPositionComponent>>& components)
	{
		const auto numInstancedComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::GridPositionComponentArrayData>>* instancedGridPositionComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::GridPositionComponentArrayData>, numInstancedComponents);

		for (u32 i = 0; i < numInstancedComponents; ++i)
		{
			const InstanceComponentT<GridPositionComponent>& instancedGridPositionComponent = r2::sarr::At(components, i);

			const auto numInstances = r2::sarr::Size(*instancedGridPositionComponent.instances);

			r2::SArray<flatbuffers::Offset<flat::GridPositionComponentData>>* gridPositionComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::GridPositionComponentData>, numInstances);

			for (u32 j = 0; j < numInstances; ++j)
			{
				const GridPositionComponent& gridPositionComponent = r2::sarr::At(*instancedGridPositionComponent.instances, j);

				auto flatLocalGridPosition = flat::CreateGridPosition(fbb, gridPositionComponent.localGridPosition.x, gridPositionComponent.localGridPosition.y, gridPositionComponent.localGridPosition.z);
				auto flatGlobalGridPosition = flat::CreateGridPosition(fbb, gridPositionComponent.globalGridPosition.x, gridPositionComponent.globalGridPosition.y, gridPositionComponent.globalGridPosition.z);

				flat::GridPositionComponentDataBuilder gridPositionComponentBuilder(fbb);

				gridPositionComponentBuilder.add_localGridPosition(flatLocalGridPosition);
				gridPositionComponentBuilder.add_globalGridPosition(flatGlobalGridPosition);

				r2::sarr::Push(*gridPositionComponents, gridPositionComponentBuilder.Finish());
			}

			auto vec = fbb.CreateVector(gridPositionComponents->mData, gridPositionComponents->mSize);

			flat::GridPositionComponentArrayDataBuilder gridPositionComponentArrayDataBuilder(fbb);

			gridPositionComponentArrayDataBuilder.add_gridPositionComponentArray(vec);

			r2::sarr::Push(*instancedGridPositionComponents, gridPositionComponentArrayDataBuilder.Finish());

			FREE(gridPositionComponents, *MEM_ENG_SCRATCH_PTR);
		}

		auto vec = fbb.CreateVector(instancedGridPositionComponents->mData, instancedGridPositionComponents->mSize);
		flat::InstancedGridPositionComponentArrayDataBuilder instancedGridPositionComponentArrayDataBuilder(fbb);
		instancedGridPositionComponentArrayDataBuilder.add_instancedGridPositionComponentArray(vec);
		fbb.Finish(instancedGridPositionComponentArrayDataBuilder.Finish());
		FREE(instancedGridPositionComponents, *MEM_ENG_SCRATCH_PTR);
	}


	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<InstanceComponentT<GridPositionComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::InstancedGridPositionComponentArrayData* instancedGridPositionComponentArrayData = flatbuffers::GetRoot<flat::InstancedGridPositionComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = instancedGridPositionComponentArrayData->instancedGridPositionComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::GridPositionComponentArrayData* flatInstancedGridPositionComponent = componentVector->Get(i);

			InstanceComponentT<GridPositionComponent> instancedGridPositionComponent;

			instancedGridPositionComponent.numInstances = flatInstancedGridPositionComponent->gridPositionComponentArray()->size();

			instancedGridPositionComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, GridPositionComponent, instancedGridPositionComponent.numInstances);

			for (flatbuffers::uoffset_t j = 0; j < instancedGridPositionComponent.numInstances; ++j)
			{
				const flat::GridPositionComponentData* flatGridPositionComponent = flatInstancedGridPositionComponent->gridPositionComponentArray()->Get(j);

				const auto flatLocalGridPosition = flatGridPositionComponent->localGridPosition();
				const auto flatGlobalGridPosition = flatGridPositionComponent->globalGridPosition();

				GridPositionComponent gridPositionComponent;
				gridPositionComponent.localGridPosition = glm::ivec3(flatLocalGridPosition->x(), flatLocalGridPosition->y(), flatLocalGridPosition->z());
				gridPositionComponent.globalGridPosition = glm::ivec3(flatGlobalGridPosition->x(), flatGlobalGridPosition->y(), flatGlobalGridPosition->z());

				r2::sarr::Push(*instancedGridPositionComponent.instances, gridPositionComponent);
			}

			r2::sarr::Push(components, instancedGridPositionComponent);
		}
	}
}


#endif // __FACING_COMPONENT_SERIALIZATIOn
