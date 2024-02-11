#ifndef __SPOT_LIGHT_COMPONENT_SERIALIZATION_H__
#define __SPOT_LIGHT_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Serialization/SpotLightComponentArrayData_generated.h"
#include "r2/Game/ECS/Components/SpotLightComponent.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/Renderer.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<SpotLightComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::SpotLightData>>* spotLightComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::SpotLightData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const SpotLightComponent& spotLightComponent = r2::sarr::At(components, i);

			flat::LightColor lightColor(spotLightComponent.lightProperties.color.r, spotLightComponent.lightProperties.color.g, spotLightComponent.lightProperties.color.b, spotLightComponent.lightProperties.color.a);
			flat::ShadowInfo shadowInfo(spotLightComponent.lightProperties.castsShadowsUseSoftShadows.x, spotLightComponent.lightProperties.castsShadowsUseSoftShadows.y);

			auto flatLightProperties = flat::CreateLightProperties(fbb, &lightColor, &shadowInfo, spotLightComponent.lightProperties.fallOff, spotLightComponent.lightProperties.intensity);

			flat::SpotLightDataBuilder spotlightComponentBuilder(fbb);
			spotlightComponentBuilder.add_lightProperties(flatLightProperties);
			spotlightComponentBuilder.add_innerCutoffAngle(spotLightComponent.innerCutoffAngle);
			spotlightComponentBuilder.add_outerCutoffAngle(spotLightComponent.outerCutoffAngle);

			r2::sarr::Push(*spotLightComponents, spotlightComponentBuilder.Finish());
		}

		auto vec = fbb.CreateVector(spotLightComponents->mData, spotLightComponents->mSize);

		flat::SpotLightComponentArrayDataBuilder spotLightComponentArrayDataBuilder(fbb);

		spotLightComponentArrayDataBuilder.add_spotLightComponentArray(vec);

		fbb.Finish(spotLightComponentArrayDataBuilder.Finish());

		FREE(spotLightComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<SpotLightComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::SpotLightComponentArrayData* spotLightComponentArrayData = flatbuffers::GetRoot<flat::SpotLightComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = spotLightComponentArrayData->spotLightComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::SpotLightData* flatSpotLightData = componentVector->Get(i);

			SpotLightComponent spotLightComponent;

			const auto* flatLightColor = flatSpotLightData->lightProperties()->lightColor();
			const auto* flatShadowInfo = flatSpotLightData->lightProperties()->shadowInfo();

			spotLightComponent.lightProperties.color = glm::vec4(flatLightColor->r(), flatLightColor->g(), flatLightColor->b(), flatLightColor->a());
			spotLightComponent.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(flatShadowInfo->castsShadows(), flatShadowInfo->useSoftShadows(), 0, 0);
			spotLightComponent.lightProperties.fallOff = flatSpotLightData->lightProperties()->fallOff();
			spotLightComponent.lightProperties.intensity = flatSpotLightData->lightProperties()->intensity();
			spotLightComponent.outerCutoffAngle = flatSpotLightData->outerCutoffAngle();
			spotLightComponent.innerCutoffAngle = flatSpotLightData->innerCutoffAngle();

			r2::draw::SpotLight newSpotLight;
			newSpotLight.lightProperties = spotLightComponent.lightProperties;
			newSpotLight.position = glm::vec4(0, 0, 0, spotLightComponent.innerCutoffAngle);
			newSpotLight.direction = glm::vec4(0, 0, -1, spotLightComponent.outerCutoffAngle);
			spotLightComponent.spotLightHandle = r2::draw::renderer::AddSpotLight(newSpotLight);

			r2::sarr::Push(components, spotLightComponent);
		}
	}
}

#endif