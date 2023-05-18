#ifndef __ASSET_CACHE_RECORD_H__
#define __ASSET_CACHE_RECORD_H__

#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/Asset.h"

namespace r2::asset
{
	class AssetBuffer;
	class AssetCache;

	struct AssetCacheRecord
	{
		AssetCacheRecord();
		AssetCacheRecord(const Asset& _asset, AssetBuffer* assetBuffer, AssetCache* noptrAssetCache);
		~AssetCacheRecord();

		AssetBuffer* GetAssetBuffer() const;
		const AssetCache* GetAssetCache() const;
		const Asset& GetAsset() const;
		
	private:
		mutable AssetBuffer* buffer;
		AssetCache* mnoptrAssetCache;
		Asset asset;
	};
}

#endif // __ASSET_CACHE_RECORD_H__
