#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/SkylightRenderSettingsPanelWidget.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"
#include "r2/Core/Engine.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"
#include "R2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"

#include "r2/Game/Level/LevelManager.h"
#include "imgui.h"

#include "r2/Editor/Editor.h"
#include "r2/Game/Level/Level.h"

namespace r2::edit
{

	SkylightRenderSettingsPanelWidget::SkylightRenderSettingsPanelWidget(r2::Editor* noptrEditor)
		:LevelRenderSettingsDataSource(noptrEditor, "Skylight Settings")
		,mNumMips(0)
	{

	}

	SkylightRenderSettingsPanelWidget::~SkylightRenderSettingsPanelWidget()
	{

	}


	void SkylightRenderSettingsPanelWidget::Init()
	{
		mNumMips = 0;
		PopulateSkylightMaterials();
	}

	void SkylightRenderSettingsPanelWidget::Update()
	{

	}

	void SkylightRenderSettingsPanelWidget::Render()
	{
		r2::draw::SkyLight* skylightPtr = r2::draw::renderer::GetSkyLightPtr(r2::draw::renderer::GetCurrentSkylightHandle());
		
		std::string diffuseIrradianceTexturePreviewString = "None";
		std::string prefilteredRoughnessTexturePreviewString = "None";
		std::string lutDFGTexturePreviewString = "None";
		
		r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();

		if (skylightPtr && skylightPtr->diffuseIrradianceTexture.containerHandle != 0)
		{
			//Get the information of the skylight
			convolvedTextureAddress = skylightPtr->diffuseIrradianceTexture;

		}

		if (skylightPtr && skylightPtr->prefilteredRoughnessTexture.containerHandle != 0)
		{
			prefilteredTextureAddress = skylightPtr->prefilteredRoughnessTexture;
			
		}

		if (skylightPtr && skylightPtr->lutDFGTexture.containerHandle != 0)
		{
			lutDFGTextureAddress = skylightPtr->lutDFGTexture;
		}

		if (convolvedTextureAddress.containerHandle != 0)
		{
			for (u32 i = 0; i < mConvolvedMaterialNames.size(); ++i)
			{
				const auto* cubemapRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mConvolvedMaterialNames[i].hashID);

				if (r2::draw::tex::AreTextureAddressEqual(convolvedTextureAddress, cubemapRenderMaterial->cubemap.texture))
				{
					diffuseIrradianceTexturePreviewString = mConvolvedMaterialNames[i].assetNameString;

				}
			}
		}
		
		if (prefilteredTextureAddress.containerHandle != 0)
		{
			for (u32 i = 0; i < mPrefilteredMaterialNames.size(); ++i)
			{
				const auto* cubemapRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mPrefilteredMaterialNames[i].hashID);

				if (r2::draw::tex::AreTextureAddressEqual(prefilteredTextureAddress, cubemapRenderMaterial->cubemap.texture))
				{
					prefilteredRoughnessTexturePreviewString = mPrefilteredMaterialNames[i].assetNameString;

				}
			}
		}
		
		if (lutDFGTextureAddress.containerHandle != 0)
		{
			for (u32 i = 0; i < mLUTDFGMaterialNames.size(); ++i)
			{
				const auto* textureRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mLUTDFGMaterialNames[i].hashID);

				if (r2::draw::tex::AreTextureAddressEqual(lutDFGTextureAddress, textureRenderMaterial->albedo.texture))
				{
					lutDFGTexturePreviewString = mLUTDFGMaterialNames[i].assetNameString;

				}
			}
		}
		
		
		bool needsUpdate = false;
		

		ImGui::Text("Diffuse Irradiance: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label diffuseirradiance", diffuseIrradianceTexturePreviewString.c_str()))
		{
			for (u32 i = 0; i < mConvolvedMaterialNames.size(); ++i)
			{
				if (ImGui::Selectable(mConvolvedMaterialNames[i].assetNameString.c_str(), diffuseIrradianceTexturePreviewString == mConvolvedMaterialNames[i].assetNameString))
				{
					const auto* cubemapRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mConvolvedMaterialNames[i].hashID);

					convolvedTextureAddress = cubemapRenderMaterial->cubemap.texture;

					needsUpdate = true;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Prefiltered: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label prefilteredroughness", prefilteredRoughnessTexturePreviewString.c_str()))
		{

			for (u32 i = 0; i < mPrefilteredMaterialNames.size(); ++i)
			{
				if (ImGui::Selectable(mPrefilteredMaterialNames[i].assetNameString.c_str(), prefilteredRoughnessTexturePreviewString == mPrefilteredMaterialNames[i].assetNameString))
				{
					const auto* cubemapRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mPrefilteredMaterialNames[i].hashID);

					prefilteredTextureAddress = cubemapRenderMaterial->cubemap.texture;
					mNumMips = mPrefilteredMipLevels[i];

					needsUpdate = true;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("LUT DFG: ");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##label lutDFG", lutDFGTexturePreviewString.c_str()))
		{
			for (u32 i = 0; i < mLUTDFGMaterialNames.size(); ++i)
			{
				if (ImGui::Selectable(mLUTDFGMaterialNames[i].assetNameString.c_str(), lutDFGTexturePreviewString == mLUTDFGMaterialNames[i].assetNameString))
				{
					const auto* lutDFGRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, mLUTDFGMaterialNames[i].hashID);

					lutDFGTextureAddress = lutDFGRenderMaterial->albedo.texture;

					needsUpdate = true;
				}
			}

			ImGui::EndCombo();
		}

		if (!r2::draw::renderer::HasSkylight())
		{
			bool disableCondition = lutDFGTextureAddress.containerHandle == 0 || convolvedTextureAddress.containerHandle == 0 || prefilteredTextureAddress.containerHandle == 0 || mNumMips == 0;

			if (disableCondition)
			{
				ImGui::BeginDisabled(true);
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
			}

			if (ImGui::Button("Add Skylight"))
			{
				r2::draw::SkyLight newSkylight;
				newSkylight.diffuseIrradianceTexture = convolvedTextureAddress;
				newSkylight.prefilteredRoughnessTexture = prefilteredTextureAddress;
				newSkylight.lutDFGTexture = lutDFGTextureAddress;

				r2::draw::renderer::AddSkyLight(newSkylight, mNumMips);
			}

			if (disableCondition)
			{
				ImGui::PopStyleVar();
				ImGui::EndDisabled();
			}
		}
		else
		{
			if (ImGui::Button("Remove Skylight"))
			{
				r2::draw::renderer::RemoveSkyLight(r2::draw::renderer::GetCurrentSkylightHandle());
			}
		}

		
	}

	bool SkylightRenderSettingsPanelWidget::OnEvent(r2::evt::Event& e)
	{
		//@TODO(Serge): performance of this is atrocious right now - do this in a smarter way. We could just do this once then wait for an event or something
		//				and repopulate it

		return false; //Whether it should consume the event or not
	}

	void SkylightRenderSettingsPanelWidget::Shutdown()
	{

	}

	void SkylightRenderSettingsPanelWidget::PopulateSkylightMaterials()
	{

		mConvolvedMaterialNames.clear();
		mPrefilteredMaterialNames.clear();
		mPrefilteredMipLevels.clear();
		mLUTDFGMaterialNames.clear();

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::draw::TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();

		u32 numMaterialManifests = r2::asset::lib::GetManifestCountForType(assetLib, asset::MATERIAL_PACK_MANIFEST);

		r2::SArray<r2::asset::ManifestAssetFile*>* materialManifestAssetFiles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::ManifestAssetFile*, numMaterialManifests);
		r2::asset::lib::GetManifestFilesForType(assetLib, asset::MATERIAL_PACK_MANIFEST, materialManifestAssetFiles);

		r2::SArray<const flat::MaterialPack*>* materialPacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::MaterialPack*, numMaterialManifests);

		u32 numConvolvedCubemaps = 0;
		u32 numPrefilteredCubemaps = 0;
		u32 numTextures = 0;
		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const r2::asset::MaterialManifestAssetFile* materialManifestAssetFile = reinterpret_cast<r2::asset::MaterialManifestAssetFile*>(r2::sarr::At(*materialManifestAssetFiles, i));
			const auto* materialPack = materialManifestAssetFile->GetMaterialPack();
			r2::sarr::Push(*materialPacks, materialPack);

			numConvolvedCubemaps += r2::draw::texche::NumCubemapTexturesInMaterialPack(texturePacksCache, materialPack, flat::TextureProcessType_CONVOLVED);
			numPrefilteredCubemaps += r2::draw::texche::NumCubemapTexturesInMaterialPack(texturePacksCache, materialPack, flat::TextureProcessType_PREFILTER);
			numTextures += r2::draw::texche::NumTexturesInMaterialPack(texturePacksCache, materialPack, flat::TextureProcessType_LUT_DFG);
		}

		r2::SArray<const flat::Material*>* convolvedCubemapMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numConvolvedCubemaps);
		r2::SArray<const flat::Material*>* prefilteredCubemapMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numPrefilteredCubemaps);
		r2::SArray<const flat::Material*>* textureMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numTextures);

		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const flat::MaterialPack* materialPack = r2::sarr::At(*materialPacks, i);
			r2::draw::texche::GetAllCubemapMaterialsAndTexturesInMaterialPack(texturePacksCache, materialPack, convolvedCubemapMaterials, flat::TextureProcessType_CONVOLVED);
			r2::draw::texche::GetAllCubemapMaterialsAndTexturesInMaterialPack(texturePacksCache, materialPack, prefilteredCubemapMaterials, flat::TextureProcessType_PREFILTER);
			r2::draw::texche::GetAllTextureMaterialsInMaterialPack(texturePacksCache, materialPack, textureMaterials, flat::TextureProcessType_LUT_DFG);
		}
		
		for (u32 i = 0; i < numConvolvedCubemaps; ++i)
		{
			const flat::Material* convolvedMaterial = r2::sarr::At(*convolvedCubemapMaterials, i);


			r2::asset::AssetName assetName;
			r2::asset::MakeAssetNameFromFlatAssetName(convolvedMaterial->assetName(), assetName);

			
			if (IsInLevelMaterialList(assetName))
			{
				mConvolvedMaterialNames.push_back(assetName);
			}

			
		}

		for (u32 i = 0; i < numPrefilteredCubemaps; ++i)
		{
			const flat::Material* prefilteredMaterial = r2::sarr::At(*prefilteredCubemapMaterials, i);

			const auto* textureParams = prefilteredMaterial->shaderParams()->textureParams();
			const auto numTextureParams = textureParams->size();
			u64 texturePackName = 0;
			for (u32 j = 0; j < numTextureParams; ++j)
			{
				if (textureParams->Get(j)->propertyType() == flat::ShaderPropertyType_ALBEDO)
				{
					texturePackName = textureParams->Get(j)->texturePack()->assetName();
					break;
				}
			}

			const auto* prefilteredCubemap = r2::draw::texche::GetCubemapTextureForTexturePack(texturePacksCache, texturePackName);

			mPrefilteredMipLevels.push_back(prefilteredCubemap->numMipLevels);

			r2::asset::AssetName assetName;
			r2::asset::MakeAssetNameFromFlatAssetName(prefilteredMaterial->assetName(), assetName);
			if (IsInLevelMaterialList(assetName))
			{
				mPrefilteredMaterialNames.push_back(assetName);
			}
		}

		for (u32 i = 0; i < numTextures; ++i)
		{
			const flat::Material* lutDFGMaterial = r2::sarr::At(*textureMaterials, i);

			r2::asset::AssetName assetName;
			r2::asset::MakeAssetNameFromFlatAssetName(lutDFGMaterial->assetName(), assetName);

			if (IsInLevelMaterialList(assetName))
			{
				mLUTDFGMaterialNames.push_back(assetName);
			}
		}

		FREE(textureMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(prefilteredCubemapMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(convolvedCubemapMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPacks, *MEM_ENG_SCRATCH_PTR);
		FREE(materialManifestAssetFiles, *MEM_ENG_SCRATCH_PTR);
	}


}

#endif

