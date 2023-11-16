#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorAssetPanel.h"
#include "imgui.h"
#include "r2/Core/File/PathUtils.h"


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

		mEngineRawDirectory = R2_ENGINE_ASSET_PATH;
		mEngineBinDirectory = R2_ENGINE_ASSET_BIN_PATH;

		char filePath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ROOT, "assets", filePath);
		mAppRawDirectory = filePath;
		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ROOT, "assets_bin", filePath);
		mAppBinDirectory = filePath;

		mCurrentDirectory = "";
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
			ImGui::Text("Assets Browser");
			ImGui::Separator();
			ImGui::Separator();
			AssetsBrowserPanel();
			ImGui::EndChild();

			ImGui::PopStyleVar();
			ImGui::End();
		}

//		ImGui::ShowDemoWindow();
	}

	void AssetPanel::FileSystemPanel()
	{
		bool wasActivated = false;

		//@TODO(Serge): figure out if we need all four of these directories. Would be nice to just work with 2 of them...
		if (ImGui::CollapsingHeader("Engine Assets Raw"))
		{
			ShowDirectoryInFileSystemPanel(mEngineRawDirectory, wasActivated);
		}

		if (ImGui::CollapsingHeader("Game Assets Raw"))
		{
			ShowDirectoryInFileSystemPanel(mAppRawDirectory, wasActivated);
		}

		if (ImGui::CollapsingHeader("Engine Assets Bin"))
		{
			ShowDirectoryInFileSystemPanel(mEngineBinDirectory, wasActivated);
		}

		if (ImGui::CollapsingHeader("Game Assets Bin"))
		{
			ShowDirectoryInFileSystemPanel(mAppBinDirectory, wasActivated);
		}
	}

	void AssetPanel::AssetsBrowserPanel()
	{
		if (mCurrentDirectory != "")
		{
			ImGui::Text(mCurrentDirectory.string().c_str());
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
}

#endif