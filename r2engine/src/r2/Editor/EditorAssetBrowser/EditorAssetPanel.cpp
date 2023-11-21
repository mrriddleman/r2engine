#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorAssetBrowser/EditorAssetPanel.h"

#include "assetlib/RModel_generated.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/File/PathUtils.h"

#include "r2/Render/Model/Materials/Material_generated.h"

#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Editor/Editor.h"

#include "imgui.h"
#include <algorithm>
#include <string>
#include <cctype>

namespace r2::edit
{

	/// Try to find in the Haystack the Needle - ignore case
	bool FindStringIC(const std::string& strHaystack, const std::string& strNeedle)
	{
		auto it = std::search(
			strHaystack.begin(), strHaystack.end(),
			strNeedle.begin(), strNeedle.end(),
			[](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
		);
		return (it != strHaystack.end());
	}

	AssetPanel::AssetPanel()

	{

	}

	AssetPanel::~AssetPanel()
	{

	}

	void AssetPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

		mEngineRawDirectory = R2_ENGINE_ASSET_PATH;
		mEngineBinDirectory = R2_ENGINE_ASSET_BIN_PATH;

		char filePath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ROOT, "assets", filePath);
		mAppRawDirectory = filePath;
		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ROOT, "assets_bin", filePath);
		mAppBinDirectory = filePath;

		mCurrentDirectory = "";
		mCurrentBaseDirectory = "";



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

		if (!ImGui::Begin("Assets", &open))
		{
			ImGui::End();
			return;
		}

		static const int NUM_COLUMNS = 2;
		static float w = 300.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::BeginChild("Asset FileSystem", ImVec2(w, 0), true);
		ImGui::Text("File System");
		ImGui::Separator();
		ImGui::Separator();
		FileSystemPanel();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::InvisibleButton("vsplitter", ImVec2(10.0f, ImGui::GetContentRegionAvail().y));
		if (ImGui::IsItemActive())
		{
			w += ImGui::GetIO().MouseDelta.x;
		}
		ImGui::SameLine();

		ImGui::BeginChild("Asset Preview", ImVec2(0, 0), true);

		if (mCurrentDirectory != "")
		{
			if (ImGui::BeginPopupContextWindow("Asset Preview", ImGuiPopupFlags_MouseButtonRight))
			{
				if (!ShowContextMenuForPath(mCurrentDirectory, false))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}
			//if (ImGui::IsWindowHovered())
			//	ImGui::SetTooltip("Right-click to open popup");
		}

		if (mCurrentDirectory != "" && mCurrentBaseDirectory != "")
		{
			if (mCurrentDirectory != std::filesystem::path(mCurrentBaseDirectory))
			{
				if (ImGui::Button("<-"))
				{
					mCurrentDirectory = mCurrentDirectory.parent_path();
				}
			}
			ImGui::SameLine();
		}

		ImGui::Text("Assets Browser");
		ImGui::Separator();
		ImGui::Separator();
		AssetsBrowserPanel(ImGui::GetContentRegionAvail().x - w);
		ImGui::EndChild();

		ImGui::PopStyleVar();
		ImGui::End();
		

