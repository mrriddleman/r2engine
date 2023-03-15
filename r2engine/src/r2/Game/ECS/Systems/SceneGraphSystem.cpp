#include "r2pch.h"

#include "r2/Game/ECS/Systems/SceneGraphSystem.h"
#include "r2/Game/ECS/Components/HeirarchyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::ecs
{

	SceneGraphSystem::SceneGraphSystem()
		:mnoptrSceneGraph(nullptr)
	{
		mKeepSorted = true;
	}

	SceneGraphSystem::~SceneGraphSystem()
	{

	}

	s32 SceneGraphSystem::FindSortedPlacement(Entity e)
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Should have a coordinator set correctly");
		R2_CHECK(mnoptrSceneGraph != nullptr, "Should have a scene graph set first!");
		const ecs::HeirarchyComponent& heirarchyComponent = mnoptrCoordinator->GetComponent<ecs::HeirarchyComponent>(e);

		ecs::Entity parent = heirarchyComponent.parent;

		r2::SArray<ecs::Entity>* entities = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);
		s32 indexToReturn = -1;

		if (parent == ecs::INVALID_ENTITY)
		{
			r2::SArray<u32>* indices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, ecs::MAX_NUM_ENTITIES);

			mnoptrSceneGraph->GetAllTopLevelEntities(*entities, *indices);

			const auto numEntities = r2::sarr::Size(*entities);

			for (u32 i = 0; i < numEntities; ++i)
			{
				ecs::Entity sibling = r2::sarr::At(*entities, i);

				if (e < sibling)
				{
					indexToReturn = r2::sarr::At(*indices, i);
					break;
				}
			}

			FREE(indices, *MEM_ENG_SCRATCH_PTR);
		}
		else
		{
			mnoptrSceneGraph->GetAllChildrenForEntity(parent, *entities);

			const auto numEntities = r2::sarr::Size(*entities);

			for (u32 i = 0; i < numEntities; ++i)
			{
				ecs::Entity sibling = r2::sarr::At(*entities, i);
				if (e < sibling)
				{
					indexToReturn = r2::sarr::IndexOf(*mEntities, sibling);
					break;
				}
			}
		}

		FREE(entities, *MEM_ENG_SCRATCH_PTR);

		return indexToReturn;
	}

	void SceneGraphSystem::SetSceneGraph(SceneGraph* sceneGraph)
	{
		mnoptrSceneGraph = sceneGraph;
	}

}