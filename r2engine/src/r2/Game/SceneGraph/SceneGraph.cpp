#include "r2pch.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Systems/SceneGraphSystem.h"
#include "r2/Game/ECS/Systems/SceneGraphTransformUpdateSystem.h"


namespace r2::ecs
{
	SceneGraph::SceneGraph()
		:mnoptrECSCoordinator(nullptr)
		,mnoptrSceneGraphSystem(nullptr) 
		,mnoptrSceneGraphTransformUpdateSystem(nullptr)
	{
	}

	SceneGraph::~SceneGraph()
	{
		R2_CHECK(mnoptrSceneGraphSystem == nullptr, "Did you forget to Shutdown the SceneGraph?");
		R2_CHECK(mnoptrECSCoordinator == nullptr, "Did you forget to Shutdown the SceneGraph?");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem == nullptr, "We haven't initialized the SceneGraph yet!");
	}

	bool SceneGraph::Init(ecs::SceneGraphSystem* sceneGraphSystem, ecs::SceneGraphTransformUpdateSystem* sceneGraphtransformUpdateSystem, ecs::ECSCoordinator* coordinator)
	{
		R2_CHECK(sceneGraphSystem != nullptr, "sceneGraphSystem is nullptr");
		R2_CHECK(sceneGraphtransformUpdateSystem != nullptr, "sceneGraphtransformUpdateSystem is nullptr");
		R2_CHECK(coordinator != nullptr, "coordinator is nullptr");

		mnoptrSceneGraphSystem = sceneGraphSystem;
		mnoptrSceneGraphTransformUpdateSystem = sceneGraphtransformUpdateSystem;
		mnoptrECSCoordinator = coordinator;

		mnoptrSceneGraphSystem->SetSceneGraph(this);

		return true;
	}

	void SceneGraph::Shutdown()
	{
		mnoptrSceneGraphSystem = nullptr;
		mnoptrSceneGraphTransformUpdateSystem = nullptr;
		mnoptrECSCoordinator = nullptr;
	}

	void SceneGraph::Update()
	{
		mnoptrSceneGraphTransformUpdateSystem->Update();
	}

	ecs::Entity SceneGraph::CreateEntity()
	{
		return CreateEntity(ecs::INVALID_ENTITY);
	}

	ecs::Entity SceneGraph::CreateEntity(ecs::Entity parent)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		ecs::Entity newEntity = mnoptrECSCoordinator->CreateEntity();

		ecs::HierarchyComponent heirarchyComponent;
		heirarchyComponent.parent = parent;

		mnoptrECSCoordinator->AddComponent<ecs::HierarchyComponent>(newEntity, heirarchyComponent);

		ecs::TransformComponent transformComponent;
		transformComponent.modelMatrix = glm::mat4(1.0f);

		mnoptrECSCoordinator->AddComponent<ecs::TransformComponent>(newEntity, transformComponent);

		ecs::TransformDirtyComponent dirty;
		dirty.dirtyFlags = ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY | ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;

		mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(newEntity, dirty);

		return newEntity;
	}

	void SceneGraph::DestroyEntity(ecs::Entity entity)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		mnoptrECSCoordinator->DestroyEntity(entity);
	}

	void SceneGraph::LoadLevel(ECSWorld& ecsWorld, const Level& level, const flat::LevelData* levelData)
	{
		mnoptrECSCoordinator->LoadAllECSDataFromLevel(ecsWorld, level, levelData);

		ecs::TransformDirtyComponent dirty;
		dirty.dirtyFlags = ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY | ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;

		mnoptrECSCoordinator->AddComponentToAllEntities<ecs::TransformDirtyComponent>(dirty);
	}

	void SceneGraph::UnloadLevel(const Level& level)
	{
		mnoptrECSCoordinator->UnloadAllECSDataFromLevel(level);
	}

	void SceneGraph::Attach(ecs::Entity entity, ecs::Entity parent, eHierarchyAttachmentType attachmentType)
	{
		R2_CHECK(entity != parent, "These shouldn't be the same?");
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		//we should check to see if we already have a heirarchy component for this entity first
		//if we don't then add one
		bool hasComponent = mnoptrECSCoordinator->HasComponent<ecs::HierarchyComponent>(entity);
		if (!hasComponent)
		{
			ecs::HierarchyComponent heirarchyComponent;
			heirarchyComponent.parent = ecs::INVALID_ENTITY; //root - ie. no parent
			mnoptrECSCoordinator->AddComponent<ecs::HierarchyComponent>(entity, heirarchyComponent);
		}
		
		ecs::HierarchyComponent& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(entity);
		heirarchyComponent.parent = parent;
		

		//We need to update the transforms of the entity
		if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(entity))
		{
			ecs::TransformDirtyComponent transformDirty;
			transformDirty.dirtyFlags = ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY | ecs::eTransformDirtyFlags::ATTACHED_TO_PARENT_DIRTY;
			transformDirty.hierarchyAttachmentType = attachmentType;

			mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(entity, transformDirty);
		}


		//essentially what we need to do here is to sort the entities in the SceneGraphSystem such that
		//parents always come before their children based on their heirarchy component
		//the actual update of the transforms will happen in the SceneGraphSystem
		//We'll have to make the transforms dirty somehow so it will update in the SceneGraphSystem

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		if (numEntities > 1)
		{
			r2::SArray<UpdatedEntity>* entitiesToUpdate = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, UpdatedEntity, ecs::MAX_NUM_ENTITIES);

			for (s32 i = static_cast<s32>(numEntities) - 1; i >= 0; --i)
			{
				ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

				const auto& heirarchy = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

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

	void SceneGraph::Detach(ecs::Entity entity, eHierarchyAttachmentType attachmentType)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		ecs::HierarchyComponent& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(entity);

		ecs::Entity oldParent = heirarchyComponent.parent;

		heirarchyComponent.parent = ecs::INVALID_ENTITY;

		//We need to update the transforms of the entity
		if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(entity))
		{
			ecs::TransformDirtyComponent transformDirty;
			transformDirty.dirtyFlags = ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;
			transformDirty.parent = oldParent;
			transformDirty.hierarchyAttachmentType = attachmentType;
			mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(entity, transformDirty);
		}

		s64 index = r2::sarr::IndexOf(*mnoptrSceneGraphSystem->mEntities, entity);

		SetDirtyFlagOnHeirarchy(entity, index);
	}

	void SceneGraph::DetachChildren(ecs::Entity parent, eHierarchyAttachmentType attachmentType)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		r2::SArray<ecs::Entity>* entitiesToUpdate = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);
			auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

			if (heirarchyComponent.parent == parent)
			{
				r2::sarr::Push(*entitiesToUpdate, e);
			}
		}

		const auto numChildren = r2::sarr::Size(*entitiesToUpdate);

		for (u32 i = 0; i < numChildren; ++i)
		{
			Detach(r2::sarr::At(*entitiesToUpdate, i), attachmentType);
		}

		FREE(entitiesToUpdate, *MEM_ENG_SCRATCH_PTR);
	}

	void SceneGraph::SetDirtyFlagOnHeirarchy(ecs::Entity entity, u32 startingIndex)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(entity))
		{
			ecs::TransformDirtyComponent c;
			c.dirtyFlags = ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;
			mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(entity, c);
		}

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (size_t i = startingIndex+1; i < numEntities; i++)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

			const auto& heirarchy = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

			if (heirarchy.parent == ecs::INVALID_ENTITY)
			{
				continue;
			}
			else if (heirarchy.parent == entity)
			{
				SetDirtyFlagOnHeirarchy(e, i);
			}
		}
	}

	void SceneGraph::GetAllChildrenForEntity(ecs::Entity parent, r2::SArray<ecs::Entity>& children)
	{
		r2::sarr::Clear(children);
		const auto numEntitiesInScene = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);
		for (u32 i = 0; i < numEntitiesInScene; i++)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

			if (heirarchyComponent.parent == parent)
			{
				r2::sarr::Push(children, e);
			}
		}
	}

	void SceneGraph::GetAllEntitiesInSubTree(ecs::Entity parent, u32 parentIndex, r2::SArray<ecs::Entity>& entities)
	{
		const auto numEntitiesInScene = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (u32 i = parentIndex+1; i < numEntitiesInScene; ++i)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);
			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

			if (heirarchyComponent.parent == parent)
			{
				r2::sarr::Push(entities, e);

				GetAllEntitiesInSubTree(e, i, entities);
			}
		}
	}

	void SceneGraph::GetAllTopLevelEntities(r2::SArray<ecs::Entity>& entities, r2::SArray<u32>& indices)
	{
		r2::sarr::Clear(entities);
		r2::sarr::Clear(indices);
		const auto numEntitiesInScene = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);
		for (u32 i = 0; i < numEntitiesInScene; i++)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(e);

			if (heirarchyComponent.parent == ecs::INVALID_ENTITY)
			{
				r2::sarr::Push(entities, e);
				r2::sarr::Push(indices, i);
			}
		}
	}

	void SceneGraph::GetAllEntitiesInScene(r2::SArray<ecs::Entity>& entities)
	{
		r2::sarr::Clear(entities);
		r2::sarr::Copy(entities, *mnoptrSceneGraphSystem->mEntities);
	}

	s32 SceneGraph::GetEntityIndex(ecs::Entity entity)
	{
		s32 numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (s32 i = 0; i < numEntities; ++i)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);

			if (e == entity)
			{
				return i;
			}
		}

		return -1;
	}

	ecs::Entity SceneGraph::GetParent(ecs::Entity entity)
	{
		const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HierarchyComponent>(entity);
		return heirarchyComponent.parent;
	}

	r2::ecs::ECSCoordinator* SceneGraph::GetECSCoordinator() const
	{
		return mnoptrECSCoordinator;
	}

	void SceneGraph::UpdateTransformForEntity(ecs::Entity entity, ecs::eTransformDirtyFlags dirtyFlags)
	{
		if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(entity))
		{
			mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(entity, { dirtyFlags });
		}

		//@TODO(Serge): might be useful for the scene graph to have this memory pre-allocated at all times - dunno
		r2::SArray<ecs::Entity>* children = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		GetAllChildrenForEntity(entity, *children);

		const auto numChildren = r2::sarr::Size(*children);

		for (u32 i = 0; i < numChildren; ++i)
		{
			ecs::Entity child = r2::sarr::At(*children, i);

			if (!mnoptrECSCoordinator->HasComponent<ecs::TransformDirtyComponent>(child))
			{
				mnoptrECSCoordinator->AddComponent<ecs::TransformDirtyComponent>(child, { ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY });
			}
		}

		FREE( children, *MEM_ENG_SCRATCH_PTR);
	}

}