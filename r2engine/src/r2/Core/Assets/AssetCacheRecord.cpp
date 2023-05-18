#include "r2pch.h"
#include "r2/Core/Assets/AssetCacheRecord.h"
#include "r2/Core/Assets/AssetCache.h"


namespace r2::asset
{
	AssetCacheRecord::AssetCacheRecord()
		:asset{}
		,buffer(nullptr)
		,mnoptrAssetCache(nullptr)
	{
	}

	AssetCacheRecord::AssetCacheRecord(const Asset& _asset, AssetBuffer* assetBuffer, AssetCache* noptrAssetCache)
		:asset(_asset)
		, buffer(assetBuffer)
		, mnoptrAssetCache(noptrAssetCache)
	{
	}

	AssetCacheRecord::~AssetCacheRecord()
	{
		buffer = nullptr;
		mnoptrAssetCache = nullptr;
	}

	r2::asset::AssetBuffer* AssetCacheRecord::GetAssetBuffer() const
	{
		if (mnoptrAssetCache)
		{
			bool isLoaded = mnoptrAssetCache->IsLoaded(asset);
			if (!isLoaded && buffer != nullptr)
			{
				buffer = nullptr;
			}
		}

		return buffer;
	}

	const r2::asset::AssetCache* AssetCacheRecord::GetAssetCache() const
	{
		return mnoptrAssetCache;
	}

	const r2::asset::Asset& AssetCacheRecord::GetAsset() const
	{
		return asset;
	}

}