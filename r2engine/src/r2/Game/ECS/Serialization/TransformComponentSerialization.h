#ifndef __TRANSFORM_COMPONENT_SERIALIZATION_H__
#define __TRANSFORM_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"

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

	inline void SerializeTransform(flexbuffers::Builder& builder, const TransformComponent& transform)
	{
		builder.Vector([&]() {
			builder.IndirectFloat(transform.localTransform.position.x);
			builder.IndirectFloat(transform.localTransform.position.y);
			builder.IndirectFloat(transform.localTransform.position.z);
			builder.IndirectFloat(transform.localTransform.scale.x);
			builder.IndirectFloat(transform.localTransform.scale.y);
			builder.IndirectFloat(transform.localTransform.scale.z);
			builder.IndirectFloat(transform.localTransform.rotation.x);
			builder.IndirectFloat(transform.localTransform.rotation.y);
			builder.IndirectFloat(transform.localTransform.rotation.z);
			builder.IndirectFloat(transform.localTransform.rotation.w);
			});
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<TransformComponent>& components)
	{
		builder.Vector([&]() {

			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const auto& transform = r2::sarr::At(components, i);
		
				SerializeTransform(builder, transform);
			}
		});
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<InstanceComponentT<TransformComponent>>& components)
	{
		builder.Vector([&]() {
			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const InstanceComponentT<TransformComponent>& instancedTransform = r2::sarr::At(components, i);

				builder.Vector([&]() {
					for (u32 j = 0; j < instancedTransform.numInstances; ++j)
					{
						const auto& transform = r2::sarr::At(*instancedTransform.instances, j);

						SerializeTransform(builder, transform);
					}
				});
			}
		});
	}

	inline void DeSerializeTransform(TransformComponent& transformComponent, flexbuffers::Reference flexTransformRef)
	{
		auto flexTransformVector = flexTransformRef.AsVector();

		transformComponent.localTransform.position.x = flexTransformVector[0].AsFloat();
		transformComponent.localTransform.position.y = flexTransformVector[1].AsFloat();
		transformComponent.localTransform.position.z = flexTransformVector[2].AsFloat();
		transformComponent.localTransform.scale.x = flexTransformVector[3].AsFloat();
		transformComponent.localTransform.scale.y = flexTransformVector[4].AsFloat();
		transformComponent.localTransform.scale.z = flexTransformVector[5].AsFloat();
		transformComponent.localTransform.rotation.x = flexTransformVector[6].AsFloat();
		transformComponent.localTransform.rotation.y = flexTransformVector[7].AsFloat();
		transformComponent.localTransform.rotation.z = flexTransformVector[8].AsFloat();
		transformComponent.localTransform.rotation.w = flexTransformVector[9].AsFloat();
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<TransformComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			TransformComponent transformComponent;
			auto temp = componentVector[i];
			DeSerializeTransform(transformComponent, temp);

			r2::sarr::Push(components, transformComponent);
		}
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<InstanceComponentT<TransformComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			InstanceComponentT<TransformComponent> instancedTransformComponent;
			
			auto flexInstancedTransformComponent = componentVector[i].AsVector();

			size_t numInstances = flexInstancedTransformComponent.size();

			instancedTransformComponent.numInstances = numInstances;

			//this is a problem right here - how do we free this?
			instancedTransformComponent.instances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, TransformComponent, numInstances);

			for (size_t j = 0; j < flexInstancedTransformComponent.size(); ++j)
			{
				TransformComponent nextTransformComponent;
				DeSerializeTransform(nextTransformComponent, flexInstancedTransformComponent[j]);

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
