#ifndef __POINT_LIGHT_COMPONENT_SERIALIZATION_H__
#define __POINT_LIGHT_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Serialization/PointLightComponentArrayData_generated.h"
#include "r2/Game/ECS/Components/PointLightComponent.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/Renderer.h"


namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<PointLightComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::PointLightData>>* pointLightComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::PointLightData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const PointLightComponent& pointLightComponent = r2::sarr::At(components, i);
	
			flat::LightColor lightColor(pointLightComponent.lightProperties.color.r, pointLightComponent.lightProperties.color.g, pointLightComponent.lightProperties.color.b, pointLightComponent.lightProperties.color.a);
			flat::ShadowInfo shadowInfo(pointLightComponent.lightProperties.castsShadowsUseSoftShadows.x, pointLightComponent.lightProperties.castsShadowsUseSoftShadows.y);

			auto flatLightProperties = flat::CreateLightProperties(fbb, &lightColor, &shadowInfo, pointLightComponent.lightProperties.fallOff, pointLightComponent.lightProperties.intensity);

			flat::PointLightDataBuilder pointlightComponentBuilder(fbb);
			pointlightComponentBuilder.add_lightProperties(flatLightProperties);
			
			r2::sarr::Push(*pointLightComponents, pointlightComponentBuilder.Finish());
		}

		auto vec = fbb.CreateVector(pointLightComponents->mData, pointLightComponents->mSize);

		flat::PointLightComponentArrayDataBuilder pointLightComponentArrayDataBuilder(fbb);

		pointLightComponentArrayDataBuilder.add_pointLightComponentArray(vec);

		fbb.Finish(pointLightComponentArrayDataBuilder.Finish());

		FREE(pointLightComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<PointLightComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::PointLightComponentArrayData* pointLightComponentArrayData = flatbuffers::GetRoot<flat::PointLightComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = pointLightComponentArrayData->pointLightComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::PointLightData* flatPointLightData = componentVector->Get(i);

			PointLightComponent pointLightComponent;

			const auto* flatLightColor = flatPointLightData->lightProperties()->lightColor();
			const auto* flatShadowInfo = flatPointLightData->lightProperties()->shadowInfo();

			pointLightComponent.lightProperties.color = glm::vec4(flatLightColor->r(), flatLightColor->g(), flatLightColor->b(), flatLightColor->a());
			pointLightComponent.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(flatShadowInfo->castsShadows(), flatShadowInfo->useSoftShadows(), 0, 0);
			pointLightComponent.lightProperties.fallOff = flatPointLightData->lightProperties()->fallOff();
			pointLightComponent.lightProperties.intensity = flatPointLightData->lightProperties()->intensity();

			r2::draw::PointLight newPointLight;
			newPointLight.lightProperties = pointLightComponent.lightProperties;
			newPointLight.position = glm::vec4(0);

			pointLightComponent.pointLightHandle = r2::draw::renderer::AddPointLight(newPointLight);

			//@NOTE(Serge): we're making sure to set the light handle of the light properties
			pointLightComponent.lightProperties = r2::draw::renderer::GetPointLightConstPtr(pointLightComponent.pointLightHandle)->lightProperties;


			r2::sarr::Push(components, pointLightComponent);
		}
	}
}

#endif // __POINT_LIGHT_COMPONENT_SERIALIZATION_H__

