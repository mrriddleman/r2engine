#ifndef __RMODEL_ASSET_LOADER_H__
#define __RMODEL_ASSET_LOADER_H__

#include "r2/Core/Assets/AssetLoader.h"

namespace r2::asset
{
	class RModelAssetLoader : public AssetLoader
	{
	public:

		RModelAssetLoader();
		virtual const char* GetPattern() override;
		virtual AssetType GetType() const override;
		virtual bool ShouldProcess() override;
		virtual u64 GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
		virtual bool LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) override;	
	};
}

#endif