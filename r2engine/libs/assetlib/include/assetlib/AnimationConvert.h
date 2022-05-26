#ifndef __ASSET_LIB_ANIMATION_CONVERT_H__
#define __ASSET_LIB_ANIMATION_CONVERT_H__

#include <filesystem>

namespace r2::assets::assetlib
{
	bool ConvertAnimation(const std::filesystem::path& inputFilePath, const std::filesystem::path& parentOutputDir, const std::string& extension);
}

#endif