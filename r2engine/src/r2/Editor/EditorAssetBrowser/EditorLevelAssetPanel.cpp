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

#include "r2/Core/Assets/AssetLib.h"

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
		
		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		if (ImGui::TreeNode("Materials"))
		{
			for (u32 i = 0; i < r2::sarr::Size(*materialsNames); ++i)
			{
				const r2::mat::MaterialName& nextMaterialName = r2::sarr::At(*materialsNames, i);

				const flat::Material* material = r2::mat::GetMaterialForMaterialName(nextMaterialName);

				if (ImGui::TreeNodeEx(material->assetName()->stringName()->c_str(), ImGuiTreeNodeFlags_Bullet))
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

				const r2::asset::AssetFile* modelAssetFile = r2::asset::lib::GetAssetFileForAsset(assetLib, r2::asset::Asset(assetHandle.handle, r2::asset::RMODEL));//gameAssetManager.GetAssetFile(assetHandle);

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
			std::string modelFileStemStr = material->assetName()->stringName()->str();
			const char* fileNameString = modelFileStemStr.c_str();
			ImGui::PushID(fileNameString);
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

			if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
			{
				LevelAssetContextMenuData data;
				data.type = LACT_MATERIAL;
				data.data = &nextMaterialName;

				if (!ShowLevelAssetContextMenu(level, data))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}

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

		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		for (u32 i = 0; i < r2::sarr::Size(*modelAssets); ++i)
		{
			r2::asset::AssetHandle assetHandle = r2::sarr::At(*modelAssets, i);

			const r2::asset::AssetFile* modelAssetFile = r2::asset::lib::GetAssetFileForAsset(assetLib, r2::asset::Asset(assetHandle.handle, r2::asset::RMODEL)); //gameAssetManager.GetAssetFile(assetHandle);

			std::filesystem::path modelFilePath = modelAssetFile->FilePath();

			auto modelFileStem = modelFilePath.stem();
			std::string modelFileStemStr = modelFileStem.string();
			const char* fileNameString = modelFileStemStr.c_str();

			ImGui::PushID(fileNameString);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

			if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
			{
				LevelAssetContextMenuData data;
				data.type = LACT_MODEL;
				data.data = &assetHandle;

				if (!ShowLevelAssetContextMenu(level, data))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}

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
			auto soundAsset = r2::sarr::At(*soundAssets, i);
			std::string soundFileStemStr = r2::audio::GetSoundBankNameFromAssetName(soundAsset);
			


			ImGui::PushID(soundFileStemStr.c_str());

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)mFileIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

			if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
			{
				LevelAssetContextMenuData data;
				data.type = LACT_SOUND;
				data.data = &soundAsset;

				if (!ShowLevelAssetContextMenu(level, data))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}

			ImGui::PopStyleColor();

			ImGui::TextWrapped(soundFileStemStr.c_str());

			ImGui::NextColumn();

			ImGui::PopID();
		}

	}

	void LevelContextMenuTitle(const std::string& title)
	{
		ImGui::Text(title.c_str());
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Separator();
		ImGui::NewLine();
	}

	bool LevelAssetPanel::ShowLevelAssetContextMenu(r2::Level& level, const LevelAssetContextMenuData& contextMenuData)
	{
		switch (contextMenuData.type)
		{
		case LACT_MATERIAL:
		{
			LevelContextMenuTitle("Level Material");

			if (ImGui::Selectable("Remove From Level", false))
			{
				printf("@TODO(Serge): Remove From Level\n");
			}
			return true;
		}
		break;
		case LACT_MODEL:
		{
			LevelContextMenuTitle("Level Model");

			if (ImGui::Selectable("Remove From Level", false))
			{
				printf("@TODO(Serge): Remove From Level\n");
			}

			return true;
		}
		break;
		case LACT_SOUND:
		{
			LevelContextMenuTitle("Level Sound");

			if (ImGui::Selectable("Remove From Level", false))
			{
				printf("@TODO(Serge): Remove From Level\n");
			}

			return true;
		}
		break;
		default:
			R2_CHECK(false, "Unsupported Level Asset Context Menu Type!");
			
			break;
		}

		return false;
	}

}

#endif