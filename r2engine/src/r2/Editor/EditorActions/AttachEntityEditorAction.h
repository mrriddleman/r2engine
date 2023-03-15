#ifndef __ATTACH_ENTITY_EDITOR_ACTION_H__
#define __ATTACH_ENTITY_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

namespace r2::edit
{
	class AttachEntityEditorAction : public EditorAction
	{
	public:
		AttachEntityEditorAction(Editor* editor, ecs::Entity entityToAttach, ecs::Entity oldParent, ecs::Entity newParent);
		virtual void Undo() override;
		virtual void Redo() override;

	private:
		ecs::Entity mEntityToAttach;
		ecs::Entity mOldParent;
		ecs::Entity mNewParent;
	};
}

#endif
#endif // __ATTACH_ENTITY_EDITOR_ACTION_H__
