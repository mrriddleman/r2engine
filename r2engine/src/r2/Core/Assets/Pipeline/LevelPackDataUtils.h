#ifndef __LEVEL_PACK_DATA_UTILS_H__
#define __LEVEL_PACK_DATA_UTILS_H__

#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)

#include <string>
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Assets/AssetReference.h"

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::asset::pln
{
	struct LevelGroup
	{
		std::string groupName;
		std::vector<r2::asset::AssetReference> levelReferences;
	};

	bool WriteNewLevelDataFromBinary(const std::string& binLevelPath, const std::string& rawJSONPath, const std::string& fbs, const void* data, u32 dataSize);
	
	bool SaveLevelData(
		const r2::ecs::ECSCoordinator* coordinator,
		const std::string& binLevelPath,
		const std::string& rawJSONPath,
		const r2::Level& editorLevel);
	void RegenerateLevelDataFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool GenerateEmptyLevelPackFile(const std::string& binFilePath, const std::string& rawFilePath);


	bool SaveLevelPackData(u32 version, const std::vector<LevelGroup>& levelGroups, const std::string& binFilePath, const std::string& rawFilePath);

}

#endif
#endif // __LEVEL_PACK_DATA_UTILS_H__
