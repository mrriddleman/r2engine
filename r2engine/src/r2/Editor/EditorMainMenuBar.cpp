#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorMainMenuBar.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <glad/glad.h>
#include "r2/Editor/Editor.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>
#include <fstream>
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Platform/Platform.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"

namespace r2::edit 
{
	MainMenuBar::MainMenuBar()
		:mShowCreateNewLevelModal(false)
	{

	}
	MainMenuBar::~MainMenuBar()
	{

	}
	void MainMenuBar::Init(Editor* noptrEditor) 
	{
		mnoptrEditor = noptrEditor;

		LoadRecentsFile();
	}

	void MainMenuBar::Shutdown()
	{

	}

	void MainMenuBar::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::EditorLevelLoadedEvent>([this](const r2::evt::EditorLevelLoadedEvent& e) {
			if (e.GetFilePath() != "")
			{
				SaveLevelToRecents(e.GetFilePath());
			}

			return e.ShouldConsume();
			});
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
					OpenCreateNewLevelWindow();
				}
				if (ImGui::MenuItem("Open Level"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseLevelFileDlgKey", "Choose File", ".rlvl", mnoptrEditor->GetAppLevelPath());
				}
				if (ImGui::BeginMenu("Open Recent"))
				{
					std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();

					size_t numPathsToShow = std::min(mLastLevelPathsOpened.size(), 10ull);

					for (u32 i = 0; i < numPathsToShow; ++i)
					{
						std::filesystem::path nextPath = mLastLevelPathsOpened[i];
						
						std::filesystem::path relPath = nextPath.lexically_relative(appLevelPath);

						if (ImGui::MenuItem(relPath.string().c_str()))
						{
							mnoptrEditor->LoadLevel(mLastLevelPathsOpened[i]);
							//OpenLevel(mLastLevelPathsOpened[i]);
							break;
						}
					}

					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Unload Level"))
				{
					UnloadLevel();
				}

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					//@TODO(Serge): maybe should be an action
					mnoptrEditor->Save();

					const auto& level = mnoptrEditor->GetEditorLevelConst();

					std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();

					std::filesystem::path fullLevelPath = appLevelPath / std::string(level.GetGroupName()) / (std::string(level.GetLevelName()) + ".rlvl");

					SaveLevelToRecents(fullLevelPath.string());
				}
				if (ImGui::MenuItem("Save As.."))
				{
					//what should happen:
					//- open a file dialog with the current level name 
					//- if we change the name - we need to update the level data

					const auto& level = mnoptrEditor->GetEditorLevelConst();

					std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();

					std::filesystem::path fullLevelPath = appLevelPath / std::string(level.GetGroupName()) / (std::string(level.GetLevelName()) + ".rlvl");
					//ImGui::SetNextWindowSize(ImVec2(600, 400));
					
					
					ImGuiFileDialog::Instance()->OpenDialog("SaveAsLevelFileDlgKey", "Save As File", ".rlvl", fullLevelPath.string());
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

		if (mShowCreateNewLevelModal)
		{
			ImGui::OpenPopup("New Level");

			ImGui::SetNextWindowSize(ImVec2(400, 100));
			const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
			auto vec2 = ImVec2(main_viewport->WorkPos.x + (main_viewport->WorkSize.x / 2.0f) - (200), main_viewport->WorkPos.y + (main_viewport->WorkSize.y / 2.0f) - (50));
			ImGui::SetNextWindowPos(vec2, ImGuiCond_None);
			mShowCreateNewLevelModal = false;
		}

		ShowCreateNewLevelModal();
		
		ImVec2 maxSize = ImVec2((float)1200, (float)800);  // The full display area
		ImVec2 minSize = ImVec2((float)1200 * 0.5, (float)800 * 0.5);  // Half the display area
		// display
		if (ImGuiFileDialog::Instance()->Display("ChooseLevelFileDlgKey", ImGuiWindowFlags_NoCollapse, minSize, maxSize))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				mnoptrEditor->LoadLevel(filePathName);
			//	OpenLevel(filePathName);
			}

			// close
			ImGuiFileDialog::Instance()->Close();
		}

		
		if (ImGuiFileDialog::Instance()->Display("SaveAsLevelFileDlgKey", ImGuiWindowFlags_NoCollapse, minSize, maxSize))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

				printf("Save As - filePathName: %s\n", filePathName.c_str());
				printf("Save As - filePath: %s\n\n", filePath.c_str());

				auto& level = mnoptrEditor->GetEditorLevelRef();

				//now figure out if the level name changed at all
				//get the group name and level name from the filePathName
				std::filesystem::path levelRelativePath = std::filesystem::path(filePathName).lexically_relative(appLevelPath);

				R2_CHECK(levelRelativePath.parent_path().stem().string() != "", "Group name is empty");
				R2_CHECK(levelRelativePath.stem().string() != "", "level name is empty");

				if ((levelRelativePath.parent_path().stem().string() != level.GetGroupName() ||
					levelRelativePath.stem().string() != level.GetLevelName()))
				{
					level.SetGroupName(levelRelativePath.parent_path().stem().string().c_str());
					level.SetLevelName(levelRelativePath.stem().string().c_str());	
				}

				mnoptrEditor->Save();
				//We're loading it again so that everything is properly sync'd in the level manager
				mnoptrEditor->LoadLevel(filePathName);
				SaveLevelToRecents(filePathName);
			}

			ImGuiFileDialog::Instance()->Close();
		}

	}

	void MainMenuBar::OpenCreateNewLevelWindow()
	{
		mShowCreateNewLevelModal = true;
		mGroupName = "NewGroup";
		mLevelName = "NewLevel";
	}

	void MainMenuBar::ShowCreateNewLevelModal()
	{
		if (ImGui::BeginPopupModal("New Level"))
		{
			//do the new level setup here

			ImGui::Text("Level Name: ");
			ImGui::SameLine(0.0f, 20.0f);
			char levelName[1000];
			strncpy(levelName, mLevelName.c_str(), 1000);
			ImGui::InputText("##hidelabel0", levelName, 1000, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll);

			mLevelName = levelName;

			ImGui::Text("Group Name: ");
			ImGui::SameLine(0.0f, 20.0f);
			char groupName[1000];
			strncpy(groupName, mGroupName.c_str(), 1000);
			ImGui::InputText("##hidelabel1", groupName, 1000, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll);

			mGroupName = groupName;

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine(0.0f, 100.0f);

			ImGui::SameLine(ImGui::GetWindowWidth() - 80);

			if (ImGui::Button("Create"))
			{
				mnoptrEditor->CreateNewLevel(groupName, levelName);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void MainMenuBar::SaveLevelToRecents(const std::string& filePathName)
	{
		char sanitizedFilePath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(filePathName.c_str(), sanitizedFilePath);

		mLastLevelPathsOpened.erase(std::remove(mLastLevelPathsOpened.begin(), mLastLevelPathsOpened.end(), sanitizedFilePath), mLastLevelPathsOpened.end());

		mLastLevelPathsOpened.push_front(sanitizedFilePath);

		SaveRecentsFile();
	}

	void MainMenuBar::LoadRecentsFile()
	{
		std::fstream fs;

		std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();

		std::filesystem::path recentsFile = appLevelPath / "recents.txt";

		fs.open(recentsFile, std::fstream::in);

		if (fs.good())
		{
			std::string line;
			while (std::getline(fs, line))
			{
				char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::SanitizeSubPath(line.c_str(), sanitizedPath);
				mLastLevelPathsOpened.push_back(sanitizedPath);
			}
		}

		fs.close();

	}

	void MainMenuBar::SaveRecentsFile()
	{
		std::fstream fs;

		std::filesystem::path appLevelPath = mnoptrEditor->GetAppLevelPath();

		std::filesystem::path recentsFile = appLevelPath / "recents.txt";

		fs.open(recentsFile, std::fstream::out);

		for (const std::string& line : mLastLevelPathsOpened)
		{
			fs << line << std::endl;
		}

		fs.close();
	}

	void MainMenuBar::UnloadLevel()
	{
		mnoptrEditor->UnloadCurrentLevel();
	}
}


#endif