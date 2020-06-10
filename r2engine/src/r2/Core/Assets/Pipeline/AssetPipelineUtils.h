#ifndef __ASSET_PIPELINE_UTILS_H__
#define __ASSET_PIPELINE_UTILS_H__

#include <string>

namespace r2::asset::pln::utils
{
	bool WriteFile(const std::string& filePath, char* data, size_t size);
	char* ReadFile(const std::string& filePath);
}

#endif // __ASSET_PIPELINE_UTILS_H__
