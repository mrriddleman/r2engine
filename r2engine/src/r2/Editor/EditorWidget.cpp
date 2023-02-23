#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI
#include "r2/Editor/EditorWidget.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace r2::edit
{

	EditorWidget::EditorWidget()
		:mnoptrEditor(nullptr)
	{

	}

	EditorWidget::~EditorWidget()
	{

	}

	void EditorWidget::Init(Editor* noptrEditor)
	{

	}

	void EditorWidget::Shutdown()
	{

	}

	void EditorWidget::OnEvent(evt::Event& e)
	{

	}

	void EditorWidget::Update()
	{

	}

	void EditorWidget::Render(u32 dockingSpaceID)
	{

	}

	void EditorWidget::DockWindowInViewport(u32 dockingSpaceID, const std::string& windowName, DockingDirection dir, float amount)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		ImGui::DockBuilderRemoveNode(dockingSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(dockingSpaceID, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockingSpaceID, viewport->Size);
		
		auto dock_id_dir = ImGui::DockBuilderSplitNode(dockingSpaceID, dir, amount, nullptr, &dockingSpaceID);

		ImGui::DockBuilderDockWindow(windowName.c_str(), dock_id_dir);

		ImGui::DockBuilderFinish(dockingSpaceID);
	}

}

#endif