#include "r2pch.h"
#include "r2/Editor/EditorActions/CreateEntityEditorAction.h"
#include "r2/Game/ECS/Components/EditorNameComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::edit
{

	CreateEntityEditorAction::CreateEntityEditorAction(SceneGraph* sceneGraph, ecs::Entity parent /*= ecs::INVALID_ENTITY*/)
		:mnoptrSceneGraph(sceneGraph)
		,mCreatedEntity(ecs::INVALID_ENTITY)
		,mParentEntity(parent)
	{

	}

	void CreateEntityEditorAction::Undo()
	{
		mnoptrSceneGraph->DestroyEntity(mCreatedEntity);
	}

	void CreateEntityEditorAction::Redo()
	{
		mCreatedEntity = mnoptrSceneGraph->CreateEntity(mParentEntity);

		ecs::ECSCoordinator* coordinator = mnoptrSceneGraph->GetECSCoordinator();

		ecs::EditorNameComponent nameComponent;

		nameComponent.editorName = std::string("Entity - ") + std::to_string(mCreatedEntity);
		
		coordinator->AddComponent<ecs::EditorNameComponent>(mCreatedEntity, nameComponent);
	}
}