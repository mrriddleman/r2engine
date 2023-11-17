#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorAssetPanel.h"
#include "imgui.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Backends/SDL_OpenGL/ImGuiImageHelpers.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::edit
{



	AssetPanel::AssetPanel()
		: mEditorFolderImageWidth(0)
		, mEditorFolderImageHeight(0)
		, mEditorFolderImage(0)
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

		std::filesystem::path editorFolderPath = std::filesystem::path(R2_ENGINE_ASSET_PATH) / "editor";

		mEditorFolderImage = CreateTextureFromFile((editorFolderPath / "DirectoryIcon.png").string(), mEditorFolderImageWidth, mEditorFolderImageHeight, r2::draw::tex::WRAP_MODE_CLAMP_TO_EDGE, r2::draw::tex::FILTER_LINEAR, r2::draw::tex::FILTER_LINEAR);
		IM_ASSERT(mEditorFolderImage != 0);

		mEditorFileImage = CreateTextureFromFile((editorFolderPath / "FileIcon.png").string(), mEditorFileImageWidth, mEditorFileImageHeight, r2::draw::tex::WRAP_MODE_CLAMP_TO_EDGE, r2::draw::tex::FILTER_LINEAR, r2::draw::tex::FILTER_LINEAR);
		IM_ASSERT(mEditorFileImage != 0);

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

		static const int NUM_COLUMNS = 2;
		static float w = 300.0f;

		bool open = true;

		if (ImGui::Begin("AssetPanel", &open))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			ImGui::BeginChild("Asset FileSystem", ImVec2(w, 0), true);
			ImGui::Text("FileSystem");
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
					ShowContextMenuForPath(mCurrentDirectory);

					ImGui::EndPopup();
				}
				if (ImGui::IsWindowHovered())
					ImGui::SetTooltip("Right-click to open popup");
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
		}

		ImGui::ShowDemoWindow();
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

		if (mCurrentDirectory != "" && mCurrentBaseDirectory != "")
		{
			ImGui::Columns(columnCount, 0, false);

			for (auto& directoryEntry : std::filesystem::directory_iterator(mCurrentDirectory))
			{
				const auto& path = directoryEntry.path();
				std::string filenameString = path.filename().string();

				ImGui::PushID(filenameString.c_str());
				unsigned int icon = directoryEntry.is_directory() ? mEditorFolderImage : mEditorFileImage;
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton((ImTextureID)icon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

				if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
				{
					ShowContextMenuForPath(directoryEntry.path());

					ImGui::EndPopup();
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Right-click to open popup");

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

			//ImGui::Text(mCurrentDirectory.string().c_str());
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

	void AssetPanel::ShowContextMenuForPath(const std::filesystem::path& path)
	{
		ImGui::Text("%s", path.string().c_str());
	}
}

#endif