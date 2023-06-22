#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorMainMenuBar.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <glad/glad.h>
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/NewLevelEditorAction.h"

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
		bool showNewLevelPopup = false;
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{

				
				if (ImGui::MenuItem("New Level"))
				{
					showNewLevelPopup = true;
					mNewEditorLevel = {};
				}
				if (ImGui::MenuItem("Open Level"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseLevelFileDlgKey", "Choose File", ".rlvl", mnoptrEditor->GetAppLevelPath());
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
					//@TODO(Serge): maybe should be an action
					mnoptrEditor->Save();
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
					mnoptrEditor->UndoLastAction();
				}
				if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
				{
					mnoptrEditor->RedoLastAction();
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

		if (showNewLevelPopup)
		{
			ImGui::OpenPopup("New Level");
			ImGui::SetNextWindowSize(ImVec2(400, 100));

			const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
			auto vec2 = ImVec2(main_viewport->WorkPos.x + (main_viewport->WorkSize.x / 2.0f) - (200), main_viewport->WorkPos.y + (main_viewport->WorkSize.y / 2.0f) - (50));
			ImGui::SetNextWindowPos(vec2, ImGuiCond_None);
		}

		if (ImGui::BeginPopupModal("New Level"))
		{
			//do the new level setup here

			ImGui::Text("Level Name: ");
			ImGui::SameLine(0.0f, 20.0f);
			char levelName[1000];
			strncpy(levelName, mNewEditorLevel.GetLevelName().c_str(), 1000);
			ImGui::InputText("##hidelabel0", levelName, 1000, ImGuiInputTextFlags_CharsNoBlank);
			
			mNewEditorLevel.SetLevelName(levelName);

			ImGui::Text("Group Name: ");
			ImGui::SameLine(0.0f, 20.0f);
			char groupName[1000];
			strncpy(groupName, mNewEditorLevel.GetGroupName().c_str(), 1000);
			ImGui::InputText("##hidelabel1", groupName, 1000, ImGuiInputTextFlags_CharsNoBlank);

			mNewEditorLevel.SetGroupName(groupName);

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine(0.0f, 100.0f);

			ImGui::SameLine(ImGui::GetWindowWidth() - 80);

			if (ImGui::Button("Create"))
			{
				mNewEditorLevel.SetVersion(1);
				mNewEditorLevel.SetLevelName(levelName);
				mNewEditorLevel.SetGroupName(groupName);

				mnoptrEditor->PostNewAction(std::make_unique<NewLevelEditorAction>(mnoptrEditor, mNewEditorLevel, mnoptrEditor->GetEditorLevel()));
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		
		// display
		if (ImGuiFileDialog::Instance()->Display("ChooseLevelFileDlgKey"))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				// action

				mnoptrEditor->LoadLevel(filePathName);
			}

			// close
			ImGuiFileDialog::Instance()->Close();
		}

	}
}


#endif