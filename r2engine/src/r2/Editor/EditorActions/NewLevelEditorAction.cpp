#include "r2pch.h"

#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/NewLevelEditorAction.h"
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"

namespace r2::edit
{
	NewLevelEditorAction::NewLevelEditorAction(Editor* editor, const EditorLevel& newEditorLevel, const EditorLevel& lastEditorLevel)
		:EditorAction(editor)
		,mNewEditorLevel(newEditorLevel)
		,mLastEditorLevel(lastEditorLevel)
	{
	}

	void NewLevelEditorAction::Undo()
	{
		mnoptrEditor->SetCurrentLevel(mLastEditorLevel);

		evt::EditorSetEditorLevel editorSetLevelEvent(mLastEditorLevel);
		mnoptrEditor->PostEditorEvent(editorSetLevelEvent);
	}

	void NewLevelEditorAction::Redo()
	{
		mnoptrEditor->SetCurrentLevel(mNewEditorLevel);

		evt::EditorNewLevelCreatedEvent newLevelEvent(mNewEditorLevel);
		mnoptrEditor->PostEditorEvent(newLevelEvent);
	}
}