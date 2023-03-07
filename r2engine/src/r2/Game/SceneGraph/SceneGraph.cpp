#include "r2pch.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2
{
	SceneGraph::SceneGraph()
		:mnoptrSceneGraphSystem(nullptr)
		, mnoptrECSCoordinator(nullptr)
	{

	}

	SceneGraph::~SceneGraph()
	{
		R2_CHECK(mnoptrSceneGraphSystem == nullptr, "Did you forget to Shutdown the SceneGraph?");
		R2_CHECK(mnoptrECSCoordinator == nullptr, "Did you forget to Shutdown the SceneGraph?");
	}



	void SceneGraph::Attach(ecs::Entity entity, ecs::Entity parent)
	{
		R2_CHECK(entity != parent, "These shouldn't be the same?");
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		//we should check to see if we already have a heirarchy component for this entity first
		//if we don't then add one
		bool hasComponent = mnoptrECSCoordinator->HasComponent<ecs::HeirarchyComponent>(entity);
		if (!hasComponent)
		{
			ecs::HeirarchyComponent heirarchyComponent;
			heirarchyComponent.parent = ecs::INVALID_ENTITY; //root - ie. no parent
			mnoptrECSCoordinator->AddComponent<ecs::HeirarchyComponent>(entity, heirarchyComponent);
		}
		
		ecs::HeirarchyComponent& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(entity);
		heirarchyComponent.parent = parent;

		//essentially what we need to do here is to sort the entities in the SceneGraphSystem such that
		//parents always come before their children based on their heirarchy component
		//the actual update of the transforms will happen in the SceneGraphSystem
		//We'll have to make the transforms dirty somehow so it will update in the SceneGraphSystem

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		if (numEntities > 1)
		{
			r2::SArray<UpdatedEntity>* entitiesToUpdate = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, UpdatedEntity, ecs::MAX_NUM_ENTITIES);

			for (size_t i = numEntities - 1; i >= 0; --i)
			{
				ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

				const auto& heirarchy = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

				if (heirarchy.parent == ecs::INVALID_ENTITY)
				{
					continue;
				}

				s64 parentIndex = r2::sarr::IndexOf(*mnoptrSceneGraphSystem->mEntities, heirarchy.parent);

				R2_CHECK(parentIndex != -1, "Not sure how this would happen?");

				if (parentIndex > i)
				{
					mnoptrECSCoordinator->MoveEntity<ecs::SceneGraphSystem>(i, parentIndex);
					i++;

					UpdatedEntity updateEntity;
					updateEntity.e = e;
					updateEntity.index = parentIndex;


					
					r2::sarr::Push(*entitiesToUpdate, updateEntity);
				}
			}


			const auto numEntitiesToUpdate = r2::sarr::Size(*entitiesToUpdate);

			for (size_t i = 0; i < numEntitiesToUpdate; i++)
			{
				const UpdatedEntity& updateEntity = r2::sarr::At(*entitiesToUpdate, i);
				ecs::Entity e = updateEntity.e;
				s32 index = updateEntity.index;
				SetDirtyFlagOnHeirarchy(e, index);
			}


			FREE(entitiesToUpdate, *MEM_ENG_SCRATCH_PTR);
		}
	}

	void SceneGraph::Detach(ecs::Entity entity)
	{

	}

	void SceneGraph::DetachChildren(ecs::Entity parent)
	{

	}

	void SceneGraph::SetDirtyFlagOnHeirarchy(ecs::Entity entity, u32 startingIndex)
	{
		if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(entity))
		{
			ecs::TransformDirtyComponent c;
			mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(entity, c);
		}

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (size_t i = startingIndex+1; i < numEntities; i++)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

			const auto& heirarchy = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

			if (heirarchy.parent == ecs::INVALID_ENTITY)
			{
				continue;
			}
			else if (heirarchy.parent == entity)
			{
				UpdatedEntity updateEntity;
				updateEntity.e = e;
				updateEntity.index = i;

				SetDirtyFlagOnHeirarchy(e, i);
			}
		}
	}

}