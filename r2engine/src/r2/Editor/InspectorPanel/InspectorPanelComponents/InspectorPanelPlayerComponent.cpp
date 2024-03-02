#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Core/Engine.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelPlayerComponent.h"
#include "r2/Game/ECS/Components/PlayerComponent.h"

#include "imgui.h"

namespace r2::edit 
{

	InspectorPanelPlayerComponentDataSource::InspectorPanelPlayerComponentDataSource()
		:InspectorPanelComponentDataSource("Player Component", 0, 0)
		, mnoptrEditor(nullptr)
	{

	}

	InspectorPanelPlayerComponentDataSource::InspectorPanelPlayerComponentDataSource(r2::Editor* noptrEditor, ecs::ECSCoordinator* coordinator)
		:InspectorPanelComponentDataSource("Player Component", coordinator->GetComponentType<ecs::PlayerComponent>(), coordinator->GetComponentTypeHash<ecs::PlayerComponent>())
		, mnoptrEditor(noptrEditor)
	{

	}

	void InspectorPanelPlayerComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance)
	{
		r2::ecs::PlayerComponent& playerComponent = coordinator->GetComponent<r2::ecs::PlayerComponent>(theEntity);

		std::string previewString = "Invalid Player";

		if (playerComponent.playerID != r2::InvalidPlayerID)
		{
			previewString = "Player " + std::to_string(playerComponent.playerID);
		}

		constexpr u32 NUM_AVAILABLE_PLAYERS = Engine::NUM_PLATFORM_CONTROLLERS + 1; //+1 for keyboard + mouse

		ImGui::Text("Player ID: ");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##label playerid", previewString.c_str()))
		{
			if (ImGui::Selectable("Invalid Player", playerComponent.playerID == InvalidPlayerID))
			{
				playerComponent.playerID = InvalidPlayerID;
			}

			for (u32 i = 0; i < NUM_AVAILABLE_PLAYERS; ++i)
			{
				std::string playerIDStr = "Player " + std::to_string(i);

				if (ImGui::Selectable(playerIDStr.c_str(), playerComponent.playerID == i))
				{
					playerComponent.playerID = i;
				}
			}

			ImGui::EndCombo();
		}
	}

	bool InspectorPanelPlayerComponentDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelPlayerComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelPlayerComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::PlayerComponent>(theEntity);
	}

	void* InspectorPanelPlayerComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelPlayerComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		const ecs::PlayerComponent& hierarchyComponent = coordinator->GetComponent<ecs::PlayerComponent>(theEntity);

		//This should probably be an action?
		coordinator->RemoveComponent<ecs::PlayerComponent>(theEntity);
	}

	void InspectorPanelPlayerComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelPlayerComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::PlayerComponent newPlayerComponent;
		newPlayerComponent.playerID = r2::InvalidPlayerID;
		//This should probably be an action?
		coordinator->AddComponent<ecs::PlayerComponent>(theEntity, newPlayerComponent);
	}

	void InspectorPanelPlayerComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{

	}

}

#endif