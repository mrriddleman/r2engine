#include "r2pch.h"

#include "r2/Game/ECS/Systems/SceneGraphTransformUpdateSystem.h"

#include "r2/Core/Math/Transform.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::ecs
{
	SceneGraphTransformUpdateSystem::SceneGraphTransformUpdateSystem()
	{
		mKeepSorted = true;
	}

	SceneGraphTransformUpdateSystem::~SceneGraphTransformUpdateSystem()
	{

	}

	void SceneGraphTransformUpdateSystem::Update()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		//Don't do anything if we have no entities since math::Combine and math::ToMatrix is fairly expensive
		const auto numEntitiesToUpdate = r2::sarr::Size(*mEntities);
		if (numEntitiesToUpdate == 0)
			return;

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		for (u32 i = 0; i < numEntitiesToUpdate; ++i)
		{
			//do the update of the matrices here
			ecs::Entity entity = r2::sarr::At(*mEntities, i);

			//get the heirarch component and the transform component of the entity
			const HierarchyComponent& entityHeirarchComponent = mnoptrCoordinator->GetComponent<HierarchyComponent>(entity);
			TransformComponent& entityTransformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(entity);

			math::Transform parentTransform;
			//now get the transform component of the parent if it exists
			if (entityHeirarchComponent.parent != INVALID_ENTITY)
			{
				TransformComponent& parentTransformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(entity);
				parentTransform = parentTransformComponent.accumTransform;
			}

			//@TODO(Serge): Need to figure out a way to optimize this - very slow at the moment
			entityTransformComponent.accumTransform = math::Combine(parentTransform, entityTransformComponent.localTransform);

			glm::mat4 worldTransform = math::ToMatrix(entityTransformComponent.accumTransform);

			RenderComponent* renderComponent = mnoptrCoordinator->GetComponentPtr<RenderComponent>(entity);
			const r2::draw::Model* model = nullptr;
			if (renderComponent)
			{
				r2::asset::Asset modelAsset = r2::asset::Asset(renderComponent->assetModelName, r2::asset::RMODEL);
				
				//@NOTE(Serge): hopefully this never actually loads anything
				r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);

				model = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);
				//@PERFORMANCE(Serge): this might be too slow when it comes to moving characters around
				worldTransform = worldTransform * model->globalTransform;
			}

			entityTransformComponent.modelMatrix = worldTransform;

			InstanceComponentT<TransformComponent>* instanceComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<TransformComponent>>(entity);
			if (instanceComponent)
			{
				for (u32 j = 0; j < instanceComponent->numInstances; j++)
				{
					TransformComponent& tranformComponent = r2::sarr::At(*instanceComponent->instances, j);
					tranformComponent.accumTransform = math::Combine(parentTransform, tranformComponent.localTransform);
					glm::mat4 worldTransform = math::ToMatrix(tranformComponent.accumTransform);
					if (renderComponent)
					{
						worldTransform = worldTransform * model->globalTransform;
					}
					tranformComponent.modelMatrix = worldTransform;
				}
			}
		}

		//We need to make a copy here because we're changing the signature of the entities such that they won't belong 
		//to our system anymore
		r2::SArray<Entity>* entitiesCopy = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, Entity, numEntitiesToUpdate);

		r2::sarr::Copy(*entitiesCopy, *mEntities);

		for (u32 i = 0; i < numEntitiesToUpdate; ++i)
		{
			//remove the dirty flags
			ecs::Entity e = r2::sarr::At(*entitiesCopy, i);
			mnoptrCoordinator->RemoveComponent<ecs::TransformDirtyComponent>(e);
		}

		FREE(entitiesCopy, *MEM_ENG_SCRATCH_PTR);
	}
}