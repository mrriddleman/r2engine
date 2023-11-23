#ifndef __TEXTURE_PACK_PANEL_H__
#define __TEXTURE_PACK_PANEL_H__

#ifdef R2_EDITOR

#include <filesystem>

namespace r2::edit
{
	void TexturePackPanel(bool& windowOpen, const std::filesystem::path& path);
}

#endif

#endif