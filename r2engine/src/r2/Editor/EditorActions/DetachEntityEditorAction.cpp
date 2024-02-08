#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/DetachEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

namespace r2::edit
{
	DetachEntityEditorAction::DetachEntityEditorAction(Editor* editor, ecs::Entity entityToDetach, ecs::Entity oldParent, ecs::eHierarchyAttachmentType attachmentType)
		:EditorAction(editor)
		,mEntityToDetach(entityToDetach)
		,mOldParent(oldParent)
		,mAttachmentType(attachmentType)
	{

	}

	void DetachEntityEditorAction::Undo()
	{
		mnoptrEditor->GetSceneGraph().Attach(mEntityToDetach, mOldParent, mAttachmentType);

		evt::EditorEntityAttachedToNewParentEvent e(mEntityToDetach, ecs::INVALID_ENTITY, mOldParent);

		mnoptrEditor->PostEditorEvent(e);
	}

	void DetachEntityEditorAction::Redo()
	{
		mnoptrEditor->GetSceneGraph().Detach(mEntityToDetach, mAttachmentType);

		evt::EditorEntityAttachedToNewParentEvent e(mEntityToDetach, mOldParent, ecs::INVALID_ENTITY);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif