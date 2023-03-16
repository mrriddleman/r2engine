#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/DestroyEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::edit
{
	DestroyEntityEditorAction::DestroyEntityEditorAction(Editor* editor, ecs::Entity entityToDestroy, ecs::Entity parentOfEntityToDestroy)
		:EditorAction(editor)
		,mEntityToDestroy(entityToDestroy)
		,mParentOfEntityToDestory(parentOfEntityToDestroy)
	{
		r2::SArray<ecs::Entity>* children = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		mnoptrEditor->GetSceneGraph().GetAllChildrenForEntity(mEntityToDestroy, *children);

		const auto numChildren = r2::sarr::Size(*children);

		for (size_t i = 0; i < numChildren; ++i)
		{
			mChildren.push_back(r2::sarr::At(*children, i));
		}

		FREE(children, *MEM_ENG_SCRATCH_PTR);

		mEntityEditorNameComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorComponent>(mEntityToDestroy);
		mEntityHeirarchyComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::HeirarchyComponent>(mEntityToDestroy);
		mEntityTransformComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::TransformComponent>(mEntityToDestroy);
	}

	void DestroyEntityEditorAction::Undo()
	{
		mEntityToDestroy = mnoptrEditor->GetSceneGraph().CreateEntity(mParentOfEntityToDestory);

		ecs::ECSCoordinator* coordinator = mnoptrEditor->GetSceneGraph().GetECSCoordinator();

		coordinator->AddComponent<ecs::EditorComponent>(mEntityToDestroy, mEntityEditorNameComponent);
		coordinator->SetComponent<ecs::HeirarchyComponent>(mEntityToDestroy, mEntityHeirarchyComponent);
		coordinator->SetComponent<ecs::TransformComponent>(mEntityToDestroy, mEntityTransformComponent);

		for (ecs::Entity e : mChildren)
		{
			mnoptrEditor->GetSceneGraph().Attach(e, mEntityToDestroy);
		}

		r2::evt::EditorEntityCreatedEvent e(mEntityToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}

	void DestroyEntityEditorAction::Redo()
	{
		const auto numChildren = mChildren.size();

		for (size_t i = 0; i < numChildren; ++i)
		{
			mnoptrEditor->GetSceneGraph().Attach(mChildren[i], mParentOfEntityToDestory);
		}

		mnoptrEditor->GetSceneGraph().DestroyEntity(mEntityToDestroy);

		r2::evt::EditorEntityDestroyedEvent e(mEntityToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif