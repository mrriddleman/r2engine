#ifndef __ASSIMP_ASSET_LOADER_H__
#define __ASSIMP_ASSET_LOADER_H__

#include "r2/Core/Assets/AssetLoader.h"

namespace r2::asset
{
	class AssimpAssetLoader : public AssetLoader
	{
	public:
		virtual const char* GetPattern() override;
		virtual AssetType GetType() const override;
		virtual bool ShouldProcess() override;
		virtual u64 GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
		virtual bool LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) override;
	private:
		u64 mNumMeshes = 0;
	};
}



#endif