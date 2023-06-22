#ifndef __NEW_LEVEL_EDITOR_ACTION_H__
#define __NEW_LEVEL_EDITOR_ACTION_H__

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Editor/EditorLevel.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class NewLevelEditorAction : public EditorAction
	{
	public:
		NewLevelEditorAction(Editor* editor, const EditorLevel& newEditorLevel, const EditorLevel& lastEditorLevel);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		EditorLevel mNewEditorLevel;
		EditorLevel mLastEditorLevel;
	};
}

#endif
