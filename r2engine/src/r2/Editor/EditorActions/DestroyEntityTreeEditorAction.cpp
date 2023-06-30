#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/DestroyEntityTreeEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Game/ECS/ECSCoordinator.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::edit
{

	DestroyEntityTreeEditorAction::DestroyEntityTreeEditorAction(Editor* editor, ecs::Entity entityToDestroy, ecs::Entity parentOfEntityToDestroy)
		:EditorAction(editor)
		,mEntityTree{}
	{
		r2::SArray<ecs::Entity>* children = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		auto index = mnoptrEditor->GetSceneGraph().GetEntityIndex(entityToDestroy);

		R2_CHECK(index != -1, "Couldn't find the index of entity: %lu", entityToDestroy);

		mnoptrEditor->GetSceneGraph().GetAllEntitiesInSubTree(entityToDestroy, index, *children);

		const auto numChildren = r2::sarr::Size(*children);
		mEntityTree.reserve(numChildren+1);
		mEntityTreeEditorNameComponents.reserve(numChildren+1);
		mEntityTreeHeirarchyComponents.reserve(numChildren+1);
		mEntityTreeTransformComponents.reserve(numChildren+1);

		mEntityTree.push_back(entityToDestroy);
		mEntityTreeEditorNameComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::EditorComponent>(entityToDestroy));
		mEntityTreeHeirarchyComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::HeirarchyComponent>(entityToDestroy));
		mEntityTreeTransformComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::TransformComponent>(entityToDestroy));

		for (size_t i = 0; i < numChildren; ++i)
		{
			ecs::Entity e = r2::sarr::At(*children, i);
			
			mEntityTree.push_back(e);

			mEntityTreeEditorNameComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::EditorComponent>(e));
			mEntityTreeHeirarchyComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::HeirarchyComponent>(e));
			mEntityTreeTransformComponents.push_back(mnoptrEditor->GetSceneGraph().GetECSCoordinator()->GetComponent<ecs::TransformComponent>(e));
		}

		FREE(children, *MEM_ENG_SCRATCH_PTR);
	}

	void DestroyEntityTreeEditorAction::Undo()
	{
		size_t numEntities = mEntityTree.size();
		for (size_t i = 0; i < numEntities; ++i)
		{
			mEntityTree[i] = mnoptrEditor->GetSceneGraph().CreateEntity(mEntityTreeHeirarchyComponents[i].parent);

			mnoptrEditor->GetEditorLevel().AddEntity(mEntityTree[i]);

			mnoptrEditor->GetECSCoordinator()->AddComponent<ecs::EditorComponent>(mEntityTree[i], mEntityTreeEditorNameComponents[i]);
			mnoptrEditor->GetECSCoordinator()->SetComponent<ecs::HeirarchyComponent>(mEntityTree[i], mEntityTreeHeirarchyComponents[i]);
			mnoptrEditor->GetECSCoordinator()->SetComponent<ecs::TransformComponent>(mEntityTree[i], mEntityTreeTransformComponents[i]);
		}

		evt::EditorEntityTreeDestroyedEvent e(mEntityTree[0]);
		mnoptrEditor->PostEditorEvent(e);
	}

	void DestroyEntityTreeEditorAction::Redo()
	{
		for (auto iter = mEntityTree.rbegin(); iter != mEntityTree.rend(); ++iter)
		{
			mnoptrEditor->GetEditorLevel().RemoveEntity(*iter);
			mnoptrEditor->GetSceneGraph().DestroyEntity(*iter);
		}

		evt::EditorEntityTreeDestroyedEvent e(mEntityTree[0]);
		
		mnoptrEditor->PostEditorEvent(e);
	}

}

#endif