//		ImGui::ShowDemoWindow();
	}

	void AssetPanel::FileSystemPanel()
	{
		bool wasActivated = false;

		//@TODO(Serge): figure out if we need all four of these directories. Would be nice to just work with 2 of them...
		if (ImGui::CollapsingHeader("Engine Assets Raw"))
		{
			ShowDirectoryInFileSystemPanel(mEngineRawDirectory, wasActivated);
			if (wasActivated)
			{
				mCurrentBaseDirectory = mEngineRawDirectory;
			}
		}

		if (ImGui::CollapsingHeader("Game Assets Raw"))
		{
			ShowDirectoryInFileSystemPanel(mAppRawDirectory, wasActivated);
			if (wasActivated)
			{
				mCurrentBaseDirectory = mAppRawDirectory;
			}
		}

		if (ImGui::CollapsingHeader("Engine Assets Bin"))
		{
			ShowDirectoryInFileSystemPanel(mEngineBinDirectory, wasActivated);

			if (wasActivated)
			{
				mCurrentBaseDirectory = mEngineBinDirectory;
			}
		}

		if (ImGui::CollapsingHeader("Game Assets Bin"))
		{
			ShowDirectoryInFileSystemPanel(mAppBinDirectory, wasActivated);

			if (wasActivated)
			{
				mCurrentBaseDirectory = mAppBinDirectory;
			}
		}
	}

	void AssetPanel::AssetsBrowserPanel(int contentWidth)
	{
		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = contentWidth;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		u32 fileIcon = mnoptrEditor->GetEditorFileImage();
		u32 folderIcon = mnoptrEditor->GetEditorFolderImage();

		if (mCurrentDirectory != "" && mCurrentBaseDirectory != "")
		{
			ImGui::Columns(columnCount, 0, false);

			for (auto& directoryEntry : std::filesystem::directory_iterator(mCurrentDirectory))
			{
				const auto& path = directoryEntry.path();
				std::string filenameString = path.filename().string();

				ImGui::PushID(filenameString.c_str());
				unsigned int icon = directoryEntry.is_directory() ? folderIcon : fileIcon;
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton((ImTextureID)icon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

				if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
				{
					if (!ShowContextMenuForPath(directoryEntry.path(), true))
						ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}
				//if (ImGui::IsItemHovered())
				//	ImGui::SetTooltip("Right-click to open popup");

				//if (ImGui::BeginDragDropSource())
				//{
				//	std::filesystem::path relativePath(path);
				//	const wchar_t* itemPath = relativePath.c_str();
				//	ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
				//	ImGui::EndDragDropSource();
				//}

				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (directoryEntry.is_directory())
						mCurrentDirectory /= path.filename();

				}
				ImGui::TextWrapped(filenameString.c_str());

				ImGui::NextColumn();

				ImGui::PopID();
			}

			ImGui::Columns(1);
		}
	}

	void AssetPanel::ShowDirectoryInFileSystemPanel(const std::filesystem::path& directory, bool& wasActivated)
	{
		for (auto& directoryEntry : std::filesystem::directory_iterator(directory))
		{
			if (!directoryEntry.is_directory())
			{
				continue;
			}

			if (ImGui::TreeNode(directoryEntry.path().filename().string().c_str()))
			{
				if (!wasActivated &&( ImGui::IsItemActivated() || ImGui::IsItemActive()))
				{
					mCurrentDirectory = directoryEntry.path();
					wasActivated = true;
				}

				ShowDirectoryInFileSystemPanel(directoryEntry.path(), wasActivated);

				ImGui::TreePop();
			}

			if (!wasActivated && ImGui::IsItemClicked())
			{
				mCurrentDirectory = directoryEntry.path();
				wasActivated = true;
			}
		}
	}

	bool AssetPanel::ShowContextMenuForPath(const std::filesystem::path& path, bool wasButtonItem)
	{
		std::filesystem::path relativePath = path.lexically_relative(mCurrentBaseDirectory).string().c_str();

		if (relativePath == "")
		{
			return false;
		}

		static auto MATERIAL_BIN_EXT = std::string(".") + flat::MaterialExtension();
		static auto LEVEL_BIN_EXT = std::string(".") + flat::LevelDataExtension();
		static auto SOUND_DEFINITION_BIN_EXT = std::string(".") + flat::SoundDefinitionsExtension();
		static auto SOUND_BANK_EXT = ".bank";
		static auto RMODEL_BIN_EXT = std::string(".") + flat::RModelExtension();
		static auto MODEL_BIN_EXT = ".modl"; //@TODO(Serge): get rid of this
		static auto TEXTURE_PACK_MANIFEST_BIN_EXT = std::string(".") + flat::TexturePacksManifestExtension();
		static auto JSON_EXT = ".json";

		std::string relativePathString = relativePath.string();

		if (std::filesystem::is_directory(path))
		{
			if (FindStringIC(relativePathString, "material"))
			{
				MaterialsDirectoryContexMenu(path);
				return true;
			}
			else if (FindStringIC(relativePathString, "level"))
			{
				LevelsDirectoryContexMenu(path);
				return true;
			}
			else if (FindStringIC(relativePathString, "texture") && wasButtonItem)
			{
				TexturePackDirectoryContexMenu(path);
				return true;
			}
		}
		else
		{
			//specific file
			if (FindStringIC(relativePathString, "material"))
			{
				//check the extension
				//if it's a .mtrl then the options should be import to level
				//else then... I dunno
				if (relativePath.extension().string() == MATERIAL_BIN_EXT)
				{
					MaterialBinContextMenu(path);
					return true;
				}
				else if(relativePath.extension().string() == JSON_EXT)
				{
					MaterialRawContextMenu(path);
					return true;
				}

			}
			else if (FindStringIC(relativePathString, "model"))
			{
				if (relativePath.extension().string() == RMODEL_BIN_EXT)
				{
					ModelBinContextMenu(path);
					return true;
				}
				else //@TODO(Serge): we should check for proper extensions that we support (when we do lol). I'm thinking we should only really support gltf files
				{
					ModelRawContextMenu(path);
					return true;
				}
			}
			else if (FindStringIC(relativePathString, "sound"))
			{
				if (relativePath.extension().string() == SOUND_BANK_EXT)
				{
					SoundBankContextMenu(path);
					return true;
				}
			}
			else if (FindStringIC(relativePathString, "level"))
			{
				if (relativePath.extension().string() == LEVEL_BIN_EXT)
				{
					LevelBinContextMenu(path);
					return true;
				}
			}
		}


		//@TODO(Serge): we need to figure out based on the path which folder we're in. If it's a directory
		//				then set the menu to be the proper menu for that directory type.
		//				If it's a specific kind of file, then we set the menu to be the proper menu for that file type.


		return false;

	}

	void ContextMenuTitle(const std::string& title)
	{
		ImGui::Text(title.c_str());
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Separator();
		ImGui::NewLine();
	}

	void AssetPanel::MaterialsDirectoryContexMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Materials");

		if (ImGui::Selectable("Make Material", false))
		{
			printf("@TODO(Serge): Make a material\n");
		}

		if (ImGui::Selectable("Make new Directory", false))
		{
			printf("@TODO(Serge): Make a new Directory\n");
		}
	}

	void AssetPanel::LevelsDirectoryContexMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Levels");

		if (ImGui::Selectable("Make New Level", false))
		{
			mnoptrEditor->OpenCreateNewLevelModal();
		}

		if (ImGui::Selectable("Make new Directory", false))
		{
			printf("@TODO(Serge): Make a new Directory\n");
		}
	}

	void AssetPanel::TexturePackDirectoryContexMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Texture Pack");

		if (ImGui::Selectable("Build Texture Pack", false))
		{
			printf("@TODO(Serge): Build texture Pack\n");
		}
	}

	void AssetPanel::MaterialBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Material");

		if (ImGui::Selectable("Import To Level"))
		{
			printf("@TODO(Serge): Import to Current Level\n");
		}
	}

	void AssetPanel::ModelBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Model");

		if (ImGui::Selectable("Import To Level"))
		{
			printf("@TODO(Serge): Import to Current Level\n");
		}
	}

	void AssetPanel::SoundBankContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Sound Bank");

		if (ImGui::Selectable("Import To Level"))
		{
			printf("@TODO(Serge): Import to Current Level\n");
		}
	}

	void AssetPanel::LevelBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Level");

		if (ImGui::Selectable("Open Level"))
		{
		//	printf("@TODO(Serge): Open level: %s\n", path.string().c_str());
			mnoptrEditor->LoadLevel(path.string());
		}
	}

	void AssetPanel::ModelRawContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Model");

		if (ImGui::Selectable("Build & Import To Level"))
		{
			printf("@TODO(Serge): Import to Current Level\n");
		}
	}

	void AssetPanel::MaterialRawContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Material");

		if (ImGui::Selectable("Build & Import to Level"))
		{
			printf("@TODO(Serge): Import to Current Level\n");
		}

	}

}

#endif