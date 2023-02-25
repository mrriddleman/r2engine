#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorMainMenuBar.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <glad/glad.h>

namespace r2::edit 
{
	MainMenuBar::MainMenuBar()
	{

	}
	MainMenuBar::~MainMenuBar()
	{

	}
	void MainMenuBar::Init(Editor* noptrEditor) 
	{
		mnoptrEditor = noptrEditor;

		
	}

	void MainMenuBar::Shutdown()
	{

	}

	void MainMenuBar::OnEvent(evt::Event& e)
	{

	}

	void MainMenuBar::Update()
	{

	}

	void MainMenuBar::Render(u32 dockingSpaceID)
	{
		ImVec2 vWindowSize = ImGui::GetMainViewport()->Size;
		ImVec2 vPos0 = ImGui::GetMainViewport()->Pos;

		ImGui::SetNextWindowPos(ImVec2((float)vPos0.x, (float)vPos0.y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2((float)vWindowSize.x, (float)vWindowSize.y), ImGuiCond_Always);

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Level"))
				{

				}
				if (ImGui::MenuItem("Open Level"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".*,.glsl,.fs,.vs,.gs", ".");
				}
				if (ImGui::BeginMenu("Open Recent"))
				{
					//@TODO(Serge): populate with a proper vector of last levels/scenes
					ImGui::MenuItem("Level 1");
					ImGui::MenuItem("Level 2");
					ImGui::MenuItem("Level 3");
					ImGui::MenuItem("Level 4");
					ImGui::MenuItem("Level 5");
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{

				}
				if (ImGui::MenuItem("Save As.."))
				{

				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "Ctrl+Z"))
				{

				}
				if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
				{

				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{

				if (ImGui::MenuItem("Console Log"))
				{

				}
				if (ImGui::MenuItem("Layer Panel"))
				{

				}
				if (ImGui::MenuItem("Memory Visualizer"))
				{

				}
				if (ImGui::MenuItem("Renderer"))
				{

				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}


		// display
		if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				// action
			}



			// close
			ImGuiFileDialog::Instance()->Close();
		}

	}
}


#endif