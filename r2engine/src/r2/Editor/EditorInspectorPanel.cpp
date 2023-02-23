#include "r2pch.h"

#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorInspectorPanel.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace r2::edit
{

	InspectorPanel::InspectorPanel()
	{

	}

	InspectorPanel::~InspectorPanel()
	{

	}

	void InspectorPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;
	}

	void InspectorPanel::Shutdown()
	{

	}

	void InspectorPanel::OnEvent(evt::Event& e)
	{

	}

	void InspectorPanel::Update()
	{

	}

	void InspectorPanel::Render(u32 dockingSpaceID)
	{
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

		//static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		//ImGuiViewport* viewport = ImGui::GetMainViewport();

		//ImGui::DockBuilderRemoveNode(dockingSpaceID); // clear any previous layout
		//ImGui::DockBuilderAddNode(dockingSpaceID, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
		//ImGui::DockBuilderSetNodeSize(dockingSpaceID, viewport->Size);


		//auto dock_id_top = ImGui::DockBuilderSplitNode(dockingSpaceID, ImGuiDir_Up, 0.2f, nullptr, &dockingSpaceID);
		//auto dock_id_down = ImGui::DockBuilderSplitNode(dockingSpaceID, ImGuiDir_Down, 0.25f, nullptr, &dockingSpaceID);
		//auto dock_id_left = ImGui::DockBuilderSplitNode(dockingSpaceID, ImGuiDir_Left, 0.2f, nullptr, &dockingSpaceID);
		//auto dock_id_right = ImGui::DockBuilderSplitNode(dockingSpaceID, ImGuiDir_Right, 0.15f, nullptr, &dockingSpaceID);


		bool open = true;
		
		if (!ImGui::Begin("Inspector", &open))
		{
			ImGui::End();
			return;
		}

		ImGui::End();
	}

}

#endif