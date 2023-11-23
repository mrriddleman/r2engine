#include "r2pch.h"

#if defined( R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Editor/EditorTexturePackPanel/TexturePackPanel.h"
#include "imgui.h"
namespace r2::edit
{

	void TexturePackPanel(bool& windowOpen, const std::filesystem::path& path)
	{
		const u32 CONTENT_WIDTH = 400;
		const u32 CONTENT_HEIGHT = 600;

		ImGui::SetNextWindowSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));
		ImGui::SetNextWindowContentSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));

		if (ImGui::Begin("Texture Pack Editor", &windowOpen))
		{



			ImGui::End();
		}
	}

}

#endif