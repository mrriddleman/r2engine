#ifndef __TRANSFORM_COMPONENT_SERIALIZATION_H__
#define __TRANSFORM_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/TransformComponent.h"

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

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<TransformComponent>& components)
	{
		builder.Vector("transforms", [&]() {

			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const auto& transform = r2::sarr::At(components, i);
		
				builder.Vector("localTransform", [&]() {
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
		});
	}
}

#endif // __TRANSFORM_COMPONENT_SERIALIZATION
