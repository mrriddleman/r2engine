#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EntityEditorNameChangedEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

namespace r2::edit
{

	EntityEditorNameChangedEditorAction::EntityEditorNameChangedEditorAction(Editor* editor, ecs::Entity entity, const std::string& newName, const std::string& prevName)
		:EditorAction(editor)
		,mEntity(entity)
		,mNewName(newName)
		,mPreviousName(prevName)
	{

	}

	void EntityEditorNameChangedEditorAction::Undo()
	{
		ecs::EditorComponent& editorNameComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorComponent>(mEntity);

		editorNameComponent.editorName = mPreviousName;

		evt::EditorEntityNameChangedEvent e(mEntity, mPreviousName);

		mnoptrEditor->PostEditorEvent(e);
	}

	void EntityEditorNameChangedEditorAction::Redo()
	{
		ecs::EditorComponent& editorNameComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorComponent>(mEntity);

		editorNameComponent.editorName = mNewName;

		evt::EditorEntityNameChangedEvent e(mEntity, mNewName);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif