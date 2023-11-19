#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorAssetBrowser/EditorLevelAssetPanel.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/Level/Level.h"

#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Render/Model/Materials/Material_generated.h"

#include "r2/Audio/SoundDefinitionHelpers.h"

#include "r2/Platform/Platform.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

#include "imgui.h"

namespace r2::edit
{

	LevelAssetPanel::LevelAssetPanel()
		:mBrowseFunction(nullptr)
	{

	}

	LevelAssetPanel::~LevelAssetPanel()
	{

	}

	void LevelAssetPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

		mFileIcon = mnoptrEditor->GetEditorFileImage();
		mFolderIcon = mnoptrEditor->GetEditorFolderImage();

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

		r2::Level& currentLevel = mnoptrEditor->GetEditorLevelRef();




		static float w = 300.0f;
		bool wasActivated = false;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::BeginChild("Level Assets", ImVec2(w, 0), true);
		ImGui::Text("Assets");
		ImGui::Separator();
		ImGui::Separator();
		//Level assets file browser

		ShowLevelFiles(currentLevel, wasActivated);

		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::InvisibleButton("vsplitter", ImVec2(10.0f, ImGui::GetContentRegionAvail().y));
		if (ImGui::IsItemActive())
		{
			w += ImGui::GetIO().MouseDelta.x;
		}
		ImGui::SameLine();



		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x - w;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::BeginChild("Asset Preview", ImVec2(0, 0), true);

		//if (mCurrentDirectory != "")
		//{
		//	if (ImGui::BeginPopupContextWindow("Asset Preview", ImGuiPopupFlags_MouseButtonRight))
		//	{
		//		if (!ShowContextMenuForPath(mCurrentDirectory, false))
		//			ImGui::CloseCurrentPopup();

		//		ImGui::EndPopup();
		//	}
		//	//if (ImGui::IsWindowHovered())
		//	//	ImGui::SetTooltip("Right-click to open popup");
		//}

		//if (mCurrentDirectory != "" && mCurrentBaseDirectory != "")
		//{
		//	if (mCurrentDirectory != std::filesystem::path(mCurrentBaseDirectory))
		//	{
		//		if (ImGui::Button("<-"))
		//		{
		//			mCurrentDirectory = mCurrentDirectory.parent_path();
		//		}
		//	}
		//	ImGui::SameLine();
		//}

		ImGui::Text("Asset Preview");
		ImGui::Separator();
		ImGui::Separator();

		if (mBrowseFunction)
		{
			ImGui::Columns(columnCount, 0, false);
			
			mBrowseFunction(currentLevel, thumbnailSize);

			ImGui::Columns(1);
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::End();
	}

	void LevelAssetPanel::ShowLevelFiles(r2::Level& level, bool& wasActivated)
	{
		r2::SArray<r2::mat::MaterialName>* materialsNames = level.GetMaterials();
		r2::SArray<r2::asset::AssetHandle>* modelAssets = level.GetModelAssets();
		r2::SArray<u64>* soundAssets = level.GetSoundBankAssetNames();

		r2::GameAssetManager& gameAssetManager = MENG.GetGameAssetManager();
		
		if (ImGui::TreeNode("Materials"))
		{
			for (u32 i = 0; i < r2::sarr::Size(*materialsNames); ++i)
			{
				const r2::mat::MaterialName& nextMaterialName = r2::sarr::At(*materialsNames, i);

				const flat::Material* material = r2::mat::GetMaterialForMaterialName(nextMaterialName);

				if (ImGui::TreeNodeEx(material->stringName()->c_str(), ImGuiTreeNodeFlags_Bullet))
				{
					if (!wasActivated && (ImGui::IsItemActivated() || ImGui::IsItemActive()))
					{
						mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelMaterials, this, std::placeholders::_1, std::placeholders::_2);
						wasActivated = true;
					}

					ImGui::TreePop();
				}
				
			}

			ImGui::TreePop();
		}

		if (!wasActivated && ImGui::IsItemClicked())
		{
			mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelMaterials, this, std::placeholders::_1, std::placeholders::_2);
			wasActivated = true;
		}

		if (ImGui::TreeNode("Models"))
		{
			for (u32 i = 0; i < r2::sarr::Size(*modelAssets); ++i)
			{
				r2::asset::AssetHandle assetHandle = r2::sarr::At(*modelAssets, i);

				const r2::asset::AssetFile* modelAssetFile = gameAssetManager.GetAssetFile(assetHandle);

				std::filesystem::path modelFilePath = modelAssetFile->FilePath();

				if (ImGui::TreeNodeEx(modelFilePath.stem().string().c_str(), ImGuiTreeNodeFlags_Bullet))
				{
					if (!wasActivated && (ImGui::IsItemActivated() || ImGui::IsItemActive()))
					{
						mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelModels, this, std::placeholders::_1, std::placeholders::_2);
						wasActivated = true;
					}

					ImGui::TreePop();
				}
				
			}
			ImGui::TreePop();
		}
		if (!wasActivated && ImGui::IsItemClicked())
		{
			mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelModels, this, std::placeholders::_1, std::placeholders::_2);
			wasActivated = true;
		}

		if (ImGui::TreeNode("Sounds"))
		{
			for (u32 i = 0; i < r2::sarr::Size(*soundAssets); ++i)
			{
				const char* soundBankName = r2::audio::GetSoundBankNameFromAssetName(r2::sarr::At(*soundAssets, i));

				if (ImGui::TreeNodeEx(soundBankName, ImGuiTreeNodeFlags_Bullet))
				{
					if (!wasActivated && (ImGui::IsItemActivated() || ImGui::IsItemActive()))
					{
						mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelSounds, this, std::placeholders::_1, std::placeholders::_2);
						wasActivated = true;
					}

					ImGui::TreePop();
				}
				
			}

			ImGui::TreePop();
		}

		if (!wasActivated && ImGui::IsItemClicked())
		{
			mBrowseFunction = std::bind(&LevelAssetPanel::ShowLevelSounds, this, std::placeholders::_1, std::placeholders::_2);
			wasActivated = true;
		}
	}

	void LevelAssetPanel::ShowLevelMaterials(r2::Level& level, float thumbnailSize)
	{
		r2::SArray<r2::mat::MaterialName>* materialsNames = level.GetMaterials();

		for (u32 i = 0; i < r2::sarr::Size(*materialsNames); ++i)
		{
			const r2::mat::MaterialName& nextMaterialName = r2::sarr::At(*materialsNames, i);

			const flat::Material* material = r2::mat::GetMaterialForMaterialName(nextMaterialName);
			std::string modelFileStemStr = material->stringName()->str();
			const char* fileNameString = modelFileStemStr.c_str();
			ImGui::PushID(fileNameString);
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
			ImGui::PopStyleColor();

			ImGui::TextWrapped(fileNameString);

			ImGui::NextColumn();

			ImGui::PopID();
		}
	}

	void LevelAssetPanel::ShowLevelModels(r2::Level& level, float thumbnailSize)
	{
		r2::SArray<r2::asset::AssetHandle>* modelAssets = level.GetModelAssets();
		r2::GameAssetManager& gameAssetManager = MENG.GetGameAssetManager();

		for (u32 i = 0; i < r2::sarr::Size(*modelAssets); ++i)
		{
			r2::asset::AssetHandle assetHandle = r2::sarr::At(*modelAssets, i);

			const r2::asset::AssetFile* modelAssetFile = gameAssetManager.GetAssetFile(assetHandle);

			std::filesystem::path modelFilePath = modelAssetFile->FilePath();

			auto modelFileStem = modelFilePath.stem();
			std::string modelFileStemStr = modelFileStem.string();
			const char* fileNameString = modelFileStemStr.c_str();

			ImGui::PushID(fileNameString);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
			ImGui::PopStyleColor();

			ImGui::TextWrapped(fileNameString);

			ImGui::NextColumn();

			ImGui::PopID();
		}
	}

	void LevelAssetPanel::ShowLevelSounds(r2::Level& level, float thumbnailSize)
	{
		r2::SArray<u64>* soundAssets = level.GetSoundBankAssetNames();

		for (u32 i = 0; i < r2::sarr::Size(*soundAssets); ++i)
		{
			const char* fileNameString = r2::audio::GetSoundBankNameFromAssetName(r2::sarr::At(*soundAssets, i));

			ImGui::PushID(fileNameString);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
			ImGui::PopStyleColor();

			ImGui::TextWrapped(fileNameString);

			ImGui::NextColumn();

			ImGui::PopID();
		}

	}

}

#endif