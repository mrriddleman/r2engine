#ifndef __DETACH_ENTITY_EDITOR_ACTION_H__
#define __DETACH_ENTITY_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

namespace r2::edit
{
	class DetachEntityEditorAction : public EditorAction
	{
	public:
		DetachEntityEditorAction(Editor* editor, ecs::Entity entityToDetach, ecs::Entity oldParent, ecs::eHierarchyAttachmentType attachmentType);
		virtual void Undo() override;
		virtual void Redo() override;

	private:
		ecs::Entity mEntityToDetach;
		ecs::Entity mOldParent;
		ecs::eHierarchyAttachmentType mAttachmentType;
	};
}

#endif
#endif // __DETACH_ENTITY_EDITOR_ACTION_H__
