#include "r2pch.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2
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

		ecs::HeirarchyComponent heirarchyComponent;
		heirarchyComponent.parent = parent;

		mnoptrECSCoordinator->AddComponent<ecs::HeirarchyComponent>(newEntity, heirarchyComponent);

		ecs::TransformComponent transformComponent;
		transformComponent.modelMatrix = glm::mat4(1.0f);

		mnoptrECSCoordinator->AddComponent<ecs::TransformComponent>(newEntity, transformComponent);

		ecs::TransformDirtyComponent dirty;
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

	void SceneGraph::LoadLevel(const Level& level)
	{
		mnoptrECSCoordinator->LoadAllECSDataFromLevel(level);

		ecs::TransformDirtyComponent dirty;

		mnoptrECSCoordinator->AddComponentToAllEntities<ecs::TransformDirtyComponent>(dirty);
	}

	void SceneGraph::UnloadLevel(const Level& level)
	{
		TODO;
	}

	void SceneGraph::Attach(ecs::Entity entity, ecs::Entity parent)
	{
		R2_CHECK(entity != parent, "These shouldn't be the same?");
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

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

			for (s32 i = static_cast<s32>(numEntities) - 1; i >= 0; --i)
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
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		ecs::HeirarchyComponent& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(entity);
		heirarchyComponent.parent = ecs::INVALID_ENTITY;

		s64 index = r2::sarr::IndexOf(*mnoptrSceneGraphSystem->mEntities, entity);

		SetDirtyFlagOnHeirarchy(entity, index);
	}

	void SceneGraph::DetachChildren(ecs::Entity parent)
	{
		R2_CHECK(mnoptrECSCoordinator != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphSystem != nullptr, "We haven't initialized the SceneGraph yet!");
		R2_CHECK(mnoptrSceneGraphTransformUpdateSystem != nullptr, "We haven't initialized the SceneGraph yet!");

		r2::SArray<ecs::Entity>* entitiesToUpdate = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		const auto numEntities = r2::sarr::Size(*mnoptrSceneGraphSystem->mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			ecs::Entity e = r2::sarr::At(*mnoptrSceneGraphSystem->mEntities, i);
			auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

			if (heirarchyComponent.parent == parent)
			{
				r2::sarr::Push(*entitiesToUpdate, e);
			}
		}

		const auto numChildren = r2::sarr::Size(*entitiesToUpdate);

		for (u32 i = 0; i < numChildren; ++i)
		{
			Detach(r2::sarr::At(*entitiesToUpdate, i));
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

			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

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
			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

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

			const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

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
		const auto& heirarchyComponent = mnoptrECSCoordinator->GetComponent<ecs::HeirarchyComponent>(entity);
		return heirarchyComponent.parent;
	}

	r2::ecs::ECSCoordinator* SceneGraph::GetECSCoordinator() const
	{
		return mnoptrECSCoordinator;
	}

	u64 SceneGraph::MemorySize(const r2::mem::utils::MemoryProperties& memProperties)
	{
		u64 memorySize = 0;

		memorySize += ecs::ECSCoordinator::MemorySizeOfSystemType<ecs::SceneGraphSystem>(memProperties);
		memorySize += ecs::ECSCoordinator::MemorySizeOfSystemType<ecs::SceneGraphTransformUpdateSystem>(memProperties);

		return memorySize;
	}
}