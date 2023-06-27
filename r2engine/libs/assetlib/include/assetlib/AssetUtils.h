#ifndef __ASSET_UTILS_H__
#define __ASSET_UTILS_H__

#include <cstdint>

namespace r2::asset
{
	using AssetType = uint32_t;

	enum EngineAssetType : uint32_t
	{
		DEFAULT = 0,
		TEXTURE,
		MODEL,
		MESH,
		CUBEMAP_TEXTURE,
		MATERIAL_PACK_MANIFEST,
		TEXTURE_PACK_MANIFEST,
		RMODEL,
		RANIMATION,
		LEVEL,
		LEVEL_PACK,
		MATERIAL,
		TEXTURE_PACK,
		SOUND,
		NUM_ENGINE_ASSET_TYPES
	};

	uint64_t GetAssetNameForFilePath(const char* filePath, AssetType assetType);
	void MakeAssetNameStringForFilePath(const char* filePath, char* dst, AssetType assetType);
	uint32_t GetNumberOfParentDirectoriesToIncludeForAssetType(AssetType assetType);
}

#endif // __ASSET_UTILS_H__
