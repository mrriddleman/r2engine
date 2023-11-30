#include "r2pch.h"
#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorAssetBrowser/EditorAssetPanel.h"

#include "r2/Core/Assets/AssetLib.h"
#include "assetlib/RModel_generated.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/File/PathUtils.h"

#include "r2/Render/Model/Materials/Material_generated.h"
#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorMaterialEditor/MaterialEditor.h"
#include "r2/Editor/EditorTexturePackPanel/TexturePackPanel.h"
#include "r2/Platform/Platform.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetReference.h"
#include "r2/Core/Assets/Pipeline/AssetConverterUtils.h"
#include "r2/Core/Application.h"
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
		:mMaterialEditorWindowOpen(false)
		,mTexturePackWindowOpen(false)
		, mTexturePackPanelDirectory("")

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
		
		if (mMaterialEditorWindowOpen)
		{
			CreateNewMaterial(mMaterialEditorWindowOpen);
		}
		
		if (mTexturePackWindowOpen)
		{
			TexturePackPanel(mTexturePackWindowOpen, mTexturePackPanelDirectory, mMetaFile);
		}

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
		ImGui::PushID(directory.string().c_str());
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
		ImGui::PopID();
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
				else if (relativePath.extension().string() == JSON_EXT)
				{
					MaterialRawContextMenu(path);
					return true;
				}

			}
			else if (FindStringIC(relativePathString, "model"))
			{
				std::string extension = relativePath.extension().string();

				if (extension == RMODEL_BIN_EXT)
				{
					ModelBinContextMenu(path);
					return true;
				}
				else if(extension == ".fbx" || extension == ".gltf") //@TODO(Serge): we should check for proper extensions that we support (when we do lol). I'm thinking we should only really support gltf files
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
			mMaterialEditorWindowOpen = true;
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

		//first check to see if we have a meta.json file
		std::filesystem::path metaJSONPath = path / "meta.json";

		std::string selectableTitle = "Build Texture Pack";
		if (std::filesystem::exists(metaJSONPath))
		{
			selectableTitle = "Edit Texture Pack";
		}

		if (ImGui::Selectable(selectableTitle.c_str(), false))
		{
			mTexturePackWindowOpen = true;
			mTexturePackPanelDirectory = path;


			mMetaFile.type = flat::TextureType_TEXTURE;
			mMetaFile.desiredMipLevels = 1;
			mMetaFile.filter = flat::MipMapFilter_BOX;
			mMetaFile.mipLevels.clear();
		
		}
		//@TODO(Serge): delete when we can actually do that correctly
	}

	void AssetPanel::MaterialBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Material");

		if (ImGui::Selectable("Import To Level"))
		{
			r2::mat::MaterialName materialName = r2::mat::GetMaterialNameForPath(path.stem().string());

			R2_CHECK(!materialName.IsInavlid(), "Should never happen");

			mnoptrEditor->AddMaterialToLevel(materialName);
		}
	}



	void AssetPanel::ModelBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Model");

		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		char filePath[r2::fs::FILE_PATH_LENGTH];
		r2::util::PathCpy(filePath, path.string().c_str());
		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(filePath, sanitizedPath);

		bool isImportedToGame = r2::asset::lib::HasAsset(assetLib, sanitizedPath, r2::asset::RMODEL);
		std::string labelString = "Import To Game";

		if (isImportedToGame)
		{
			labelString = "Import To Level";
		}

		if (ImGui::Selectable(labelString.c_str()))
		{
			if (isImportedToGame)
			{
				auto modelAsset = r2::asset::Asset::MakeAssetFromFilePath(sanitizedPath, r2::asset::RMODEL);
				mnoptrEditor->AddModelToLevel(modelAsset);
			}
			else
			{
				//add the path to assetLib
				auto rawAssetPath = FindRawAssetPathFromBinaryAsset(path);
				if (rawAssetPath != "")
				{
					r2::asset::lib::ImportAsset(assetLib,  r2::asset::CreateNewAssetReference(path, rawAssetPath, r2::asset::RMODEL), r2::asset::RMODEL);
				}
				
			}
		}
	}

	

	void AssetPanel::ModelRawContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Model");

		if (ImGui::Selectable("Build Model"))
		{
			std::filesystem::path preferredPath = path;
			preferredPath = preferredPath.make_preferred();

			auto binAssetPath = FindBinAssetPathFromRawAsset(path);

			std::filesystem::path materialManifestPath = "";

			if (std::filesystem::relative(binAssetPath, mAppBinDirectory) != "")
			{
				//get the app's material manifest
				materialManifestPath = MENG.GetApplication().GetMaterialPacksManifestsBinaryPaths().at(0);
			}
			else
			{
				//else get r2's
				char materialsPath[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN, materialsPath, "engine_material_pack.mpak");

				materialManifestPath = materialsPath;
			}

			materialManifestPath = materialManifestPath.make_preferred();


			//@TODO(Serge): put in the hotreload pipeline for threading
			int result = r2::asset::pln::assetconvert::RunModelConverter(preferredPath.string(), binAssetPath.make_preferred().parent_path().string(), materialManifestPath.string());

			R2_CHECK(result == 0, "?");
		}
	}

	void AssetPanel::SoundBankContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Sound Bank");

		if (ImGui::Selectable("Import To Level"))
		{

			//@TODO(Serge): should the level/editor do this?
			r2::audio::AudioEngine audioEngine;
			audioEngine.LoadBank(path.string().c_str(), 0);


			mnoptrEditor->AddSoundBankToLevel(r2::asset::GetAssetNameForFilePath(path.string().c_str(), r2::asset::SOUND));
		}
	}

	void AssetPanel::LevelBinContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Level");

		if (ImGui::Selectable("Open Level"))
		{
			mnoptrEditor->LoadLevel(path.string());
		}
	}

	void AssetPanel::MaterialRawContextMenu(const std::filesystem::path& path)
	{
		ContextMenuTitle("Material");

		if (ImGui::Selectable("Build & Import to Level"))
		{
			//Technically, if the material exists, it should have already been built (I THINK!)
			r2::mat::MaterialName materialName = r2::mat::GetMaterialNameForPath(path.stem().string());

			R2_CHECK(!materialName.IsInavlid(), "Should never happen");

			mnoptrEditor->AddMaterialToLevel(materialName);
		}

	}


	std::filesystem::path AssetPanel::FindRawAssetPathFromBinaryAsset(const std::filesystem::path& path)
	{
		//@NOTE(Serge): this whole method relies on the fact that we mirror our data in assets_bin to assets
		//				probably will only work for models and textures

		if (!std::filesystem::exists(path))
			return "";

		//now check each binary base against the path
		//if the path is a non-empty relative path then use the other
		std::filesystem::path pathToUse = path;
		pathToUse = pathToUse.make_preferred();

		std::filesystem::path result = "";

		std::filesystem::path stem = "";
		std::filesystem::path parentDirectory = "";
		if ((result = std::filesystem::relative(pathToUse, mAppBinDirectory)) != "")
		{
			stem = result.stem();
			parentDirectory = mAppRawDirectory / result.parent_path();
		}
		else if ((result = std::filesystem::relative(pathToUse, mEngineBinDirectory)) != "")
		{
			//@NOTE(Serge): this case should be basically non-existent since we shouldn't ever be
			//				importing stuff into the engine. But, we're going to provide it just in case
			stem = result.stem();
			parentDirectory = mEngineRawDirectory / result.parent_path();
		}

		if (!std::filesystem::exists(parentDirectory))
		{
			R2_CHECK(false, "Maybe just for testing");
			return "";
		}

		for (const auto& entry : std::filesystem::directory_iterator(parentDirectory))
		{
			//look through the parent directory for the stem name
			if (entry.path().stem() == stem)
			{
				return entry.path();
			}
		}

		return "";
	}

	std::filesystem::path AssetPanel::FindBinAssetPathFromRawAsset(const std::filesystem::path& path)
	{
		//@NOTE(Serge): this whole method relies on the fact that we mirror our data in assets_bin to assets
		//				probably will only work for models and textures

		if (!std::filesystem::exists(path))
			return "";

		//now check each binary base against the path
		//if the path is a non-empty relative path then use the other
		std::filesystem::path pathToUse = path;
		pathToUse = pathToUse.make_preferred();

		std::filesystem::path result = "";

		std::filesystem::path stem = "";
		std::filesystem::path parentDirectory = "";
		if ((result = std::filesystem::relative(pathToUse, mAppRawDirectory)) != "")
		{
			stem = result.stem();
			parentDirectory = mAppBinDirectory / result.parent_path();
		}
		else if ((result = std::filesystem::relative(pathToUse, mEngineRawDirectory)) != "")
		{
			//@NOTE(Serge): this case should be basically non-existent since we shouldn't ever be
			//				importing stuff into the engine. But, we're going to provide it just in case
			stem = result.stem();
			parentDirectory = mEngineBinDirectory / result.parent_path();
		}

		if (!std::filesystem::exists(parentDirectory))
		{
			R2_CHECK(false, "Maybe just for testing");
			return "";
		}

		for (const auto& entry : std::filesystem::directory_iterator(parentDirectory))
		{
			//look through the parent directory for the stem name
			if (entry.path().stem() == stem)
			{
				return entry.path();
			}
		}

		return "";
	}
}

#endif