#ifndef __TRANSFORM_COMPONENT_SERIALIZATION_H__
#define __TRANSFORM_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Serialization/TransformComponentArrayData_generated.h"
#include "r2/Game/ECS/Serialization/InstancedTransformComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	/*
	* 	struct Transform
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = glm::quat(1, 0, 0, 0);
	};

	struct TransformComponent
	{
		r2::math::Transform localTransform; //local only
		r2::math::Transform accumTransform; //parent + local?
		glm::mat4 modelMatrix; //full world matrix including parents
	};
	*/

	inline void SerializeTransform(flatbuffers::FlatBufferBuilder& fbb,
		r2::SArray<flatbuffers::Offset<flat::TransformComponentData>>* transformComponents,
		const TransformComponent& transformComponent)
	{
		auto positionData = flat::CreateTransformComponentPosition(
			fbb,
			transformComponent.localTransform.position.x,
			transformComponent.localTransform.position.y,
			transformComponent.localTransform.position.z);

		auto rotationData = flat::CreateTransformComponentRotation(
			fbb,
			transformComponent.localTransform.rotation.x,
			transformComponent.localTransform.rotation.y,
			transformComponent.localTransform.rotation.z,
			transformComponent.localTransform.rotation.w);

		auto scaleData = flat::CreateTransformComponentScale(
			fbb,
			transformComponent.localTransform.scale.x,
			transformComponent.localTransform.scale.y,
			transformComponent.localTransform.scale.z);

		flat::TransformComponentDataBuilder transformComponentBuilder(fbb);

		transformComponentBuilder.add_position(positionData);
		transformComponentBuilder.add_rotation(rotationData);
		transformComponentBuilder.add_scale(scaleData);

		r2::sarr::Push(*transformComponents, transformComponentBuilder.Finish());
	}

	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<TransformComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::TransformComponentData>>* transformComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::TransformComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const TransformComponent& transformComponent = r2::sarr::At(components, i);

			SerializeTransform(fbb, transformComponents, transformComponent);
		}

		flat::TransformComponentArrayDataBuilder transformComponentArrayDataBuilder(fbb);

		transformComponentArrayDataBuilder.add_transformComponentArray(fbb.CreateVector(transformComponents->mData, transformComponents->mSize));

		fbb.Finish(transformComponentArrayDataBuilder.Finish());

		FREE(transformComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<InstanceComponentT<TransformComponent>>& components)
	{
		const auto numInstancedComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::TransformComponentArrayData>>* instancedTransformComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::TransformComponentArrayData>, numInstancedComponents);

		for (u32 i = 0; i < numInstancedComponents; ++i)
		{
			const InstanceComponentT<TransformComponent>& instancedTransformComponent = r2::sarr::At(components, i);

			const auto numInstances = r2::sarr::Size(*instancedTransformComponent.instances);

			r2::SArray<flatbuffers::Offset<flat::TransformComponentData>>* transformComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::TransformComponentData>, numInstances);

			for (u32 j = 0; j < numInstances; ++j)
			{
				const TransformComponent& transformComponent = r2::sarr::At(*instancedTransformComponent.instances, j);

				SerializeTransform(fbb, transformComponents, transformComponent);
			}

			flat::TransformComponentArrayDataBuilder transformComponentArrayDataBuilder(fbb);

			transformComponentArrayDataBuilder.add_transformComponentArray(fbb.CreateVector(transformComponents->mData, transformComponents->mSize));

			r2::sarr::Push(*instancedTransformComponents, transformComponentArrayDataBuilder.Finish());

			FREE(transformComponents, *MEM_ENG_SCRATCH_PTR);
		}

		flat::InstancedTransformComponentArrayDataBuilder instancedTransformComponentArrayDataBuilder(fbb);
		instancedTransformComponentArrayDataBuilder.add_instancedTransformComponentArray(fbb.CreateVector(instancedTransformComponents->mData, instancedTransformComponents->mSize));
		fbb.Finish(instancedTransformComponentArrayDataBuilder.Finish());
		FREE(instancedTransformComponents, *MEM_ENG_SCRATCH_PTR);
	}

	inline void DeSerializeTransform(const flat::TransformComponentData* flatTransformComponentData, TransformComponent& transformComponent)
	{
		transformComponent.localTransform.position.x = flatTransformComponentData->position()->x();
		transformComponent.localTransform.position.y = flatTransformComponentData->position()->y();
		transformComponent.localTransform.position.z = flatTransformComponentData->position()->z();

		transformComponent.localTransform.rotation.x = flatTransformComponentData->rotation()->x();
		transformComponent.localTransform.rotation.y = flatTransformComponentData->rotation()->y();
		transformComponent.localTransform.rotation.z = flatTransformComponentData->rotation()->z();
		transformComponent.localTransform.rotation.w = flatTransformComponentData->rotation()->w();

		transformComponent.localTransform.scale.x = flatTransformComponentData->scale()->x();
		transformComponent.localTransform.scale.y = flatTransformComponentData->scale()->y();
		transformComponent.localTransform.scale.z = flatTransformComponentData->scale()->z();
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<TransformComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::TransformComponentArrayData* transformComponentArrayData = flatbuffers::GetRoot<flat::TransformComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = transformComponentArrayData->transformComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::TransformComponentData* flatTransformComponent = componentVector->Get(i);

			TransformComponent transformComponent;

			DeSerializeTransform(flatTransformComponent, transformComponent);

			r2::sarr::Push(components, transformComponent);
		}
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<InstanceComponentT<TransformComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::InstancedTransformComponentArrayData* instancedTransformComponentArrayData = flatbuffers::GetRoot<flat::InstancedTransformComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = instancedTransformComponentArrayData->instancedTransformComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::TransformComponentArrayData* flatInstancedTransformComponent = componentVector->Get(i);

			InstanceComponentT<TransformComponent> instancedTransformComponent;

			instancedTransformComponent.numInstances = flatInstancedTransformComponent->transformComponentArray()->size();

			instancedTransformComponent.instances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, TransformComponent, instancedTransformComponent.numInstances);

			for (flatbuffers::uoffset_t j = 0; j < instancedTransformComponent.numInstances; ++j)
			{
				TransformComponent nextTransformComponent;

				DeSerializeTransform(flatInstancedTransformComponent->transformComponentArray()->Get(j), nextTransformComponent);

				r2::sarr::Push(*instancedTransformComponent.instances, nextTransformComponent);
			}

			r2::sarr::Push(components, instancedTransformComponent);
		}
	}

	template<>
	inline void CleanupDeserializeComponentArray(r2::SArray<InstanceComponentT<TransformComponent>>& components)
	{
		s32 size = r2::sarr::Size(components);

		for (s32 i = size - 1; i >= 0; --i)
		{
			const InstanceComponentT<TransformComponent>& instancedTransformComponent = r2::sarr::At(components, i);
			FREE(instancedTransformComponent.instances, *MEM_ENG_SCRATCH_PTR);
		}
	}
}

#endif // __TRANSFORM_COMPONENT_SERIALIZATION
