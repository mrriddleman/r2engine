#ifndef __CREATE_ENTITY_EDITOR_ACTION_H__
#define __CREATE_ENTITY_EDITOR_ACTION_H__

#ifdef R2_EDITOR
#include "r2/Editor/EditorActions/EditorAction.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class CreateEntityEditorAction : public EditorAction
	{
	public:

		CreateEntityEditorAction(Editor* editor, ecs::Entity parent = ecs::INVALID_ENTITY);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		ecs::Entity mCreatedEntity;
		ecs::Entity mParentEntity;
	};
}

#endif
#endif // __CREATE_ENTITY_EDITOR_ACTION_H__
