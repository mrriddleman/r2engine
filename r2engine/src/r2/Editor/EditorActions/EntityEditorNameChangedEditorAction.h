#ifndef __ENTITY_EDITOR_NAME_CHANGED_EDITOR_ACTION_H__
#define __ENTITY_EDITOR_NAME_CHANGED_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class EntityEditorNameChangedEditorAction : public EditorAction
	{
	public:
		EntityEditorNameChangedEditorAction(Editor* editor, ecs::Entity entity, const std::string& newName, const std::string& prevName);
		virtual void Undo() override;
		virtual void Redo() override;
	private:
		ecs::Entity mEntity;
		std::string mNewName;
		std::string mPreviousName;
	};
}

#endif

#endif