#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/DestroyEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorNameComponent.h"

namespace r2::edit
{
	DestroyEntityEditorAction::DestroyEntityEditorAction(Editor* editor, ecs::Entity entityToDestroy, ecs::Entity parentOfEntityToDestroy)
		:mnoptrEditor(editor)
		,mEntityToDestroy(entityToDestroy)
		,mParentOfEntityToDestory(parentOfEntityToDestroy)
	{

	}

	void DestroyEntityEditorAction::Undo()
	{
		mEntityToDestroy = mnoptrEditor->GetSceneGraph().CreateEntity(mParentOfEntityToDestory);

		ecs::ECSCoordinator* coordinator = mnoptrEditor->GetSceneGraph().GetECSCoordinator();

		ecs::EditorNameComponent nameComponent;

		nameComponent.editorName = std::string("Entity - ") + std::to_string(mEntityToDestroy);

		coordinator->AddComponent<ecs::EditorNameComponent>(mEntityToDestroy, nameComponent);

		r2::evt::EditorEntityCreatedEvent e(mEntityToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}

	void DestroyEntityEditorAction::Redo()
	{
		mnoptrEditor->GetSceneGraph().DestroyEntity(mEntityToDestroy);

		r2::evt::EditorEntityDestroyedEvent e(mEntityToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif