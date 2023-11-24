#include "r2pch.h"

#if defined( R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Editor/EditorTexturePackPanel/TexturePackPanel.h"
#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"


#include "imgui.h"
namespace r2::edit
{
	static const char* const* s_TextureTypeNames = flat::EnumNamesTextureType();
	static const char* const* s_MipMapFilterTypeNames = flat::EnumNamesMipMapFilter();
	static const char* const* s_CubeMapTypeNames = flat::EnumNamesCubemapSide();

	void TexturePackPanel(bool& windowOpen, const std::filesystem::path& path)
	{
		const u32 CONTENT_WIDTH = 500;
		const u32 CONTENT_HEIGHT = 600;

		ImGui::SetNextWindowSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowContentSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));

		std::filesystem::path metaJSONPath = path / "meta.json";
		bool metaJSONExists = std::filesystem::exists(metaJSONPath);

		asset::pln::tex::TexturePackMetaFile metaFile;
		//@TODO(Serge): set default values
		metaFile.type = flat::TextureType_TEXTURE;
		metaFile.desiredMipLevels = 1;
		metaFile.filter = flat::MipMapFilter_BOX;

		if (metaJSONExists)
		{
			//load it and populate the metaFile struct
			std::string schemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

			std::filesystem::path texturePackMetaSchemaPath = std::filesystem::path(schemaPath) / "TexturePackMetaData.fbs";

			std::string jsonText = "";
			std::string texturePackMetaSchemaFile = "";

			bool result = flatbuffers::LoadFile(texturePackMetaSchemaPath.string().c_str(), false, &texturePackMetaSchemaFile) &&
				flatbuffers::LoadFile(metaJSONPath.string().c_str(), false, &jsonText);

			R2_CHECK(result, "Should never happen");
			flatbuffers::Parser parser;

			result = parser.Parse(texturePackMetaSchemaFile.c_str()) && parser.Parse(jsonText.c_str());
			R2_CHECK(result, "Should never happen");
 
			const flat::TexturePackMetaData* texturePackMetaData = flat::GetTexturePackMetaData(parser.builder_.GetBufferPointer());

			R2_CHECK(texturePackMetaData != nullptr, "Should never happen");
			
			//now fill the metaFile
			r2::asset::pln::tex::MakeTexturePackMetaFileFromFlat(texturePackMetaData, metaFile);
		}

		if (ImGui::Begin("Texture Pack Editor", &windowOpen))
		{
			ImGui::Text("Pack Name: %s", path.stem().string().c_str());
			ImGui::SameLine();

			if (ImGui::Button("Build"))
			{
				r2::asset::pln::tex::GenerateTexturePackMetaJSONFile(path, metaFile);
				if (metaJSONExists)
				{
					//@TODO(Serge): build it again
				}
			}

			//right now we don't want the ability to modify this if it exists already - more for future proofing
			if (metaJSONExists)
			{
				ImGui::BeginDisabled(true);
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
			}

			int textureType = metaFile.type;
			ImGui::Text("Type: ");
			ImGui::RadioButton(s_TextureTypeNames[0], &textureType, flat::TextureType_TEXTURE);
			ImGui::RadioButton(s_TextureTypeNames[1], &textureType, flat::TextureType_CUBEMAP);

			metaFile.type = static_cast<flat::TextureType>( textureType );

			if (metaJSONExists)
			{
				ImGui::PopStyleVar();
				ImGui::EndDisabled();
			}

			if (metaFile.type == flat::TextureType_TEXTURE )
			{
				ImGui::BeginDisabled(true);
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
			}

			ImGui::Text("Desired Mip Levels: ");
			ImGui::SameLine();
			ImGui::InputInt("##label desiredmiplevels", &metaFile.desiredMipLevels);

			ImGui::Text("Mip Map Filter: ");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##label mipmapfilter", s_MipMapFilterTypeNames[metaFile.filter]))
			{
				for (s32 i = flat::MipMapFilter_BOX; i <= flat::MipMapFilter_KAISER; ++i)
				{
					if (ImGui::Selectable(s_MipMapFilterTypeNames[i], i == metaFile.filter))
					{
						metaFile.filter = static_cast<flat::MipMapFilter>( i );
						break;
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::CollapsingHeader("Mip Levels"))
			{
				for (u32 i = 0; i < metaFile.mipLevels.size(); ++i)
				{
					const auto& mipLevel = metaFile.mipLevels[i];

					std::string levelString = std::string("Level: ") + std::to_string(mipLevel.level);

					if (ImGui::TreeNode(levelString.c_str()))
					{
						for (u32 j = 0; j < mipLevel.sides.size(); ++j)
						{
							std::string sideString = std::string("Side: ") + s_CubeMapTypeNames[mipLevel.sides[j].cubemapSide];

							if (ImGui::TreeNode(sideString.c_str()))
							{
								ImGui::Text("Texture Name: %s", mipLevel.sides[j].textureName.c_str());

								ImGui::TreePop();
							}
						}

						ImGui::TreePop();
					}
				}
			}

			if (metaFile.type == flat::TextureType_TEXTURE )
			{
				ImGui::PopStyleVar();
				ImGui::EndDisabled();
			}

			ImGui::End();
		}
	}

}

#endif