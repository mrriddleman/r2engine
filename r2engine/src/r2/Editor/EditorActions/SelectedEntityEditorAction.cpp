#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"

namespace r2::edit
{
	SelectedEntityEditorAction::SelectedEntityEditorAction(Editor* editor, ecs::Entity entitySelected, s32 instanceSelected, ecs::Entity prevEntitySelected, s32 prevInstanceSelected)
		:EditorAction(editor)
		,mSelectedEntity(entitySelected)
		,mSelectedInstance(instanceSelected)
		,mPrevSelectedEntity(prevEntitySelected)
		,mPrevSelectedInstance(prevInstanceSelected)
	{
	}

	void SelectedEntityEditorAction::Undo()
	{
		evt::EditorEntitySelectedEvent e(mPrevSelectedEntity, mPrevSelectedInstance, mSelectedEntity, mSelectedInstance);
		mnoptrEditor->PostEditorEvent(e);
	}

	void SelectedEntityEditorAction::Redo()
	{
		evt::EditorEntitySelectedEvent e(mSelectedEntity, mSelectedInstance, mPrevSelectedEntity, mPrevSelectedInstance);
		mnoptrEditor->PostEditorEvent(e);
	}
}


#endif