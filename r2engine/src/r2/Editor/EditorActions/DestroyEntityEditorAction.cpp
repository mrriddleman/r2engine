#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/DestroyEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorNameComponent.h"

namespace r2::edit
{
	DestroyEntityEditorAction::DestroyEntityEditorAction(Editor* editor, ecs::Entity entityToDestroy)
		:mnoptrEditor(editor)
		,mEditorToDestroy(entityToDestroy)
	{

	}

	void DestroyEntityEditorAction::Undo()
	{
		ecs::Entity parent = mnoptrEditor->GetSceneGraph().GetParent(mEditorToDestroy);

		mEditorToDestroy = mnoptrEditor->GetSceneGraph().CreateEntity(parent);

		ecs::ECSCoordinator* coordinator = mnoptrEditor->GetSceneGraph().GetECSCoordinator();

		ecs::EditorNameComponent nameComponent;

		nameComponent.editorName = std::string("Entity - ") + std::to_string(mEditorToDestroy);

		coordinator->AddComponent<ecs::EditorNameComponent>(mEditorToDestroy, nameComponent);

		r2::evt::EditorEntityCreatedEvent e(mEditorToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}

	void DestroyEntityEditorAction::Redo()
	{
		mnoptrEditor->GetSceneGraph().DestroyEntity(mEditorToDestroy);

		r2::evt::EditorEntityDestroyedEvent e(mEditorToDestroy);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif