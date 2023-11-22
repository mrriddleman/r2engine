#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/EditorMaterialEditor/MaterialEditor.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/File/PathUtils.h"

#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace r2::edit
{

	void MaterialEditor(const r2::mat::MaterialName& materialName, bool& windowOpen)
	{
#ifdef R2_ASSET_PIPELINE

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		r2::asset::MaterialManifestAssetFile* materialManifestFile = (r2::asset::MaterialManifestAssetFile*)r2::asset::lib::GetManifest(assetLib, materialName.packName);

		R2_CHECK(materialManifestFile != nullptr, "Should never be the case");

		std::vector<r2::mat::Material>& materials = materialManifestFile->GetMaterials();

		r2::mat::Material* foundMaterial = nullptr;
		//find the material we care about
		for (u32 i = 0; i < materials.size(); ++i)
		{
			if (materials[i].materialName == materialName)
			{
				foundMaterial = &materials[i];
				break;
			}
		}

		R2_CHECK(foundMaterial != nullptr, "Should always be the case");

		static float CONTENT_WIDTH = 600;
		static float CONTENT_HEIGHT = 1000;

		static std::string s_meshPassStrings[] = { "FORWARD", "TRANSPARENT" };
		static const char* const* s_shaderPropertyTypeStrings = flat::EnumNamesShaderPropertyType();
		static const char* const* s_texturePackingTypeStrings = flat::EnumNamesShaderPropertyPackingType();
		static const char* const* s_minTextureFilterStrings = flat::EnumNamesMinTextureFilter();
		static const char* const* s_magTextureFilterStrings = flat::EnumNamesMagTextureFilter();
		static const char* const* s_textureWrapModeStrings = flat::EnumNamesTextureWrapMode();

		static const std::vector<flat::ShaderPropertyType> s_floatPropertyTypes = {
			flat::ShaderPropertyType_ROUGHNESS,
			flat::ShaderPropertyType_METALLIC,
			flat::ShaderPropertyType_REFLECTANCE,
			flat::ShaderPropertyType_AMBIENT_OCCLUSION,
			flat::ShaderPropertyType_CLEAR_COAT,
			flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS,
			flat::ShaderPropertyType_HEIGHT_SCALE,
			flat::ShaderPropertyType_ANISOTROPY,
			flat::ShaderPropertyType_EMISSION_STRENGTH

		};

		static const std::vector<flat::ShaderPropertyType> s_colorPropertyTypes = {
			flat::ShaderPropertyType_ALBEDO,
			flat::ShaderPropertyType_EMISSION,
			flat::ShaderPropertyType_DETAIL
		};

		static const std::vector<flat::ShaderPropertyType> s_texturePropertyTypes = {
			flat::ShaderPropertyType_ALBEDO,
			flat::ShaderPropertyType_NORMAL,
			flat::ShaderPropertyType_EMISSION,
			flat::ShaderPropertyType_METALLIC,
			flat::ShaderPropertyType_ROUGHNESS,
			flat::ShaderPropertyType_AMBIENT_OCCLUSION,
			flat::ShaderPropertyType_HEIGHT,
			flat::ShaderPropertyType_ANISOTROPY,
			flat::ShaderPropertyType_DETAIL,

			flat::ShaderPropertyType_CLEAR_COAT,
			flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS,
			flat::ShaderPropertyType_CLEAR_COAT_NORMAL

			//@TODO(Serge): we have more property types but I don't think they are supported atm
			//eg. SHEEN_COLOR, SHEEN_ROUGHNESS, BENT_NORMAL, ANISOTROPY_DIRECTION
		};

		static const std::vector<flat::ShaderPropertyType> s_boolPropertyTypes = {
			flat::ShaderPropertyType_DOUBLE_SIDED
		};

		ImGui::SetNextWindowContentSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));

		bool hasMaterialChanged = false;

		if (ImGui::Begin("Material Editor", &windowOpen))
		{
			//We can't change the material name after the fact right now since certain objects (Level files, models etc) hold on to
			//their material names for later. Only when we make a new material can we modify the name
			//we would somehow have to know everyone who has the material name and change theirs.
			//Or we would have to change how MaterialName works in that everyone only have a reference to it - too much work for now
			//The other way to solve it is generate a GUID for each material made (and pack probably) and use that as the reference - not the name itself (https://github.com/graeme-hill/crossguid)
			std::string materialNameLabel = std::string("Material Name: ") + foundMaterial->stringName;
			ImVec2 size = ImGui::GetContentRegionAvail();

			ImGui::Text(materialNameLabel.c_str());
			ImGui::PushItemWidth(CONTENT_WIDTH / 3);

			ImGui::SameLine(size.x - (CONTENT_WIDTH / 3));

			if (ImGui::Button("Save"))
			{
				materialManifestFile->SaveManifest();
			}

			ImGui::PopItemWidth();

			int transparencyType = foundMaterial->transparencyType;

			ImGui::Text("Transparency:");

			if (ImGui::RadioButton("Opaque", &transparencyType, flat::eTransparencyType_OPAQUE))
			{
				hasMaterialChanged = true;
			}
			if (ImGui::RadioButton("Transparent", &transparencyType, flat::eTransparencyType_TRANSPARENT))
			{
				hasMaterialChanged = true;
			}

			foundMaterial->transparencyType = static_cast<flat::eTransparencyType>(transparencyType);

			auto& meshPasses = foundMaterial->shaderEffectPasses.meshPasses;

			for (u32 i = flat::eMeshPass_FORWARD; i <= flat::eMeshPass_TRANSPARENT; ++i) //only doing the ones we support
			{
				ImGui::PushID(i);
				auto& shaderEffect = meshPasses[i];

				std::string shaderEffectTitle = std::string("Shader Effect: ") + s_meshPassStrings[i];

				ImGui::Text(shaderEffectTitle.c_str());
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 2.0f);
				if (ImGui::BeginCombo("##label shaderName", shaderEffect.assetNameString.c_str()))
				{
					//@TODO(Serge): list all the shader effects we have - not sure how we're going to do that atm
					ImGui::EndCombo();
				}

				ImGui::PopItemWidth();

				ImGui::PopID();
			}

			//@TODO(Serge): implement shader params
			//for now just implement the ones we care about - float params, color params, bool params and texture params

			if (ImGui::CollapsingHeader("Float Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.floatParams.size(); ++i)
				{
					auto& floatParam = foundMaterial->shaderParams.floatParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[floatParam.propertyType];

					std::string floatShaderParamsID = "floatshaderparam:" + std::to_string(i);
					ImGui::PushID(floatShaderParamsID.c_str());
					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);

					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						open = false;
					}
					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);
						ImGui::SameLine();


						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string floatInputValueLabel = std::string("##label floatinputvalue") + std::string(propertyType);
						if (ImGui::InputFloat(floatInputValueLabel.c_str(), &floatParam.value))
						{
							hasMaterialChanged = true;
						}

						ImGui::TreePop();
					}

					ImGui::PopID();


				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.floatParams.erase(foundMaterial->shaderParams.floatParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableFloatProperties;

				for (u32 i = 0; i < s_floatPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.floatParams.begin(), foundMaterial->shaderParams.floatParams.end(), [i](const r2::draw::ShaderFloatParam& floatParam)
						{
							return floatParam.propertyType == s_floatPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.floatParams.end())
					{
						availableFloatProperties.push_back(s_floatPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType floatPropertyTypeToAdd = flat::ShaderPropertyType_ALBEDO;

				std::string availableFloatPropertyTypePreview = "";

				if (availableFloatProperties.size() > 0)
				{
					floatPropertyTypeToAdd = availableFloatProperties[0];
					availableFloatPropertyTypePreview = s_shaderPropertyTypeStrings[floatPropertyTypeToAdd];
				}

				ImGui::Text("Add New Float Property: ");
				ImGui::SameLine();

				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availablefloatpropertyTypes", availableFloatPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableFloatProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableFloatProperties[i]], floatPropertyTypeToAdd == availableFloatProperties[i]))
						{
							floatPropertyTypeToAdd = availableFloatProperties[i];

						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewFloatProperty = availableFloatProperties.empty() || floatPropertyTypeToAdd == flat::ShaderPropertyType_ALBEDO;
				if (disableAddNewFloatProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Float Property"))
				{
					r2::draw::ShaderFloatParam newFloatParam;
					newFloatParam.propertyType = floatPropertyTypeToAdd;
					newFloatParam.value = 0.0f;
					foundMaterial->shaderParams.floatParams.push_back(newFloatParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewFloatProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}

			//Color Params
			if (ImGui::CollapsingHeader("Color Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				auto size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.colorParams.size(); ++i)
				{
					auto& colorParam = foundMaterial->shaderParams.colorParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[colorParam.propertyType];
					std::string colorShaderParamsID = "colorshaderparam:" + std::to_string(i);
					ImGui::PushID(colorShaderParamsID.c_str());

					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);


					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						open = false;
					}
					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string colorInputValueLabel = std::string("##label colorinputvalue") + std::string(propertyType);

						if (ImGui::ColorEdit4(colorInputValueLabel.c_str(), glm::value_ptr(colorParam.value)))
						{
							hasMaterialChanged = true;
						}

						ImGui::TreePop();
					}

					ImGui::PopID();

				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.colorParams.erase(foundMaterial->shaderParams.colorParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableColorProperties;

				for (u32 i = 0; i < s_colorPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.colorParams.begin(), foundMaterial->shaderParams.colorParams.end(), [i](const r2::draw::ShaderColorParam& colorParam)
						{
							return colorParam.propertyType == s_colorPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.colorParams.end())
					{
						availableColorProperties.push_back(s_colorPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType colorPropertyTypeToAdd = flat::ShaderPropertyType_ROUGHNESS;

				std::string availableColorPropertyTypePreview = "";

				if (availableColorProperties.size() > 0)
				{
					colorPropertyTypeToAdd = availableColorProperties[0];
					availableColorPropertyTypePreview = s_shaderPropertyTypeStrings[colorPropertyTypeToAdd];
				}

				ImGui::Text("Add New Color Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availablecolorpropertyTypes", availableColorPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableColorProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableColorProperties[i]], colorPropertyTypeToAdd == availableColorProperties[i]))
						{
							colorPropertyTypeToAdd = availableColorProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewColorProperty = availableColorProperties.empty() || colorPropertyTypeToAdd == flat::ShaderPropertyType_ROUGHNESS;
				if (disableAddNewColorProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Color Property"))
				{
					r2::draw::ShaderColorParam newColorParam;
					newColorParam.propertyType = colorPropertyTypeToAdd;
					newColorParam.value = glm::vec4(0, 0, 0, 1);
					foundMaterial->shaderParams.colorParams.push_back(newColorParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewColorProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}

			if (ImGui::CollapsingHeader("Texture Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				char textureName[r2::fs::FILE_PATH_LENGTH];
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.textureParams.size(); ++i)
				{

					const std::string I_STRING = std::to_string(i);

					auto& textureParam = foundMaterial->shaderParams.textureParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[textureParam.propertyType];
					std::string textureShaderParamsID = "textureshaderparam:" + I_STRING;
					ImGui::PushID(textureShaderParamsID.c_str());

					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);


					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						open = false;
					}
					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string textureInputValueLabel = std::string("##label textureinputvalue") + std::string(propertyType);

						//@NOTE(Serge): for now we don't want to list all our textures, just show what the texture is
						//@TODO(Serge): be able to modify this later
						r2::asset::AssetHandle textureAssetHandle = { textureParam.value, gameAssetManager.GetAssetCacheSlot() };
						const r2::asset::AssetFile* assetFile = gameAssetManager.GetAssetFile(textureAssetHandle);
						if (assetFile)
						{
							const char* textureAssetFilePath = assetFile->FilePath();

							r2::fs::utils::CopyFileNameWithParentDirectories(textureAssetFilePath, textureName, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE));
						}
						else
						{
							r2::util::PathCpy(textureName, "");
						}

						ImGui::Text(textureName);

						//if (ImGui::BeginCombo(textureInputValueLabel.c_str(), "Texture Name here"))
						//{
						//	
						//	//@TODO(Serge): how do we fill this?
						//	ImGui::EndCombo();
						//}

						//@NOTE(Serge): this should change with the texture you set above
						std::string texturePackName = std::string("Texture Pack: ") + textureParam.texturePackNameString;
						ImGui::Text(texturePackName.c_str());

						ImGui::Text("Packing Type: ");
						ImGui::SameLine();
						std::string packingTypeLabel = std::string("##label packingType") + I_STRING;
						if (ImGui::BeginCombo(packingTypeLabel.c_str(), s_texturePackingTypeStrings[textureParam.packingType]))
						{
							for (int pt = flat::ShaderPropertyPackingType_R; pt <= flat::ShaderPropertyPackingType_RGBA; pt++)
							{
								if (ImGui::Selectable(s_texturePackingTypeStrings[pt], pt == textureParam.packingType))
								{
									textureParam.packingType = static_cast<flat::ShaderPropertyPackingType>(pt);
									hasMaterialChanged = true;
								}
							}

							ImGui::EndCombo();
						}

						//min filtering
						ImGui::Text("Min Filter: ");
						ImGui::SameLine();
						std::string minFilterLabel = std::string("##label minfilter") + I_STRING;
						if (ImGui::BeginCombo(minFilterLabel.c_str(), s_minTextureFilterStrings[textureParam.minFilter]))
						{
							for (int mf = flat::MinTextureFilter_LINEAR; mf <= flat::MinTextureFilter_LINEAR_MIPMAP_LINEAR; ++mf)
							{
								if (ImGui::Selectable(s_minTextureFilterStrings[mf], mf == textureParam.minFilter))
								{
									textureParam.minFilter = static_cast<flat::MinTextureFilter>(mf);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Mag Filter: ");
						ImGui::SameLine();
						std::string magFilterLabel = std::string("##label magfilter") + I_STRING;
						if (ImGui::BeginCombo(magFilterLabel.c_str(), s_magTextureFilterStrings[textureParam.magFilter]))
						{
							for (int mf = flat::MagTextureFilter_LINEAR; mf <= flat::MagTextureFilter_NEAREST; ++mf)
							{
								if (ImGui::Selectable(s_magTextureFilterStrings[mf], mf == textureParam.magFilter))
								{
									textureParam.magFilter = static_cast<flat::MagTextureFilter>(mf);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Anisotropy: ");
						ImGui::SameLine();
						std::string anisotropyFilterLabel = std::string("##label anisotropyfiltering") + I_STRING;
						if (ImGui::DragFloat(anisotropyFilterLabel.c_str(), &textureParam.anisotropicFiltering, 1.0, 0.0, 16.0)) //@TODO(Serge): replace 16 with the max amount for the system
						{
							//@TODO(Serge): Update the render material somehow
							hasMaterialChanged = true;
						}

						//wrap modes

						ImGui::Text("Wrap S: ");
						ImGui::SameLine();
						std::string wrapSLabel = std::string("##label wrapS") + I_STRING;
						if (ImGui::BeginCombo(wrapSLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapS]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapS))
								{
									textureParam.wrapS = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Wrap T: ");
						ImGui::SameLine();
						std::string wrapTLabel = std::string("##label wrapT") + I_STRING;
						if (ImGui::BeginCombo(wrapTLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapT]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapT))
								{
									textureParam.wrapT = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Wrap R: ");
						ImGui::SameLine();
						std::string wrapRLabel = std::string("##label wrapR") + I_STRING;
						if (ImGui::BeginCombo(wrapRLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapR]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapR))
								{
									textureParam.wrapR = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::TreePop();
					}


					ImGui::PopID();
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.textureParams.erase(foundMaterial->shaderParams.textureParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableTextureProperties;

				for (u32 i = 0; i < s_texturePropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.textureParams.begin(), foundMaterial->shaderParams.textureParams.end(), [i](const r2::draw::ShaderTextureParam& textureParam)
						{
							return textureParam.propertyType == s_texturePropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.textureParams.end())
					{
						availableTextureProperties.push_back(s_texturePropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType texturePropertyTypeToAdd = flat::ShaderPropertyType_REFLECTANCE;

				std::string availableTexturePropertyTypePreview = "";

				if (availableTextureProperties.size() > 0)
				{
					texturePropertyTypeToAdd = availableTextureProperties[0];
					availableTexturePropertyTypePreview = s_shaderPropertyTypeStrings[texturePropertyTypeToAdd];
				}


				ImGui::Text("Add New Texture Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availabletexturepropertyTypes", availableTexturePropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableTextureProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableTextureProperties[i]], texturePropertyTypeToAdd == availableTextureProperties[i]))
						{
							texturePropertyTypeToAdd = availableTextureProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewTextureProperty = availableTextureProperties.empty() || texturePropertyTypeToAdd == flat::ShaderPropertyType_REFLECTANCE;
				if (disableAddNewTextureProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Texture Property"))
				{
					r2::draw::ShaderTextureParam newTextureParam;
					newTextureParam.propertyType = texturePropertyTypeToAdd;
					newTextureParam.value = STRING_ID("");
					newTextureParam.texturePackName = newTextureParam.value;
					newTextureParam.texturePackNameString = "";
					newTextureParam.packingType = flat::ShaderPropertyPackingType_RGBA;
					newTextureParam.minFilter = flat::MinTextureFilter_LINEAR;
					newTextureParam.magFilter = flat::MagTextureFilter_LINEAR;
					newTextureParam.anisotropicFiltering = 0.0f;
					newTextureParam.wrapS = flat::TextureWrapMode_REPEAT;
					newTextureParam.wrapT = flat::TextureWrapMode_REPEAT;
					newTextureParam.wrapR = flat::TextureWrapMode_REPEAT;

					foundMaterial->shaderParams.textureParams.push_back(newTextureParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewTextureProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}

			//add bool param for double sided
			if (ImGui::CollapsingHeader("Bool Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.boolParams.size(); ++i)
				{
					auto& boolParam = foundMaterial->shaderParams.boolParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[boolParam.propertyType];

					std::string boolShaderParamsID = "boolshaderparam:" + std::to_string(i);
					ImGui::PushID(boolShaderParamsID.c_str());
					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);

					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						open = false;
					}

					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						if (ImGui::Checkbox(propertyType, &boolParam.value))
						{
							hasMaterialChanged = true;
						}
						ImGui::TreePop();
					}

					ImGui::PopID();
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.boolParams.erase(foundMaterial->shaderParams.boolParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableBoolProperties;

				for (u32 i = 0; i < s_boolPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.boolParams.begin(), foundMaterial->shaderParams.boolParams.end(), [i](const r2::draw::ShaderBoolParam& boolParam)
						{
							return boolParam.propertyType == s_boolPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.boolParams.end())
					{
						availableBoolProperties.push_back(s_boolPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType boolPropertyTypeToAdd = flat::ShaderPropertyType_ROUGHNESS;

				std::string availableBoolPropertyTypePreview = "";

				if (availableBoolProperties.size() > 0)
				{
					boolPropertyTypeToAdd = availableBoolProperties[0];
					availableBoolPropertyTypePreview = s_shaderPropertyTypeStrings[boolPropertyTypeToAdd];
				}

				ImGui::Text("Add New Boolean Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availableboolpropertyTypes", availableBoolPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableBoolProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableBoolProperties[i]], boolPropertyTypeToAdd == availableBoolProperties[i]))
						{
							boolPropertyTypeToAdd = availableBoolProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewBoolProperty = availableBoolProperties.empty() || boolPropertyTypeToAdd == flat::ShaderPropertyType_ROUGHNESS;
				if (disableAddNewBoolProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Boolean Property"))
				{
					r2::draw::ShaderBoolParam newBoolParam;
					newBoolParam.propertyType = boolPropertyTypeToAdd;
					newBoolParam.value = false;
					foundMaterial->shaderParams.boolParams.push_back(newBoolParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewBoolProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}



			ImGui::End();
		}

		if (hasMaterialChanged)
		{
			flatbuffers::FlatBufferBuilder builder;
			const flat::Material* flatMaterial = r2::mat::MakeFlatMaterialFromMaterial(builder, *foundMaterial);

			r2::SArray<r2::draw::tex::Texture>* textures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, 200);
			r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, r2::draw::tex::Cubemap);

			r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
			R2_CHECK(renderMaterialCache != nullptr, "This should never be nullptr");

			bool isLoaded = r2::draw::rmat::IsMaterialLoadedOnGPU(*renderMaterialCache, materialName.name);

			bool result = gameAssetManager.GetTexturesForFlatMaterial(flatMaterial, textures, cubemaps);
			R2_CHECK(result, "This should always work");

			r2::draw::tex::CubemapTexture* cubemapTextureToUse = nullptr;
			r2::SArray<r2::draw::tex::Texture>* texturesToUse = nullptr;

			if (r2::sarr::Size(*textures) > 0)
			{
				texturesToUse = textures;
			}

			if (r2::sarr::Size(*cubemaps) > 0)
			{
				cubemapTextureToUse = &r2::sarr::At(*cubemaps, 0);
			}

			if (isLoaded)
			{
				gameAssetManager.LoadMaterialTextures(flatMaterial); //@NOTE(Serge): this actually does nothing at the moment since this pack is probably loaded already
				result = r2::draw::rmat::UploadMaterialTextureParams(*renderMaterialCache, flatMaterial, texturesToUse, cubemapTextureToUse, true);
				R2_CHECK(result, "This should always work");
			}

			FREE(cubemaps, *MEM_ENG_SCRATCH_PTR);
			FREE(textures, *MEM_ENG_SCRATCH_PTR);
		}

#endif
	}

}

#endif