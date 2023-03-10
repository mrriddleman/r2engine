#ifndef __CREATE_ENTITY_EDITOR_ACTION_H__
#define __CREATE_ENTITY_EDITOR_ACTION_H__

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

namespace r2::edit
{
	class CreateEntityEditorAction : public EditorAction
	{
	public:

		CreateEntityEditorAction(SceneGraph* sceneGraph, ecs::Entity parent = ecs::INVALID_ENTITY);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		SceneGraph* mnoptrSceneGraph;
		ecs::Entity mCreatedEntity;
		ecs::Entity mParentEntity;
	};
}

#endif // __CREATE_ENTITY_EDITOR_ACTION_H__
