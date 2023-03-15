#ifndef __DESTROY_ENTITY_EDITOR_ACTION_H__
#define __DESTROY_ENTITY_EDITOR_ACTION_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Components/EditorNameComponent.h"
#include "r2/Game/ECS/Components/HeirarchyComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"

namespace
{
	class Editor;
}

namespace r2::edit
{
	class DestroyEntityEditorAction : public EditorAction
	{
	public:

		DestroyEntityEditorAction(Editor* editor, ecs::Entity entityToDestroy, ecs::Entity parentOfEntityToDestroy);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		ecs::Entity mEntityToDestroy;
		ecs::Entity mParentOfEntityToDestory;
		std::vector<ecs::Entity> mChildren; 

		ecs::EditorNameComponent mEntityEditorNameComponent;
		ecs::HeirarchyComponent mEntityHeirarchyComponent;
		ecs::TransformComponent mEntityTransformComponent;

	};
}

#endif
#endif // __DESTROY_ENTITY_EDITOR_ACTION_H__
