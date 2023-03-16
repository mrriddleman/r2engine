#ifndef __ENTITY_EDITOR_FLAG_CHANGED_EDITOR_ACTION_H__
#define __ENTITY_EDITOR_FLAG_CHANGED_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class EntityEditorFlagChangedEditorAction : public EditorAction
	{
	public:
		EntityEditorFlagChangedEditorAction(Editor* editor, ecs::Entity e, u32 flag, bool enabled);
		virtual void Undo() override;
		virtual void Redo() override;

	private:
		ecs::Entity mEntity;
		u32 mFlag;
		bool mEnabled;
	};
}

#endif

#endif