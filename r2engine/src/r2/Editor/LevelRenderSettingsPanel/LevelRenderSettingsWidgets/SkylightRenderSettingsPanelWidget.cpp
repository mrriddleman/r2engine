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
#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"
#include "imgui.h"

namespace r2::edit
{

	SkylightRenderSettingsPanelWidget::SkylightRenderSettingsPanelWidget()
		:LevelRenderSettingsDataSource("Skylight Settings")
	{

	}

	SkylightRenderSettingsPanelWidget::~SkylightRenderSettingsPanelWidget()
	{

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

		//@TODO(Serge): performance of this is atrocious right now - do this in a smarter way. We could just do this once then wait for an event or something
		//				and repopulate it

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::draw::TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();
		r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();

		u32 numMaterialManifests = r2::asset::lib::GetManifestCountForType(assetLib, asset::MATERIAL_PACK_MANIFEST);

		r2::SArray<r2::asset::ManifestAssetFile*>* materialManifestAssetFiles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::ManifestAssetFile*, numMaterialManifests);
		r2::asset::lib::GetManifestFilesForType(assetLib, asset::MATERIAL_PACK_MANIFEST, materialManifestAssetFiles);

		r2::SArray<const flat::MaterialPack*>* materialPacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::MaterialPack*, numMaterialManifests);

		u32 numCubemaps = 0;
		u32 numTextures = 0;
		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const r2::asset::MaterialManifestAssetFile* materialManifestAssetFile = reinterpret_cast<r2::asset::MaterialManifestAssetFile*>(r2::sarr::At(*materialManifestAssetFiles, i));
			const auto* materialPack = materialManifestAssetFile->GetMaterialPack();
			r2::sarr::Push(*materialPacks, materialPack);

			numCubemaps += r2::draw::texche::NumCubemapTexturesInMaterialPack(texturePacksCache, materialPack);
			numTextures += r2::draw::texche::NumTexturesInMaterialPack(texturePacksCache, materialPack);
		}

		r2::SArray< const flat::Material*>* cubemapMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numCubemaps);
		r2::SArray<const flat::Material*>* textureMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::Material*, numTextures);

		for (u32 i = 0; i < numMaterialManifests; ++i)
		{
			const flat::MaterialPack* materialPack = r2::sarr::At(*materialPacks, i);
			r2::draw::texche::GetAllCubemapMaterialsAndTexturesInMaterialPack(texturePacksCache, materialPack, cubemapMaterials);
			r2::draw::texche::GetAllTextureMaterialsInMaterialPack(texturePacksCache, materialPack, textureMaterials);
		}

		if (skylightPtr)
		{
			//Get the information of the skylight
			
			for (u32 i= 0; i < numCubemaps; ++i)
			{
				const auto* cubemapMaterial = r2::sarr::At(*cubemapMaterials, i);

				const auto* cubemapRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, cubemapMaterial->assetName()->assetName());

				if (r2::draw::tex::AreTextureAddressEqual(skylightPtr->diffuseIrradianceTexture, cubemapRenderMaterial->cubemap.texture))
				{
					diffuseIrradianceTexturePreviewString = cubemapMaterial->assetName()->stringName()->str();
				}
				else if (r2::draw::tex::AreTextureAddressEqual(skylightPtr->prefilteredRoughnessTexture, cubemapRenderMaterial->cubemap.texture))
				{
					prefilteredRoughnessTexturePreviewString = cubemapMaterial->assetName()->stringName()->str();
				}
				
			}

			for (u32 i = 0; i < numTextures; ++i)
			{
				const auto* textureMaterial = r2::sarr::At(*textureMaterials, i);

				const auto* textureRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, textureMaterial->assetName()->assetName());

				if (r2::draw::tex::AreTextureAddressEqual(skylightPtr->lutDFGTexture, textureRenderMaterial->albedo.texture))
				{
					lutDFGTexturePreviewString = textureMaterial->assetName()->stringName()->str();
				}
			}
		}
		


		ImGui::Text("Diffuse Irradiance: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label diffuerirradiance", diffuseIrradianceTexturePreviewString.c_str()))
		{



			ImGui::EndCombo();
		}

		ImGui::Text("Prefiltered: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label prefilteredroughness", prefilteredRoughnessTexturePreviewString.c_str()))
		{
			ImGui::EndCombo();
		}

		ImGui::Text("LUT DFG: ");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##label lutDFG", lutDFGTexturePreviewString.c_str()))
		{
			ImGui::EndCombo();
		}


		if (!r2::draw::renderer::HasSkylight())
		{
			if (ImGui::Button("Add Skylight"))
			{

			}
		}
		else
		{
			if (ImGui::Button("Remove Skylight"))
			{

			}
		}

		FREE(textureMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(cubemapMaterials, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPacks, *MEM_ENG_SCRATCH_PTR);
		FREE(materialManifestAssetFiles, *MEM_ENG_SCRATCH_PTR);
	}

	bool SkylightRenderSettingsPanelWidget::OnEvent(r2::evt::Event& e)
	{
		return false; //Whether it should consume the event or not
	}

	void SkylightRenderSettingsPanelWidget::Shutdown()
	{

	}



}

#endif

