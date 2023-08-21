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
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"

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

				R2_CHECK(entitySignature.test(coordinator->GetComponentType<r2::ecs::EditorComponent>()), "Should always have an editor component");

				InspectorPanelEditorComponent(mnoptrEditor, mEntitySelected, coordinator);

				for (size_t i=0; i < mComponentWidgets.size(); ++i)
				{
					if (entitySignature.test(mComponentWidgets[i].GetComponentType()))
					{
						mComponentWidgets[i].ImGuiDraw(*this, mEntitySelected);
					}
				}
			}
			
			ImGui::End();
		}
	}

	void InspectorPanel::RegisterComponentType(
		const std::string& componentName,
		u32 sortOrder,
		r2::ecs::ComponentType componentType,
		InspectorPanelComponentWidgetFunc componentWidgetFunc,
		InspectorPanelRemoveComponentFunc removeComponentFunc)
	{
		InspectorPanelComponentWidget componentWidget{ componentName, componentType, componentWidgetFunc, removeComponentFunc };

		componentWidget.SetSortOrder(sortOrder);

		auto iter = std::find_if(mComponentWidgets.begin(), mComponentWidgets.end(), [&](const InspectorPanelComponentWidget& widget)
			{
				return widget.GetComponentType() == componentType;
			});

		if (iter == mComponentWidgets.end())
		{
			mComponentWidgets.push_back(componentWidget);

			std::sort(mComponentWidgets.begin(), mComponentWidgets.end(), [](const InspectorPanelComponentWidget& w1, const InspectorPanelComponentWidget& w2)
				{
					return w1.GetSortOrder() < w2.GetSortOrder();
				});
		}
	}

	Editor* InspectorPanel::GetEditor()
	{
		return mnoptrEditor;
	}
}

#endif