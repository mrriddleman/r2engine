#ifdef R2_EDITOR
#ifndef __EDITOR_LEVEL_ASSET_PANEL_H__
#define __EDITOR_LEVEL_ASSET_PANEL_H__

#include "r2/Editor/EditorWidget.h"

namespace r2
{
	class Level;
}

namespace r2::edit
{
	class LevelAssetPanel : public EditorWidget
	{
	public:
		LevelAssetPanel();
		~LevelAssetPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;
	private:
		void ShowLevelFiles(r2::Level& level, bool& wasActivated);
		using LevelAssetBrowseFunc = std::function<void(r2::Level& level, float thumbnailSize)>;

		LevelAssetBrowseFunc mBrowseFunction;
		u32 mFileIcon;
		u32 mFolderIcon;

		void ShowLevelMaterials(r2::Level& level, float thumbnailSize);
		void ShowLevelModels(r2::Level& level, float thumbnailSize);
		void ShowLevelSounds(r2::Level& level, float thumbnailSize);

		enum eLevelAssetContextMenuType
		{
			LACT_MATERIAL = 0,
			LACT_MODEL,
			LACT_SOUND
		};

		struct LevelAssetContextMenuData
		{
			eLevelAssetContextMenuType type;
			const void* data;
		};

		bool ShowLevelAssetContextMenu(r2::Level& level, const LevelAssetContextMenuData& contextMenuData);

	};
}

#endif
#endif