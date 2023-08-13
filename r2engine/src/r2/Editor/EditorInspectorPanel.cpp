#include "r2pch.h"

#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Core/Events/Event.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"
#include "r2/Core/Engine.h"
#include "r2/Core/Application.h"

namespace r2::edit
{

	InspectorPanel::InspectorPanel()
		:mEntitySelected(ecs::INVALID_ENTITY)
	{

	}

	InspectorPanel::~InspectorPanel()
	{

	}

	void InspectorPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

		RegisterAllEngineComponentWidgets(*this);
		CENG.GetApplication().RegisterComponentImGuiWidgets(*this);
	}

	void InspectorPanel::Shutdown()
	{

	}

	void InspectorPanel::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);


		dispatcher.Dispatch<r2::evt::EditorEntitySelectedEvent>([this](const r2::evt::EditorEntitySelectedEvent& e)
			{
				mEntitySelected = e.GetEntity();
				return e.ShouldConsume();
			});
	}

	void InspectorPanel::Update()
	{

	}

	void InspectorPanel::Render(u32 dockingSpaceID)
	{
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

		bool open = true;
		
		if (ImGui::Begin("Inspector", &open))
		{
			ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();

			if (coordinator && mEntitySelected != ecs::INVALID_ENTITY)
			{
				const ecs::Signature& entitySignature = coordinator->GetSignature(mEntitySelected);

				for (size_t i = 0; i < entitySignature.size(); ++i)
				{
					if (entitySignature[i])
					{
						ecs::ComponentType componentType = static_cast<ecs::ComponentType>(i);

						auto iter = mComponentWidgets.find(componentType);

						if (iter != mComponentWidgets.end())
						{
							mComponentWidgets[componentType](mEntitySelected, coordinator);
						}
					}
				}
			}
			
			ImGui::End();
		}
	}

	void InspectorPanel::RegisterComponentType(r2::ecs::ComponentType componentType, InspectorPanelComponentWidgetFunc componentWidget)
	{
		mComponentWidgets[componentType] = componentWidget;
	}

}

#endif