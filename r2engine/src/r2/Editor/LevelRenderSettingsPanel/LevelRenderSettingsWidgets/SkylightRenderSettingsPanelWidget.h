#ifndef __SKYLIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#define __SKYLIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"

namespace r2::edit
{
	class SkylightRenderSettingsPanelWidget : public LevelRenderSettingsDataSource
	{
	public:

		SkylightRenderSettingsPanelWidget();
		~SkylightRenderSettingsPanelWidget();

		void Update() override;

		void Render() override;

		bool OnEvent(r2::evt::Event& e) override;

		void Shutdown() override;
	private:

	};
}


#endif
#endif