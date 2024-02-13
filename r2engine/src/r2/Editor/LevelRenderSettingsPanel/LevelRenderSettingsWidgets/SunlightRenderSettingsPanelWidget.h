#ifndef __SUN_LIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#define __SUN_LIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"
#include "r2/Render/Model/Light.h"

namespace r2::edit
{
	class SunLightRenderSettingsPanelWidget : public LevelRenderSettingsDataSource
	{
	public:

		SunLightRenderSettingsPanelWidget(r2::Editor* noptrEditor);
		~SunLightRenderSettingsPanelWidget();
		void Init() override;

		void Update() override;

		void Render() override;

		bool OnEvent(r2::evt::Event& e) override;

		void Shutdown() override;
	private:
		
		r2::draw::DirectionLightHandle mDirectionLightHandle;
		r2::draw::LightProperties mDirectionLightLightProperties;
		glm::vec3 mDirection;
		glm::vec3 mPosition; //always should be fixed - just for imguizmo
		u32 mImGuizmoOperation;
	};
}


#endif
#endif