#ifndef __CAMERA_RENDER_SETTINGS_PANEL_WIDGET_H__
#define __CAMERA_RENDER_SETTINGS_PANEL_WIDGET_H__

#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"

namespace r2::edit 
{
	class CameraRenderSettingsPanelWidget : public LevelRenderSettingsDataSource
	{
	public:

		CameraRenderSettingsPanelWidget(r2::Editor* noptrEditor);

		~CameraRenderSettingsPanelWidget();

		void Init() override;

		void Update() override;

		void Render() override;

		bool OnEvent(r2::evt::Event& e) override;

		void Shutdown() override;
	private:
		char newCameraText[2048];
		
	};
}


#endif


#endif