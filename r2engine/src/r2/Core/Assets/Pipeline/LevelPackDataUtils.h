#ifndef __LEVEL_PACK_DATA_UTILS_H__
#define __LEVEL_PACK_DATA_UTILS_H__

#ifdef R2_ASSET_PIPELINE

#include <string>

namespace r2::ecs
{
	class ECSCoordinator;
}


namespace r2::asset::pln
{
	bool WriteNewLevelDataFromBinary(const std::string& binLevelPath, const std::string& rawJSONPath, const void* data, u32 dataSize);
	
	bool SaveLevelData(const r2::ecs::ECSCoordinator* coordinator, u32 version, const std::string& binLevelPath, const std::string& rawJSONPath);
	void RegenerateLevelDataFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool GenerateEmptyLevelPackFile(const std::string& binFilePath, const std::string& rawFilePath);
}

#endif
#endif // __LEVEL_PACK_DATA_UTILS_H__
