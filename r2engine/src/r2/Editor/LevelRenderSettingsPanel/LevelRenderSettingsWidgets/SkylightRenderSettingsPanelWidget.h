#ifndef __SKYLIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#define __SKYLIGHT_RENDER_SETTINGS_PANEL_WIDGET_H__
#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::edit
{
	class SkylightRenderSettingsPanelWidget : public LevelRenderSettingsDataSource
	{
	public:

		SkylightRenderSettingsPanelWidget(r2::Editor* noptrEditor);
		~SkylightRenderSettingsPanelWidget();
		void Init() override;

		void Update() override;

		void Render() override;

		bool OnEvent(r2::evt::Event& e) override;

		void Shutdown() override;
	private:
		void PopulateSkylightMaterials();


		std::vector<r2::asset::AssetName> mConvolvedMaterialNames;
		std::vector<r2::asset::AssetName> mPrefilteredMaterialNames;
		std::vector<u32> mPrefilteredMipLevels;
		std::vector<r2::asset::AssetName> mLUTDFGMaterialNames;

		r2::draw::tex::TextureAddress convolvedTextureAddress;
		r2::draw::tex::TextureAddress prefilteredTextureAddress;
		r2::draw::tex::TextureAddress lutDFGTextureAddress;
		u32 mNumMips;
	};
}


#endif
#endif