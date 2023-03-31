#ifndef __LEVEL_ASSET_WRITER_H__
#define __LEVEL_ASSET_WRITER_H__

#include "r2/Core/Assets/AssetWriters/AssetWriter.h"

namespace r2::asset
{
	class LevelAssetWriter : public AssetWriter
	{
	private:
		LevelAssetWriter();
		~LevelAssetWriter();

		virtual const char* GetPattern() override;
		virtual AssetType GetType() const override;
		virtual bool WriteAsset(AssetFile& assetFile, const Asset& asset, const void* rawBuffer, u32 rawSize, u32 offset) override;
	};
}


#endif // __LEVEL_ASSET_WRITER_H__
