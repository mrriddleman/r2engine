
#ifdef R2_EDITOR
#ifndef __EDITOR_LEVEL_RENDER_SETTINGS_PANEL_H__
#define __EDITOR_LEVEL_RENDER_SETTINGS_PANEL_H__

#include "r2/Editor/EditorWidget.h"
#include "r2/Utils/Utils.h"

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"

namespace r2::edit
{
	class LevelRenderSettingsPanel : public EditorWidget
	{
	public:
		LevelRenderSettingsPanel();
		~LevelRenderSettingsPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;

		void RegisterLevelRenderSettingsWidget(std::unique_ptr<LevelRenderSettingsDataSource> renderSettingsWidget);

		Editor* GetEditor();

	private:
		Editor* mnoptrEditor;
		std::vector<std::unique_ptr<LevelRenderSettingsDataSource>> mDataSources;
	};
}


#endif
#endif
