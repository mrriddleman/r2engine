#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/ColorGradingRenderSettingsPanelWidget.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Core/Engine.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"
#include "R2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Render/Model/Materials/MaterialHelpers.h"

#include "imgui.h"
namespace r2::edit 
{
	
	ColorGradingRenderSettingsPanelWidget::ColorGradingRenderSettingsPanelWidget(r2::Editor* noptrEditor)
		:LevelRenderSettingsDataSource(noptrEditor, "Color Grading Settings")
	{

	}

	ColorGradingRenderSettingsPanelWidget::~ColorGradingRenderSettingsPanelWidget()
	{

	}

	void ColorGradingRenderSettingsPanelWidget::Init()
	{
		PopulateColorGradingMaterials();
	}

	void ColorGradingRenderSettingsPanelWidget::Update()
	{

	}

	void ColorGradingRenderSettingsPanelWidget::Render()
	{
		r2::draw::TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();

		std::string previewString = "None";

		if (mCurrentColorGradingMaterialName.hashID != r2::asset::INVALID_ASSET_HANDLE)
		{
			previewString = mCurrentColorGradingMaterialName.assetNameString;
		}

		bool needsUpdate = false;
		
		bool isColorGradingEnabled = r2::draw::renderer::IsColorGradingEnabled();
		if (ImGui::Checkbox("Enable Color Grading: ", &isColorGradingEnabled))
		{
			r2::draw::renderer::EnableColorGrading(isColorGradingEnabled);
		
		}

		ImGui::Text("Color Grading LUTs: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label colorgradingluts", previewString.c_str()))
		{
			for (u32 i = 0; i < mColorGradingMaterials.size(); ++i)
			{
				if (ImGui::Selectable(mColorGradingMaterials[i].assetName.assetNameString.c_str(), previewString == mColorGradingMaterials[i].assetName.assetNameString))
				{
					mCurrentColorGradingMaterialName = mColorGradingMaterials[i].assetName;
					
					const auto* textureName = r2::draw::texche::GetAlbedoTextureForMaterialName(texturePacksCache, mColorGradingMaterials[i].materialPack, mColorGradingMaterials[i].assetName.hashID);

					r2::draw::renderer::SetColorGradingLUT(textureName, 32);

					break;
				}
			}

			ImGui::EndCombo();
		}

		float colorGradingContribution = r2::draw::renderer::GetColorGradingContribution();

		ImGui::Text("Contribution: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label contribution", &colorGradingContribution, 0.01, 0.0, 1.0))
		{
			r2::draw::renderer::SetColorGradingContribution(colorGradingContribution);
		}

	}

	bool ColorGradingRenderSettingsPanelWidget::OnEvent(r2::evt::Event& e)
	{
		return false;
	}

	void ColorGradingRenderSettingsPanelWidget::Shutdown()
	{

	}

	void ColorGradingRenderSettingsPanelWidget::PopulateColorGradingMaterials()
	{
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::draw::TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();


		u32 numMaterialManifests = r2::asset::lib::GetManifestCountForType(assetLib, asset::MATERIAL_PACK_MANIFEST);

		r2::SArray<r2::asset::ManifestAssetFile*>* materialManifestAssetFiles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::ManifestAssetFile*, numMaterialManifests);
		r2::asset::lib::GetManifestFilesForType(assetLib, asset::MATERIAL_PACK_MANIFEST, materialManifestAssetFiles);

		r2::SArray<const flat::MaterialPack*>* materialPacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::MaterialPack*, numMaterialManifests);

		struct MaterialPackOffset
		{
			u32 start = 0;
			u32 num = 0;
		};

		r2::SArray< MaterialPackOffset>* materialPackOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, MaterialPackOffset, numMaterialManifests);

		u32 numConvolvedCubemaps = 0;
		u32 numPrefilteredCubemaps = 0;
		u32 numTextures = 0;
		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const r2::asset::MaterialManifestAssetFile* materialManifestAssetFile = reinterpret_cast<r2::asset::MaterialManifestAssetFile*>(r2::sarr::At(*materialManifestAssetFiles, i));
			const auto* materialPack = materialManifestAssetFile->GetMaterialPack();
			r2::sarr::Push(*materialPacks, materialPack);

			MaterialPackOffset nextOffset;
			nextOffset.start = numTextures;

			numTextures += r2::draw::texche::NumTexturesInMaterialPack(texturePacksCache, materialPack, flat::TextureProcessType_LUT);

			nextOffset.num = numTextures - nextOffset.start;

			r2::sarr::Push(*materialPackOffsets, nextOffset);
		}

		r2::SArray<const flat::Material*>* textureMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numTextures);



		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const flat::MaterialPack* materialPack = r2::sarr::At(*materialPacks, i);
			r2::draw::texche::GetAllTextureMaterialsInMaterialPack(texturePacksCache, materialPack, textureMaterials, flat::TextureProcessType_LUT);
		}

		for (u32 i = 0; i < numTextures; ++i)
		{
			const flat::Material* lutMaterial = r2::sarr::At(*textureMaterials, i);

			r2::asset::AssetName assetName;
			r2::asset::MakeAssetNameFromFlatAssetName(lutMaterial->assetName(), assetName);

			MaterialNameAndPack temp;
			temp.assetName = assetName;
			temp.materialPack = nullptr;
			for (u32 p = 0; p < numMaterialManifests; ++p)
			{
				const MaterialPackOffset offset = r2::sarr::At(*materialPackOffsets, p);

				if (i >= offset.start && i < offset.start + offset.num)
				{
					temp.materialPack = r2::sarr::At(*materialPacks, p);
					break;
				}
			}

			R2_CHECK(temp.materialPack != nullptr, "Should never happen");

			if (IsInLevelMaterialList(assetName))
			{
				mColorGradingMaterials.push_back(temp);
			}
			
		}

		FREE(textureMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPackOffsets, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPacks, *MEM_ENG_SCRATCH_PTR);
		FREE(materialManifestAssetFiles, *MEM_ENG_SCRATCH_PTR);
	}

}

#endif