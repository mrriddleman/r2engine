#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorAssetPanel.h"
#include "imgui.h"

namespace r2::edit
{

	AssetPanel::AssetPanel()
	{

	}

	AssetPanel::~AssetPanel()
	{

	}

	void AssetPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;
	}

	void AssetPanel::Shutdown()
	{

	}

	void AssetPanel::OnEvent(evt::Event& e)
	{

	}

	void AssetPanel::Update()
	{

	}

	void AssetPanel::Render(u32 dockingSpaceID)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 450), ImGuiCond_FirstUseEver);

		bool open = true;

		if (!ImGui::Begin("AssetPanel", &open))
		{
			ImGui::End();
			return;
		}

		ImGui::End();
	}

}

#endif