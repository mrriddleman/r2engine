#include "r2pch.h"

#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorScenePanel.h"
#include "imgui.h"
namespace r2::edit
{

	ScenePanel::ScenePanel()
	{

	}

	ScenePanel::~ScenePanel()
	{

	}

	void ScenePanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;
	}

	void ScenePanel::Shutdown()
	{

	}

	void ScenePanel::OnEvent(evt::Event& e)
	{

	}

	void ScenePanel::Update()
	{

	}

	void ScenePanel::Render(u32 dockingSpaceID)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);

		bool open = true;

		if (!ImGui::Begin("ScenePanel", &open))
		{
			ImGui::End();
			return;
		}

		ImGui::End();
	}

}

#endif