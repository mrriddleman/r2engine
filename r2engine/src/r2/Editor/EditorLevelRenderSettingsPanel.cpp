#include "r2pch.h"

#if defined(R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Editor/EditorLevelRenderSettingsPanel.h"

//Widgets
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/SkylightRenderSettingsPanelWidget.h"
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/ColorGradingRenderSettingsPanelWidget.h"
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/SunlightRenderSettingsPanelWidget.h"
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/CameraRenderSettingsPanelWidget.h"

#include "imgui.h"

namespace r2::edit
{

	LevelRenderSettingsPanel::LevelRenderSettingsPanel()
		:mnoptrEditor(nullptr)
	{
		mDataSources.clear();
	}

	LevelRenderSettingsPanel::~LevelRenderSettingsPanel()
	{
		mDataSources.clear();
	}

	void LevelRenderSettingsPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

		std::unique_ptr<r2::edit::SkylightRenderSettingsPanelWidget> skylightWidget = std::make_unique<r2::edit::SkylightRenderSettingsPanelWidget>(mnoptrEditor);
		RegisterLevelRenderSettingsWidget(std::move(skylightWidget));

		std::unique_ptr<r2::edit::ColorGradingRenderSettingsPanelWidget> colorGradingWidget = std::make_unique<r2::edit::ColorGradingRenderSettingsPanelWidget>(mnoptrEditor);
		RegisterLevelRenderSettingsWidget(std::move(colorGradingWidget));

		std::unique_ptr<r2::edit::SunLightRenderSettingsPanelWidget> sunlightWidget = std::make_unique<r2::edit::SunLightRenderSettingsPanelWidget>(mnoptrEditor);
		RegisterLevelRenderSettingsWidget(std::move(sunlightWidget));

		std::unique_ptr<r2::edit::CameraRenderSettingsPanelWidget> cameraWidget = std::make_unique<r2::edit::CameraRenderSettingsPanelWidget>(mnoptrEditor);
		RegisterLevelRenderSettingsWidget(std::move(cameraWidget));

	}

	void LevelRenderSettingsPanel::Shutdown()
	{
		for (auto&& widget : mDataSources)
		{
			widget->Shutdown();
		}
	}

	void LevelRenderSettingsPanel::OnEvent(evt::Event& e)
	{
		for (size_t i = 0; i < mDataSources.size(); ++i)
		{
			if (mDataSources[i]->OnEvent(e))
			{
				break;
			}
		}
	}

	void LevelRenderSettingsPanel::Update()
	{
		for (auto&& widget : mDataSources)
		{
			widget->Update();
		}
	}

	void LevelRenderSettingsPanel::Render(u32 dockingSpaceID)
	{
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

		bool open = true;

		if (!ImGui::Begin("Render Settings", &open))
		{
			ImGui::End();
			return;
		}

		for (size_t i = 0; i < mDataSources.size(); i++)
		{
			const auto& widget = mDataSources[i];
			bool widgetOpen = ImGui::CollapsingHeader(widget->GetTitle().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen);
			if (widgetOpen)
			{
				widget->Render();
			}
		}
		ImGui::End();
		
	}

	r2::Editor* LevelRenderSettingsPanel::GetEditor()
	{
		return mnoptrEditor;
	}

	void LevelRenderSettingsPanel::RegisterLevelRenderSettingsWidget(std::unique_ptr<LevelRenderSettingsDataSource> renderSettingsWidget)
	{
		auto componentType = renderSettingsWidget->GetTitle();

		auto iter = std::find_if(mDataSources.begin(), mDataSources.end(), [&](const std::unique_ptr<LevelRenderSettingsDataSource>& widget)
			{
				return widget->GetTitle() == componentType;
			});

		if (iter == mDataSources.end())
		{
			mDataSources.push_back(std::move(renderSettingsWidget));
			mDataSources.back()->Init();
			std::sort(mDataSources.begin(), mDataSources.end(), [](const std::unique_ptr < LevelRenderSettingsDataSource>& w1, const std::unique_ptr < LevelRenderSettingsDataSource>& w2)
				{
					return w1->GetSortOrder() < w2->GetSortOrder();
				});
		}
	}

}


#endif