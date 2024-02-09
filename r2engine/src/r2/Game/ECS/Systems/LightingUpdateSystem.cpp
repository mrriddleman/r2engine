#include "r2pch.h"

#include "r2/Game/ECS/Systems/LightingUpdateSystem.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

#include "r2/Render/Renderer/Renderer.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/PointLightComponent.h"
#include "r2/Game/ECS/Components/LightUpdateComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::ecs
{

	LightingUpdateSystem::LightingUpdateSystem()
	{
		mKeepSorted = false;
	}

	LightingUpdateSystem::~LightingUpdateSystem()
	{

	}

	void LightingUpdateSystem::Update()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		//Don't do anything if we have no entities since math::Combine and math::ToMatrix is fairly expensive
		const auto numEntitiesToUpdate = r2::sarr::Size(*mEntities);
		if (numEntitiesToUpdate == 0)
			return;

		for (u32 i = 0; i < numEntitiesToUpdate; ++i)
		{
			//do the update of the matrices here
			ecs::Entity entity = r2::sarr::At(*mEntities, i);

			const TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(entity);
			const LightUpdateComponent& lightUpdateComponent = mnoptrCoordinator->GetComponent <LightUpdateComponent >(entity);

			if (lightUpdateComponent.flags.IsSet(POINT_LIGHT_UPDATE))
			{
				const PointLightComponent& pointLightComponent = mnoptrCoordinator->GetComponent<PointLightComponent>(entity);

				auto* pointLight = r2::draw::renderer::GetPointLightPtr(pointLightComponent.pointLightHandle);

				pointLight->lightProperties = pointLightComponent.lightProperties;
				pointLight->position = glm::vec4(transformComponent.accumTransform.position, pointLight->position.w);
			}

			//@TODO(Serge): the rest of the lights

		}

		//We need to make a copy here because we're changing the signature of the entities such that they won't belong 
		//to our system anymore
		r2::SArray<Entity>* entitiesCopy = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, Entity, numEntitiesToUpdate);

		r2::sarr::Copy(*entitiesCopy, *mEntities);

		for (u32 i = 0; i < numEntitiesToUpdate; ++i)
		{
			//remove the dirty flags
			ecs::Entity e = r2::sarr::At(*entitiesCopy, i);
			mnoptrCoordinator->RemoveComponent<ecs::LightUpdateComponent>(e);
		}

		FREE(entitiesCopy, *MEM_ENG_SCRATCH_PTR);
	}

}