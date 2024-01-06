#ifndef __ASSET_COMMAND_TYPES_H__
#define __ASSET_COMMAND_TYPES_H__

#ifdef R2_ASSET_PIPELINE
#include <vector>
#include <filesystem>
namespace r2::asset::pln
{
	enum AssetHotReloadCommandType
	{
		AHRCT_GAME_ASSET = 0,
		AHRCT_TEXTURE_ASSET,
		AHRCT_MATERIAL_ASSET,
		AHRCT_MODEL_ASSET,
		AHRCT_ANIMATION_ASSET,
		AHRCT_SHADER_ASSET,
		AHRCT_SOUND_ASSET,
		AHRCT_LEVEL_ASSET
	};
	
	enum HotReloadType : u32
	{
		CHANGED = 0,
		ADDED,
		DELETED
	};

	struct AssetBuildRequest
	{
		AssetHotReloadCommandType commandType;
		HotReloadType reloadType;
		
		std::vector<std::filesystem::path> paths;
		bool forceRebuild = false;
	};
}

#endif
#endif