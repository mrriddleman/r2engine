#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EntityEditorFlagChangedEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::edit
{
	EntityEditorFlagChangedEditorAction::EntityEditorFlagChangedEditorAction(Editor* editor, ecs::Entity e, u32 flag, bool enabled)
		:EditorAction(editor)
		,mEntity(e)
		,mFlag(flag)
		,mEnabled(enabled)
	{
	}

	void EntityEditorFlagChangedEditorAction::Undo()
	{
		ecs::EditorComponent& editorComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorComponent>(mEntity);

		bool enabled = !mEnabled;

		if (enabled)
		{
			editorComponent.flags.Set(mFlag);
		}
		else
		{
			editorComponent.flags.Remove(mFlag);
		}

		evt::EditorEntityEditorFlagChangedEvent e(mEntity, mFlag, enabled);
		mnoptrEditor->PostEditorEvent(e);
	}

	void EntityEditorFlagChangedEditorAction::Redo()
	{
		ecs::EditorComponent& editorComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorComponent>(mEntity);

		if (mEnabled)
		{
			editorComponent.flags.Set(mFlag);
		}
		else
		{
			editorComponent.flags.Remove(mFlag);
		}

		evt::EditorEntityEditorFlagChangedEvent e(mEntity, mFlag, mEnabled);
		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif