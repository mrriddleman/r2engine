#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/AttachEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

namespace r2::edit
{

	AttachEntityEditorAction::AttachEntityEditorAction(Editor* editor, ecs::Entity entityToAttach, ecs::Entity oldParent, ecs::Entity newParent)
		:EditorAction(editor)
		,mEntityToAttach(entityToAttach)
		,mOldParent(oldParent)
		,mNewParent(newParent)
	{

	}

	void AttachEntityEditorAction::Undo()
	{
		mnoptrEditor->GetSceneGraph().Attach(mEntityToAttach, mOldParent);

		evt::EditorEntityAttachedToNewParentEvent e(mEntityToAttach, mNewParent, mOldParent);

		mnoptrEditor->PostEditorEvent(e);
	}

	void AttachEntityEditorAction::Redo()
	{
		mnoptrEditor->GetSceneGraph().Attach(mEntityToAttach, mNewParent);

		evt::EditorEntityAttachedToNewParentEvent e(mEntityToAttach, mOldParent, mNewParent);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif