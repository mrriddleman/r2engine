#ifndef __SELECTED_ENTITY_EDITOR_ACTION_H__
#define __SELECTED_ENTITY_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class SelectedEntityEditorAction : public EditorAction
	{
	public:
		SelectedEntityEditorAction(Editor* editor, ecs::Entity entitySelected, ecs::Entity prevEntitySelected);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		Editor* mnoptrEditor;
		ecs::Entity mSelectedEntity;
		ecs::Entity mPrevSelectedEntity;
	};
}

#endif
#endif