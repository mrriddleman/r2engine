#ifndef __ASSET_CONVERTER_UTILS_H__
#define __ASSET_CONVERTER_UTILS_H__

#ifdef R2_ASSET_PIPELINE
#include <string>

namespace r2::asset::pln::assetconvert
{
	int RunConverter(const std::string& inputDir, const std::string& outputDir);
	int RunModelConverter(const std::string& inputDir, const std::string& outputDir, const std::string& materialManifestPath);
}


#endif
#endif