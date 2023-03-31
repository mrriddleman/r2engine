#ifndef __ASSET_WRITER_H__
#define __ASSET_WRITER_H__

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::asset
{
	class Asset;
	class AssetFile;

	class AssetWriter
	{
	public:
		virtual ~AssetWriter(){}
		virtual const char* GetPattern() = 0;
		virtual AssetType GetType() const = 0;
		virtual bool WriteAsset(AssetFile& assetFile, const Asset& asset, const void* rawBuffer, u32 rawSize, u32 offset) = 0;
	};
}


#endif // __ASSET_WRITER_H__
