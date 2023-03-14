#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"

namespace r2::edit
{
	SelectedEntityEditorAction::SelectedEntityEditorAction(Editor* editor, ecs::Entity entitySelected, ecs::Entity prevEntitySelected)
		:mnoptrEditor(editor)
		,mSelectedEntity(entitySelected)
		,mPrevSelectedEntity(prevEntitySelected)
	{
	}

	void SelectedEntityEditorAction::Undo()
	{
		evt::EditorEntitySelectedEvent e(mPrevSelectedEntity, mSelectedEntity);
		mnoptrEditor->PostEditorEvent(e);
	}

	void SelectedEntityEditorAction::Redo()
	{
		evt::EditorEntitySelectedEvent e(mSelectedEntity, mPrevSelectedEntity);
		mnoptrEditor->PostEditorEvent(e);
	}
}


#endif