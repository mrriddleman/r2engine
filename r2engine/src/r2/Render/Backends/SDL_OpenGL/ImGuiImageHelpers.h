#ifndef __IMGUI_IMAGE_HELPERS_H__
#define __IMGUI_IMAGE_HELPERS_H__

#if defined( R2_EDITOR) && defined(R2_IMGUI)

namespace r2::edit 
{
	unsigned int CreateTextureFromFile(const std::string& imagePath, int& width, int& height, unsigned int wrapMode, unsigned int magFilter, unsigned int minFilter);
}

#endif

#endif