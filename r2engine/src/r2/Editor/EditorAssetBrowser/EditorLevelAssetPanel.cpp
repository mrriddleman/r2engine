#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorAssetBrowser/EditorLevelAssetPanel.h"
#include "imgui.h"

namespace r2::edit
{

	LevelAssetPanel::LevelAssetPanel()
	{

	}

	LevelAssetPanel::~LevelAssetPanel()
	{

	}

	void LevelAssetPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

	}

	void LevelAssetPanel::Shutdown()
	{

	}

	void LevelAssetPanel::OnEvent(evt::Event& e)
	{

	}

	void LevelAssetPanel::Update()
	{

	}

	void LevelAssetPanel::Render(u32 dockingSpaceID)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 450), ImGuiCond_FirstUseEver);

		bool open = true;

		if (!ImGui::Begin("Level Assets", &open))
		{
			ImGui::End();
			return;
		}





		ImGui::End();
	}

}

#endif