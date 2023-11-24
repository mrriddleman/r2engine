#include "r2pch.h"

#if defined( R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Editor/EditorTexturePackPanel/TexturePackPanel.h"
#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#include <ctype.h>

#include "imgui.h"
namespace r2::edit
{
	static const char* const* s_TextureTypeNames = flat::EnumNamesTextureType();
	static const char* const* s_MipMapFilterTypeNames = flat::EnumNamesMipMapFilter();
	static const char* const* s_CubeMapTypeNames = flat::EnumNamesCubemapSide();

	void TexturePackPanel(bool& windowOpen, const std::filesystem::path& path, r2::asset::pln::tex::TexturePackMetaFile& metaFile)
	{
		const u32 CONTENT_WIDTH = 500;
		const u32 CONTENT_HEIGHT = 600;

		ImGui::SetNextWindowSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowContentSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));

		std::filesystem::path metaJSONPath = path / "meta.json";
		bool metaJSONExists = std::filesystem::exists(metaJSONPath);

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
		
		//look through the albedo directory (if it exists) and see if we have files
		if(metaFile.mipLevels.empty() && !metaJSONExists)
		{
			static std::vector<std::string> cubemapSideNames = { "_right", "_left", "_top", "_bottom", "_front", "_back" };
			static std::unordered_map<std::string, flat::CubemapSide> cubemapSideMap = { 
				{"_right", flat::CubemapSide_RIGHT},
				{"_left", flat::CubemapSide_LEFT}, 
				{"_top", flat::CubemapSide_TOP},
				{"_bottom", flat::CubemapSide_BOTTOM},
				{"_front", flat::CubemapSide_FRONT},
				{"_back", flat::CubemapSide_BACK},

			};
			std::filesystem::path albedoDir = path / "albedo";
			if (std::filesystem::exists(albedoDir))
			{
				for (auto& directoryEntry : std::filesystem::directory_iterator(albedoDir))
				{
					bool isCubemapSide = false;
					u32 level = 0;
					flat::CubemapSide cubemapSide;
					std::string directoryEntryString = directoryEntry.path().filename().string();
					for (u32 i = 0; i < cubemapSideNames.size(); ++i)
					{
						size_t position = directoryEntryString.find(cubemapSideNames[i]);
						if (position != std::string::npos && directoryEntryString[position+cubemapSideNames[i].size()] == '.')
						{
							isCubemapSide = true;
							//this is a cubemap side
							cubemapSide = cubemapSideMap[cubemapSideNames[i]];
							//@TODO(Serge): figure out the mip level
							for (s32 j = position; j >= 0; --j)
							{
								if (std::isdigit(directoryEntryString[j]))
								{
									int end = j;
									for (s32 k = j - 1; k >= 0; --k)
									{
										if (!std::isdigit(directoryEntryString[k]))
										{
											end = k + 1;
											break;
										}
									}

									std::string levelStr = directoryEntryString.substr(end, j - end + 1);
									
									level = std::atoi(levelStr.c_str());
								}
							}
							break;
						}
					}

					//
					if (isCubemapSide)
					{
						std::filesystem::path textureName = directoryEntryString;
						textureName.replace_extension(".rtex");

						r2::asset::pln::tex::TexturePackMetaFileMipLevel* mipLevel = nullptr;
						//find the mip level
						for (u32 i = 0; i < metaFile.mipLevels.size(); ++i)
						{
							if (metaFile.mipLevels[i].level == level)
							{
								mipLevel = &metaFile.mipLevels[i];
								break;
							}
						}

						if (mipLevel == nullptr)
						{
							metaFile.mipLevels.push_back(
								{ 
									level, 
									{
										{"", flat::CubemapSide_RIGHT},
										{"", flat::CubemapSide_LEFT},
										{"", flat::CubemapSide_TOP},
										{"", flat::CubemapSide_BOTTOM},
										{"", flat::CubemapSide_FRONT},
										{"", flat::CubemapSide_BACK}
									}
								}
							);

							mipLevel = &metaFile.mipLevels.back();
						}

						R2_CHECK(mipLevel != nullptr, "Should never happen");

						mipLevel->sides[cubemapSide].textureName = textureName.string();
					}
				}
			}
		}


		if (ImGui::Begin("Texture Pack Import Editor", &windowOpen))
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