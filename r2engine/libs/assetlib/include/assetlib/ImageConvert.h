#pragma once

#ifndef __ASSET_LIB_IMAGE_CONVERT_H__
#define __ASSET_LIB_IMAGE_CONVERT_H__

#include "assetlib/TextureMetaData_generated.h"
#include <filesystem>


namespace flat
{
	enum MipMapFilter : int;
}



namespace r2::assets::assetlib
{
	bool ConvertImage(const std::filesystem::path& inputFilePath, const std::filesystem::path& parentOutputDir, const std::string& extension, uint32_t desiredMipLevels,const flat::MipMapFilter& filter);
}


#endif