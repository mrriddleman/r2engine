#if defined( R2_EDITOR ) && defined R2_IMGUI
#ifndef __EDITOR_ASSET_PANEL_H__
#define __EDITOR_ASSET_PANEL_H__

#include "r2/Editor/EditorWidget.h"
#include <filesystem>
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"

namespace r2::edit
{
	class AssetPanel : public EditorWidget
	{
	public:
		AssetPanel();
		~AssetPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;
	private:


		void FileSystemPanel();
		void AssetsBrowserPanel(int contentWidth);
		void ShowDirectoryInFileSystemPanel(const std::filesystem::path& directory, bool& wasActivated);


		bool ShowContextMenuForPath(const std::filesystem::path& path, bool wasButtonItem);

		void MaterialsDirectoryContexMenu(const std::filesystem::path& path);
		void LevelsDirectoryContexMenu(const std::filesystem::path& path);
		void TexturePackDirectoryContexMenu(const std::filesystem::path& path);

		void MaterialBinContextMenu(const std::filesystem::path& path);
		void ModelBinContextMenu(const std::filesystem::path& path);
		void SoundBankContextMenu(const std::filesystem::path& path);
		void LevelBinContextMenu(const std::filesystem::path& path);

		void MaterialRawContextMenu(const std::filesystem::path& path);
		void ModelRawContextMenu(const std::filesystem::path& path);
		size_t GetNumberOfSelectedDirectories();

		std::filesystem::path FindRawAssetPathFromBinaryAsset(const std::filesystem::path& path);

		std::filesystem::path FindBinAssetPathFromRawAsset(const std::filesystem::path& path);


		std::filesystem::path mEngineRawDirectory;
		std::filesystem::path mEngineBinDirectory;
		std::filesystem::path mAppRawDirectory;
		std::filesystem::path mAppBinDirectory;


		std::filesystem::path mCurrentDirectory;
		std::filesystem::path mCurrentBaseDirectory;

		bool mMaterialEditorWindowOpen;
		bool mTexturePackWindowOpen;
		std::filesystem::path mTexturePackPanelDirectory;

		asset::pln::tex::TexturePackMetaFile mMetaFile;

		std::unordered_map<std::string, bool> mCurrentSelectedItemsInDirectory;
	
	};
}


#endif
#endif // R2_EDITOR
