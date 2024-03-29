#ifndef __COLOR_GRADING_RENDER_SETTINGS_PANEL_WIDGET_H__
#define __COLOR_GRADING_RENDER_SETTINGS_PANEL_WIDGET_H__
#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace flat
{
	struct MaterialPack;
}

namespace r2::edit
{
	class ColorGradingRenderSettingsPanelWidget : public LevelRenderSettingsDataSource
	{
	public:

		ColorGradingRenderSettingsPanelWidget(r2::Editor* noptrEditor);
		~ColorGradingRenderSettingsPanelWidget();
		void Init() override;

		void Update() override;

		void Render() override;

		bool OnEvent(r2::evt::Event& e) override;

		void Shutdown() override;
	private:
		void PopulateColorGradingMaterials();


		struct MaterialNameAndPack
		{
			r2::asset::AssetName assetName;
			const flat::MaterialPack* materialPack;
		};

		r2::asset::AssetName mCurrentColorGradingMaterialName;
		std::vector<MaterialNameAndPack> mColorGradingMaterials;

	};
}


#endif
#endif