#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/CreateEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::edit
{
	CreateEntityEditorAction::CreateEntityEditorAction(Editor* editor, ecs::Entity parent /*= ecs::INVALID_ENTITY*/)
		:EditorAction(editor)
		,mCreatedEntity(ecs::INVALID_ENTITY)
		,mParentEntity(parent)
	{

	}

	void CreateEntityEditorAction::Undo()
	{
		mnoptrEditor->GetSceneGraph().DestroyEntity(mCreatedEntity);

		r2::evt::EditorEntityDestroyedEvent e(mCreatedEntity);

		mnoptrEditor->PostEditorEvent(e);
	}

	void CreateEntityEditorAction::Redo()
	{
		mCreatedEntity = mnoptrEditor->GetSceneGraph().CreateEntity(mParentEntity);

		ecs::ECSCoordinator* coordinator = mnoptrEditor->GetSceneGraph().GetECSCoordinator();

		ecs::EditorComponent nameComponent;

		nameComponent.editorName = std::string("Entity - ") + std::to_string(mCreatedEntity);
		
		nameComponent.flags.Set(ecs::EDITOR_FLAG_SHOW_ENTITY);
		
		nameComponent.flags.Set(ecs::EDITOR_FLAG_ENABLE_ENTITY);

		coordinator->AddComponent<ecs::EditorComponent>(mCreatedEntity, nameComponent);

		r2::evt::EditorEntityCreatedEvent e(mCreatedEntity);

		mnoptrEditor->PostEditorEvent(e);
	}
}

#endif